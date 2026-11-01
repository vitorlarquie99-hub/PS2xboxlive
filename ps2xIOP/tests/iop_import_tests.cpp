#include "emulator/core/iop_cpu.h"
#include "emulator/core/iop_kernel.h"
#include "emulator/core/iop_memory.h"
#include "emulator/imports/iop_cdvd.h"
#include "emulator/imports/iop_imports.h"
#include "emulator/imports/iop_ioman.h"
#include "emulator/imports/iop_loadcore.h"
#include "emulator/imports/iop_timrman.h"
#include "emulator/imports/iop_vblank.h"
#include "emulator/iop_emulator_const.h"
#include <vector>
#include <functional>
#include "emulator/services/iop_rpc.h"
#include "ps2x/iop/iop_host.h"

#include <cstdint>
#include <chrono>
#include <cstring>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <string_view>

namespace
{
    using namespace ps2x::iop;
    using namespace ps2x::iop::detail;

    constexpr uint32_t kExportMagic = 0x41C00000u;
    constexpr int32_t kLibraryNotFound = -213;
    constexpr int32_t kIllegalLibrary = -214;

    class NullHost : public IopHost
    {
    public:
        bool readGuest(uint32_t, void *, size_t) const override { return false; }
        bool writeGuest(uint32_t, const void *, size_t) override { return false; }
        bool zeroGuest(uint32_t, size_t) override { return false; }
        bool normalizeGuestAddress(uint32_t, uint32_t &) const override { return false; }
        uint32_t allocateIopHandle(IopHandleKind) override { return 1u; }
        uint32_t allocateGuest(uint32_t, uint32_t) override { return 0u; }
        void freeGuest(uint32_t) override {}
        void audioCommand(uint32_t, uint32_t, GuestBuffer, GuestBuffer) override {}
        std::string hostPath(HostPathKind) const override { return {}; }
        std::string translateGuestPath(std::string_view path) const override { return std::string(path); }
        uint64_t openHostFile(std::string_view) override { return 0u; }
        bool hostFileSize(uint64_t, uint64_t &) const override { return false; }
        bool readHostFile(uint64_t, uint64_t, void *, size_t, size_t &) override { return false; }
        void closeHostFile(uint64_t) override {}
        int32_t memoryCard(const MemoryCardRequest &) override { return 0; }
        bool hasGuestFunction(uint32_t) const override { return false; }
        bool invokeGuestFunction(uint64_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t *) override { return false; }
        void log(LogLevel, std::string_view) override {}
    };

    class CdRootHost final : public NullHost
    {
    public:
        explicit CdRootHost(std::filesystem::path rootPath)
            : root(std::move(rootPath))
        {
        }

        std::string hostPath(HostPathKind kind) const override
        {
            return kind == HostPathKind::CdRoot ? root.string() : std::string{};
        }

    private:
        std::filesystem::path root;
    };

    class RecordingExecutor final : public IopGuestExecutor
    {
    public:
        uint32_t executeGuestFunction(uint32_t address,
                                      uint32_t a0,
                                      uint32_t,
                                      uint32_t,
                                      uint32_t,
                                      uint32_t gp) override
        {
            addresses.push_back(address);
            if (onCall) onCall();
            ++calls;
            lastAddress = address;
            lastArgument = a0;
            lastGp = gp;
            return callbackResult;
        }

        std::vector<uint32_t> addresses;
        std::function<void()> onCall;
        uint32_t callbackResult = 0u;
        uint32_t calls = 0u;
        uint32_t lastAddress = 0u;
        uint32_t lastArgument = 0u;
        uint32_t lastGp = 0u;
    };

    bool expect(bool condition, std::string_view message)
    {
        if (condition)
            return true;
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }

    bool testLoadcoreRebootLibraryMode()
    {
        IopMemory memory;
        IopImportRegistry imports(memory);
        IopLoadcore loadcore(memory, imports);

        IopCpuState cpu{};
        cpu.gpr[4] = 0u;
        cpu.gpr[5] = 2u;
        if (!expect(loadcore.dispatchImport(27u, cpu), "loadcore:27 was not handled") ||
            !expect(static_cast<int32_t>(cpu.gpr[2]) == kIllegalLibrary,
                    "loadcore:27 did not reject a null export table"))
            return false;

        constexpr uint32_t table = 0x1000u;
        memory.write32(table, kExportMagic);
        memory.write16(table + 8u, 0x0101u);
        memory.write16(table + 10u, 0x1234u);
        const char name[8] = {'t', 'e', 's', 't', 'l', 'i', 'b', '\0'};
        (void)memory.writeRam(table + 12u, name, sizeof(name));
        memory.write32(table + 20u, 0u);

        cpu = {};
        cpu.gpr[4] = table;
        cpu.gpr[5] = 2u;
        if (!expect(loadcore.dispatchImport(27u, cpu), "loadcore:27 rejected a valid export table") ||
            !expect(cpu.gpr[2] == 0u, "loadcore:27 returned an error for a valid export table") ||
            !expect(memory.read16(table + 10u) == 0x1232u,
                    "loadcore:27 did not replace only export mode bits 1 and 2"))
            return false;

        constexpr uint32_t invalidTable = 0x1100u;
        memory.write32(invalidTable, 0xDEADBEEFu);
        cpu = {};
        cpu.gpr[4] = invalidTable;
        cpu.gpr[5] = 6u;
        if (!expect(loadcore.dispatchImport(27u, cpu), "loadcore:27 did not consume an invalid-table call") ||
            !expect(static_cast<int32_t>(cpu.gpr[2]) == kLibraryNotFound,
                    "loadcore:27 returned the wrong invalid-table error"))
            return false;

        if (!expect(imports.registerExportTable(table), "test export table did not register"))
            return false;
        memory.write32(table, 0u);
        cpu = {};
        cpu.gpr[4] = table;
        cpu.gpr[5] = 6u;
        return expect(loadcore.dispatchImport(27u, cpu), "loadcore:27 rejected a registered table") &&
               expect(cpu.gpr[2] == 0u, "loadcore:27 returned an error for a registered table") &&
               expect(memory.read16(table + 10u) == 0x1236u,
                      "loadcore:27 did not update a registered table's mode");
    }

    bool pollEvent(IopKernel &kernel, int eventId, uint32_t bits, uint32_t resultAddress, int32_t expected)
    {
        IopCpuState cpu{};
        cpu.gpr[4] = static_cast<uint32_t>(eventId);
        cpu.gpr[5] = bits;
        cpu.gpr[6] = 0u; // WEF_AND
        cpu.gpr[7] = resultAddress;
        return expect(kernel.dispatchEventImport(11u, cpu), "PollEventFlag was not handled") &&
               expect(static_cast<int32_t>(cpu.gpr[2]) == expected, "PollEventFlag returned an unexpected result");
    }

    bool testCdvdSpecialControl()
    {
        NullHost host;
        IopMemory memory;
        IopKernel kernel(memory);
        kernel.reset();
        IopCdvd cdvd(host, memory, kernel);
        cdvd.reset();

        constexpr uint32_t param = 0x2000u;
        constexpr uint32_t eventResult = 0x2010u;
        IopCpuState cpu{};
        cpu.gpr[4] = static_cast<uint32_t>(-11); // sceCdSC: return cdvdman interrupt event flag
        cpu.gpr[5] = param;
        if (!expect(cdvd.dispatchImport(50u, cpu), "cdvdman:50 was not handled") ||
            !expect(static_cast<int32_t>(cpu.gpr[2]) > 0, "sceCdSC(-11) did not return a valid event flag"))
            return false;
        const int eventId = static_cast<int>(cpu.gpr[2]);

        if (!pollEvent(kernel, eventId, 0x29u, eventResult, 0) ||
            !expect(memory.read32(eventResult) == 0x29u, "cdvdman event flag did not start with bits 0x29"))
            return false;

        IopCpuState clear{};
        clear.gpr[4] = static_cast<uint32_t>(eventId);
        clear.gpr[5] = ~0x29u;
        if (!expect(kernel.dispatchEventImport(8u, clear), "ClearEventFlag was not handled") ||
            !pollEvent(kernel, eventId, 0x29u, eventResult, -418))
            return false;

        cpu = {};
        cpu.gpr[4] = 0x12345u;
        if (!expect(cdvd.dispatchImport(7u, cpu), "sceCdSeek was not handled") ||
            !pollEvent(kernel, eventId, 0x29u, eventResult, 0))
            return false;

        memory.write8(param, 0x30u);
        cpu = {};
        cpu.gpr[4] = static_cast<uint32_t>(-2);
        cpu.gpr[5] = param;
        if (!expect(cdvd.dispatchImport(50u, cpu), "sceCdSC(-2) was not handled") ||
            !expect(cpu.gpr[2] == 0x30u, "sceCdSC(-2) did not store the low-byte error"))
            return false;

        memory.write32(param, 0u);
        cpu = {};
        cpu.gpr[4] = static_cast<uint32_t>(-1);
        cpu.gpr[5] = param;
        if (!expect(cdvd.dispatchImport(50u, cpu), "sceCdSC(-1) was not handled") ||
            !expect(cpu.gpr[2] == 0u, "sceCdSC(-1) returned the wrong initial stream state") ||
            !expect(memory.read32(param) == 0x30u, "sceCdSC(-1) did not publish the last error"))
            return false;

        cpu = {};
        cpu.gpr[4] = 2u;
        cpu.gpr[5] = param;
        if (!expect(cdvd.dispatchImport(50u, cpu), "sceCdSC(2) was not handled") ||
            !expect(cpu.gpr[2] == 2u, "sceCdSC(2) did not update the stream state"))
            return false;

        cpu = {};
        cpu.gpr[4] = static_cast<uint32_t>(-1);
        cpu.gpr[5] = param;
        return expect(cdvd.dispatchImport(50u, cpu), "second sceCdSC(-1) was not handled") &&
               expect(cpu.gpr[2] == 2u, "sceCdSC(-1) did not preserve the stream state");
    }

    bool testCdromDevctlInterruptEvent()
    {
        NullHost host;
        IopMemory memory;
        IopKernel kernel(memory);
        IopCdvd cdvd(host, memory, kernel);
        IopIoman ioman(memory, cdvd);
        RecordingExecutor executor;
        constexpr uint32_t path = 0x1000u, stack = 0x1100u, output = 0x1200u;
        const char device[] = "cdrom0:";
        if (!memory.writeRam(path, device, sizeof(device)))
            return false;
        memory.write32(stack + 16u, output);
        memory.write32(stack + 20u, 4u);
        IopCpuState cpu{};
        cpu.gpr[4] = path;
        cpu.gpr[5] = 0x4391u;
        cpu.gpr[29] = stack;
        if (!expect(ioman.dispatchImport(31u, cpu, executor), "cdrom devctl was not handled") ||
            !expect(cpu.gpr[2] == 0u, "cdrom devctl returned an error"))
            return false;
        const uint32_t event = memory.read32(output);
        IopCpuState sc{};
        sc.gpr[4] = static_cast<uint32_t>(-11);
        if (!expect(cdvd.dispatchImport(50u, sc), "sceCdSC(-11) was not handled") ||
            !expect(event != 0u && event == sc.gpr[2], "devctl did not return the cdvd event flag") ||
            !pollEvent(kernel, static_cast<int>(event), 0x29u, output + 4u, 0))
            return false;

        memory.write32(stack + 20u, 3u);
        cpu.gpr[2] = 0u;
        if (!expect(ioman.dispatchImport(31u, cpu, executor), "short output was not handled") ||
            !expect(static_cast<int32_t>(cpu.gpr[2]) == -22, "short output should fail") ||
            !expect(memory.read32(output) == event, "short output overwrote memory"))
            return false;

        cpu.gpr[5] = 0x4392u;
        return expect(!ioman.dispatchImport(31u, cpu, executor), "unknown devctl must remain unhandled");
    }

    bool testCdvdSearchFile()
    {
        const auto suffix = std::to_string(
            static_cast<unsigned long long>(std::chrono::steady_clock::now().time_since_epoch().count()));
        const std::filesystem::path root =
            std::filesystem::temp_directory_path() / ("ps2x-iop-cdvd-search-" + suffix);
        const std::filesystem::path movieDirectory = root / "MOVIE";
        const std::filesystem::path moviePath = movieDirectory / "OPENING.PSS";
        std::error_code error;
        std::filesystem::create_directories(movieDirectory, error);
        if (!expect(!error, "could not create the temporary CD root"))
            return false;
        {
            std::ofstream movie(moviePath, std::ios::binary);
            movie.write("PSS!", 4);
        }

        CdRootHost host(root);
        IopMemory memory;
        IopKernel kernel(memory);
        kernel.reset();
        IopCdvd cdvd(host, memory, kernel);
        cdvd.reset();

        constexpr uint32_t resultAddress = 0x2400u;
        constexpr uint32_t pathAddress = 0x2480u;
        const char path[] = "cdrom0:\\movie\\opening.pss;1";
        (void)memory.writeRam(pathAddress, path, sizeof(path));

        IopCpuState cpu{};
        cpu.gpr[4] = resultAddress;
        cpu.gpr[5] = pathAddress;
        const bool handled = cdvd.dispatchImport(10u, cpu);
        bool passed =
            expect(handled, "cdvdman:10 was not handled") &&
            expect(cpu.gpr[2] == 1u, "sceCdSearchFile did not find a case-insensitive ISO path") &&
            expect(memory.read32(resultAddress) >= 20u, "sceCdSearchFile returned an invalid LSN") &&
            expect(memory.read32(resultAddress + 4u) == 4u, "sceCdSearchFile returned the wrong size") &&
            expect(memory.readString(resultAddress + 8u, 16u) == "OPENING.PSS",
                   "sceCdSearchFile returned the wrong file name");

        IopIoman ioman(memory, cdvd);
        RecordingExecutor executor;
        cpu = {};
        cpu.gpr[4] = pathAddress;
        cpu.gpr[5] = resultAddress;
        memory.write32(resultAddress + 40u, 0xAABBCCDDu);
        passed = expect(ioman.dispatchImport(16u, cpu, executor), "cdrom getstat was not handled") &&
                 expect(cpu.gpr[2] == 0u, "cdrom getstat returned an error") &&
                 expect(memory.read32(resultAddress) == 0x14u, "cdrom getstat returned the wrong mode") &&
                 expect(memory.read32(resultAddress + 8u) == 4u, "cdrom getstat returned the wrong size") &&
                 expect(memory.read32(resultAddress + 40u) == 0xAABBCCDDu, "getstat exceeded the legacy structure") && passed;

        const char missing[] = "cdrom0:\\missing.bin;1";
        (void)memory.writeRam(pathAddress, missing, sizeof(missing));
        passed = expect(ioman.dispatchImport(16u, cpu, executor), "missing cdrom file was not handled") &&
                 expect(static_cast<int32_t>(cpu.gpr[2]) == -2, "missing cdrom file should return ENOENT") && passed;

        (void)memory.writeRam(pathAddress, path, sizeof(path));
        cpu.gpr[5] = 0u;
        passed = expect(ioman.dispatchImport(16u, cpu, executor), "invalid getstat output was not handled") &&
                 expect(static_cast<int32_t>(cpu.gpr[2]) == -22, "invalid getstat output should fail") && passed;

        const char otherDevice[] = "host0:\\opening.pss";
        (void)memory.writeRam(pathAddress, otherDevice, sizeof(otherDevice));
        passed = expect(!ioman.dispatchImport(16u, cpu, executor), "other devices must remain unhandled") && passed;

        std::filesystem::remove_all(root, error);
        return passed;
    }

    bool testVblankCallbacks()
    {
        IopMemory memory;
        IopKernel kernel(memory);
        IopVblank vblank(kernel);
        RecordingExecutor executor;
        executor.callbackResult = 1u;
        const auto registration = [&](unsigned phase, int priority, uint32_t function)
        {
            IopCpuState cpu{};
            cpu.gpr[4] = phase; cpu.gpr[5] = static_cast<uint32_t>(priority);
            cpu.gpr[6] = function; cpu.gpr[7] = 0x1234u; cpu.gpr[28] = 0x5678u;
            (void)vblank.dispatchImport(8u, cpu, 0u);
            return static_cast<int32_t>(cpu.gpr[2]);
        };
        const auto release = [&](unsigned phase, uint32_t function)
        {
            IopCpuState cpu{};
            cpu.gpr[4] = phase; cpu.gpr[5] = function;
            (void)vblank.dispatchImport(9u, cpu, 0u);
            return static_cast<int32_t>(cpu.gpr[2]);
        };
        if (!expect(registration(0u, 20, 0x100u) == 0 && registration(0u, -1, 0x200u) == 0 &&
                    registration(0u, 20, 0x300u) == 0 && registration(1u, 0, 0x400u) == 0,
                    "VBlank registration failed") ||
            !expect(registration(0u, 0, 0x100u) == -104, "duplicate VBlank handler was accepted") ||
            !expect(release(1u, 0x100u) == -105, "release ignored VBlank phase") ||
            !expect(vblank.nextEventCycle(UINT64_MAX) == kVblankEndPhaseCycles,
                    "first VBlank end deadline was lost")) return false;
        vblank.serviceDue(kVblankEndPhaseCycles - 1u, executor);
        if (!expect(executor.calls == 0u, "VBlank callback ran early")) return false;
        vblank.serviceDue(kVblankEndPhaseCycles, executor);
        if (!expect(executor.addresses == std::vector<uint32_t>{0x400u} &&
                    executor.lastArgument == 0x1234u && executor.lastGp == 0x5678u,
                    "VBlank end callback lost its function, argument or GP")) return false;
        bool contextRejected = false;
        executor.onCall = [&]
        {
            vblank.serviceDue(kVblankPeriodCycles, executor); // Must not recurse.
            contextRejected = registration(0u, 0, 0x900u) == -100 && release(0u, 0x100u) == -100;
        };
        vblank.serviceDue(kVblankPeriodCycles, executor);
        if (!expect(contextRejected && executor.addresses == std::vector<uint32_t>{0x400u, 0x200u, 0x100u, 0x300u},
                    "VBlank priority, equal-priority order or interrupt context is wrong")) return false;
        executor.onCall = {};
        if (!expect(release(0u, 0x100u) == 0 && release(0u, 0x100u) == -105,
                    "VBlank release lifecycle failed")) return false;
        executor.callbackResult = 0u;
        vblank.serviceDue(4u * kVblankPeriodCycles, executor);
        const uint32_t calls = executor.calls;
        vblank.serviceDue(8u * kVblankPeriodCycles, executor);
        if (!expect(calls == 7u && executor.calls == calls && vblank.nextEventCycle(123u) == 123u,
                    "VBlank zero-return removal or missed-field coalescing failed")) return false;
        for (unsigned index = 0; index < 14u; ++index)
            if (!expect(registration(index % 2u, 0, 0x100u + index * 4u) == 0,
                        "VBlank handler capacity is too small")) return false;
        if (!expect(registration(0u, 0, 0x999u) == -400, "VBlank slot exhaustion was not reported")) return false;
        vblank.reset();
        vblank.serviceDue(10u * kVblankPeriodCycles, executor);
        return expect(executor.calls == calls && vblank.nextEventCycle(123u) == 123u &&
                      registration(0u, 0, 0x100u) == 0, "VBlank reset retained callbacks or slots");
    }

    bool testTimrmanPeriodicCallback()
    {
        IopTimrman timrman;
        timrman.reset();
        IopCpuState cpu{};

        cpu.gpr[4] = 1u; // SYSCLK
        cpu.gpr[5] = 32u;
        cpu.gpr[6] = 1u;
        if (!expect(timrman.dispatchImport(4u, cpu, 100u), "AllocHardTimer was not handled") ||
            !expect(static_cast<int32_t>(cpu.gpr[2]) > 0, "AllocHardTimer did not allocate a 32-bit timer"))
            return false;
        const uint32_t timerId = cpu.gpr[2];

        cpu = {};
        cpu.gpr[4] = timerId;
        cpu.gpr[5] = 100u;
        cpu.gpr[6] = 0x12340u;
        cpu.gpr[7] = 0x45670u;
        cpu.gpr[28] = 0x89AB0u;
        if (!expect(timrman.dispatchImport(20u, cpu, 100u), "SetTimerHandler was not handled") ||
            !expect(cpu.gpr[2] == 0u, "SetTimerHandler failed"))
            return false;

        cpu = {};
        cpu.gpr[4] = timerId;
        cpu.gpr[5] = 1u;
        cpu.gpr[6] = 0u;
        cpu.gpr[7] = 1u;
        if (!expect(timrman.dispatchImport(22u, cpu, 100u), "SetupHardTimer was not handled") ||
            !expect(cpu.gpr[2] == 0u, "SetupHardTimer failed"))
            return false;

        cpu = {};
        cpu.gpr[4] = timerId;
        if (!expect(timrman.dispatchImport(23u, cpu, 100u), "StartHardTimer was not handled") ||
            !expect(cpu.gpr[2] == 0u, "StartHardTimer failed") ||
            !expect(timrman.nextEventCycle(1000u) == 200u, "timer compare was scheduled at the wrong cycle"))
            return false;

        RecordingExecutor executor;
        executor.callbackResult = 100u;
        timrman.serviceDue(199u, executor);
        if (!expect(executor.calls == 0u, "timer callback ran too early"))
            return false;
        timrman.serviceDue(200u, executor);
        return expect(executor.calls == 1u, "timer callback did not run") &&
               expect(executor.lastAddress == 0x12340u, "timer called the wrong handler") &&
               expect(executor.lastArgument == 0x45670u, "timer passed the wrong common argument") &&
               expect(executor.lastGp == 0x89AB0u, "timer callback lost the registering module GP") &&
               expect(timrman.nextEventCycle(1000u) == 300u, "timer callback return did not rearm compare");
    }
}

int main()
{
    if (!testLoadcoreRebootLibraryMode() || !testCdvdSpecialControl() || !testCdromDevctlInterruptEvent() || !testCdvdSearchFile() ||
        !testTimrmanPeriodicCallback() || !testVblankCallbacks())
        return 1;
    std::cout << "ps2xIOP import tests passed\n";
    return 0;
}
