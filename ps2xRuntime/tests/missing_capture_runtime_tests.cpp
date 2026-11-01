#include "ps2_runtime.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace
{
    void setCaptureEnv(const char *value)
    {
#ifdef _WIN32
        _putenv_s("PS2X_CAPTURE_MISSING_DIR", value ? value : "");
#else
        if (value) setenv("PS2X_CAPTURE_MISSING_DIR", value, 1);
        else unsetenv("PS2X_CAPTURE_MISSING_DIR");
#endif
    }
}

int main()
{
    const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto directory = std::filesystem::temp_directory_path() / ("ps2x-runtime-capture-" + suffix);
    const char *previous = std::getenv("PS2X_CAPTURE_MISSING_DIR");
    const bool hadPrevious = previous != nullptr;
    const std::string saved = previous ? previous : "";
    const std::string destination = directory.string();
    setCaptureEnv(destination.c_str());
    bool passed = false;
    {
        PS2Runtime runtime;
        runtime.setMissingFunctionPolicy(PS2Runtime::MissingFunctionPolicy::Stop);
        if (runtime.memory().initialize(128))
        {
            uint8_t *ram = runtime.memory().getRDRAM();
            ram[0] = 0x5a;
            ram[127] = 0xa5;
            R5900Context ctx{};
            runtime.reportMissingFunction(ram, &ctx, 0x2000, 0x1000,
                                          PS2Runtime::GuestBranchKind::DirectCall, "test");
            std::ifstream dump(directory / "ram.bin", std::ios::binary);
            const std::string bytes((std::istreambuf_iterator<char>(dump)), {});
            std::ifstream info(directory / "target.txt");
            const std::string metadata((std::istreambuf_iterator<char>(info)), {});
            passed = bytes.size() == 128 && static_cast<unsigned char>(bytes[0]) == 0x5a &&
                     static_cast<unsigned char>(bytes[127]) == 0xa5 &&
                     metadata == "source_pc=0x1000\ntarget_pc=0x2000\nram_size=128\n" &&
                     runtime.isStopRequested() && ctx.pc == 0x2000;
        }
    }
    setCaptureEnv(hadPrevious ? saved.c_str() : nullptr);
    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
    if (!passed || ec)
    {
        std::cerr << "runtime capture integration test failed\n";
        return 1;
    }
    return 0;
}
