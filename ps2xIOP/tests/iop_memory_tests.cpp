#include "emulator/core/iop_memory.h"
#include "emulator/services/iop_module_loader.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
using namespace ps2x::iop::detail;
static void check(bool ok) { if (!ok) throw std::runtime_error("IOP memory regression"); }
static std::vector<uint8_t> image(uint32_t offset)
{
    std::vector<uint8_t> elf(88);
    auto u16 = [&](size_t at, uint16_t value) { std::memcpy(elf.data()+at,&value,2); };
    auto u32 = [&](size_t at, uint32_t value) { std::memcpy(elf.data()+at,&value,4); };
    elf[0]=0x7f; elf[1]='E'; elf[2]='L'; elf[3]='F'; elf[4]=1; elf[5]=1; elf[6]=1;
    u16(16,2); u16(18,8); u32(20,1); u32(24,0x10000); u32(28,52);
    u16(40,52); u16(42,32); u16(44,1);
    u32(52,1); u32(56,offset); u32(60,0x10000); u32(68,4); u32(72,4096);
    return elf;
}
int main()
{
    IopMemory m;
    check(m.read8(0xBF808264u) == 0xFFu);
    check(m.read32(0x1F80826Cu) == 0x1D100u);
    m.write32(0x1F80826Cu, 0u); check(m.read32(0x1F80826Cu) == 0x1D100u);
    m.write32(0xBF808268u, 0x3BCu); check(m.interruptStatus() == 0u);
    m.write32(0xBF808268u, 0x3BDu);
    check(m.read32(0x1F808280u) == 1u && (m.interruptStatus() & (1u << 17u)) != 0u);
    m.write32(0x1F801070u, ~(1u << 17u));
    m.write32(0x1F808268u, 0x3BDu); check(m.interruptStatus() == 0u); // Latched; no duplicate edge.
    m.write16(0x1F808282u, 0xFFFFu); check(m.read32(0x1F808280u) == 1u);
    m.write8(0x1F808281u, 0xFFu); check(m.read32(0x1F808280u) == 1u);
    m.write32(0x1F808280u, 0u); check(m.read32(0x1F808280u) == 1u);
    m.write32(0x1F808280u, 2u); check(m.read32(0x1F808280u) == 1u);
    m.write32(0x1F808280u, 1u); check(m.read32(0x1F808280u) == 0u);
    m.write32(0x1F808268u, 0x3BDu); check((m.interruptStatus() & (1u << 17u)) != 0u);
    m.reset(); check(m.interruptStatus() == 0u && m.read32(0x1F808280u) == 0u);
    m.setControllerReader([](unsigned port, uint16_t &buttons) { buttons=0xBFF7u; return port==0u; });
    m.write32(0x1F808200u, 0x00140540u);
    for (uint8_t v : {1,0x42,0,0,0}) m.write8(0x1F808260u,v);
    for (uint8_t v : {0xFF,0x41,0x5A,0xF7,0xBF}) check(m.read8(0x1F808264u)==v);
    check(m.read8(0x1F808264u)==0xFFu && m.read32(0x1F80826Cu)==0x1100u);
    m.write32(0x1F808200u, 0x00140541u);
    for (uint8_t v : {1,0x42,0,0,0}) m.write8(0x1F808260u,v);
    for (unsigned i=0;i<5;++i) check(m.read8(0x1F808264u)==0xFFu);
    check(m.read32(0x1F80826Cu)==0x2D100u);
    m.write32(0x1F808200u, 0x00140540u);
    for (uint8_t v : {1,0x99,0,0,0}) m.write32(0x1F808260u,v);
    for (unsigned i=0;i<5;++i) check(m.read8(0x1F808264u)==0xFFu);
    m.reset();
    m.write32(0x1F808200u, 0x00140540u);
    for (uint8_t v : {1,0x42,0,0,0}) m.write16(0x1F808260u,v);
    check(m.read16(0x1F808264u)==0xFFu && m.read8(0x1F808264u)==0x41u);
    check(m.read32(0x1F808264u)==0x5Au);
    check(m.read8(0x1F808265u)==0u && m.read8(0x1F808264u)==0xF7u);
    m.reset(); check(m.read8(0x1F808264u)==0xFFu);
    const auto packet = [&](unsigned port, uint8_t command, uint8_t argument, unsigned length=5u)
    {
        m.write32(0x1F808200u, (5u << 18u) | (length << 8u) | port);
        for (unsigned i=0; i<length; ++i)
            m.write8(0x1F808260u, i==0 ? 1u : i==1 ? command : i==3 ? argument : 0u);
        std::vector<uint8_t> reply;
        for (unsigned i=0; i<5; ++i) reply.push_back(m.read8(0x1F808264u));
        return reply;
    };
    check(packet(0,0x43,1)==std::vector<uint8_t>({0xFF,0x41,0x5A,0,0}));
    check(packet(0,0x43,1)==std::vector<uint8_t>({0xFF,0xF3,0x5A,0,0}));
    check(packet(0,0x42,0)==std::vector<uint8_t>({0xFF,0xF3,0x5A,0xF7,0xBF}));
    check(packet(0,0x99,0)==std::vector<uint8_t>(5,0xFF));
    check(packet(1,0x43,1)==std::vector<uint8_t>(5,0xFF));
    check(packet(0,0x43,0,3)==std::vector<uint8_t>({0xFF,0xF3,0x5A,0,0})); // Missing selector preserves state.
    check(packet(0,0x43,0)==std::vector<uint8_t>({0xFF,0xF3,0x5A,0,0}));
    check(packet(0,0x42,0)==std::vector<uint8_t>({0xFF,0x41,0x5A,0xF7,0xBF}));
    (void)packet(0,0x43,1);
    m.reset();
    check(packet(0,0x42,0)==std::vector<uint8_t>({0xFF,0x41,0x5A,0xF7,0xBF}));
    m.setControllerAxesReader([](unsigned port,std::array<uint8_t,4> &axes) { axes={1,2,3,4}; return port==0; });
    const auto command = [&](uint8_t cmd, std::vector<uint8_t> args={}, unsigned receive=9u)
    {
        std::vector<uint8_t> input{1,cmd,0}; input.insert(input.end(),args.begin(),args.end()); input.resize(9,0);
        m.write32(0x1F808200u,(receive<<18u)|(9u<<8u));
        for (auto v:input) m.write8(0x1F808260u,v);
        std::vector<uint8_t> reply; for(unsigned i=0;i<receive;++i) reply.push_back(m.read8(0x1F808264u));
        return reply;
    };
    check(command(0x45)==std::vector<uint8_t>(9,0xFF)); // Config-only query rejected outside config.
    (void)command(0x43,{1});
    check(command(0x45)==std::vector<uint8_t>({0xFF,0xF3,0x5A,3,2,0,2,1,0}));
    check(command(0x46,{0})==std::vector<uint8_t>({0xFF,0xF3,0x5A,0,0,1,2,0,0x0A}));
    check(command(0x46,{1})==std::vector<uint8_t>({0xFF,0xF3,0x5A,0,0,1,1,1,0x14}));
    check(command(0x47)==std::vector<uint8_t>({0xFF,0xF3,0x5A,0,0,2,0,1,0}));
    check(command(0x4C,{0})[6]==4 && command(0x4C,{1})[6]==7);
    check(command(0x40)[8]==0x5A);
    check(command(0x4D,{0,1})==std::vector<uint8_t>({0xFF,0xF3,0x5A,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}));
    auto map=command(0x4D,{1,0}); check(map[3]==0 && map[4]==1);
    (void)command(0x44,{1,3});
    check(command(0x45)[5]==1);
    check(command(0x41)==std::vector<uint8_t>({0xFF,0xF3,0x5A,0xFF,0xFF,3,0,0,0x5A}));
    (void)command(0x43,{0});
    check(command(0x42)==std::vector<uint8_t>({0xFF,0x73,0x5A,0xF7,0xBF,1,2,3,4}));
    (void)command(0x43,{1}); (void)command(0x4F,{0xFF,0xFF,3}); (void)command(0x43,{0});
    auto pressure=command(0x42,{},21); check(pressure[1]==0x79 && pressure[15]==0xFF && pressure[9]==0);
    (void)command(0x43,{1}); (void)command(0x4F,{0x3F,0,0}); (void)command(0x43,{0});
    check(command(0x42)[1]==0x73);
    m.reset(); check(command(0x42)[1]==0x41);
    const auto initial=m.totalFreeMemory();
    const auto loaded=IopModuleLoader::load(image(84),m,0x10000);
    check(loaded.error==IopImageLoadError::None);
    check(!m.freeAllocation(0x10000)); // A guest free cannot remove a module reservation.
    check(m.allocate(4096,16,0x10000)==0);
    auto a=m.allocate(0x58000,64), b=m.allocate(0x58000,64), c=m.allocate(0x58000,64);
    check(a && b && c && a!=b && b!=c);
    check(m.freeAllocation(b));
    const auto free=m.totalFreeMemory();
    check(m.allocate(0x58000,64)==b);
    check(m.totalFreeMemory()==free-0x58000);
    check(m.allocate(0xffffffffu)==0 && m.allocate(16,3)==0);
    check(m.allocate(32,16,0xfffffff0)==0);
    m.reset(); check(m.totalFreeMemory()==initial && m.maxFreeMemory()==initial);
    const auto failure=IopModuleLoader::load(image(1000),m,0x10000);
    check(failure.error!=IopImageLoadError::None && failure.nextModuleCursor==0x10000);
    check(m.totalFreeMemory()==initial); // Failed image releases its reservation.
    check(m.allocate(4096,16,0x10000)==0x10000);
    check(IopModuleLoader::load(image(84),m,0x10000).error==IopImageLoadError::ArenaExhausted);
    m.reset();
    check(m.reserveModule(0x120000,4096)); // Former heap boundary is no longer a module ceiling.
    check(m.allocate(4096,16,0x120000)==0);
    check(m.maxFreeMemory()<m.totalFreeMemory());
    std::cout << "IOP shared memory tests passed\n";
}
