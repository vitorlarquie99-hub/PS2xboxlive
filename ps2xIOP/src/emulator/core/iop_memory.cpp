#include "iop_memory.h"

#include <algorithm>
#include <cstring>

namespace ps2x::iop::detail
{
    namespace
    {
        constexpr uint32_t kDmaSpu0Chcr = 0x1F8010C8u;
        constexpr uint32_t kDmaSpu1Chcr = 0x1F801508u;
        constexpr uint32_t kDmaStart = 1u << 24u;
        constexpr int kDmaSpu0Irq = 0x24;
        constexpr int kDmaSpu1Irq = 0x28;

        uint32_t alignUp(uint32_t value, uint32_t alignment)
        {
            return (value + alignment - 1u) & ~(alignment - 1u);
        }
    }

    IopMemory::IopMemory()
        : m_ram(RamSize), m_owned(RamSize), m_scratch(ScratchSize)
    {
        reset();
    }

    void IopMemory::reset()
    {
        std::fill(m_ram.begin(), m_ram.end(), uint8_t{0});
        std::fill(m_owned.begin(), m_owned.end(), uint8_t{0});
        std::fill(m_scratch.begin(), m_scratch.end(), uint8_t{0});
        m_sio2Output.clear();
        m_sio2Input.clear();
        m_sio2Command = 0u;
        m_sio2Config = {};
        m_sio2Mode = {0x41u, 0x41u};
        for (auto &map : m_sio2MotorMap) map.fill(0xFFu);
        m_hardware.clear();
        // Until a device protocol is attached, report an empty SIO2 bus.
        m_hardware[0x1F80826Cu] = 0x1D100u;
        m_hardware[0x1F808270u] = 0xFu;

        m_allocations.clear();
        m_interruptStatus = 0;
        m_interruptMask = 0;
        m_interruptControl = 1;
        m_dmaStart.reset();
    }

    uint32_t IopMemory::physicalAddress(uint32_t address) noexcept
    {
        return address & 0x1FFFFFFFu;
    }

    uint8_t IopMemory::read8(uint32_t address) const
    {
        const uint32_t phys = physicalAddress(address);
        if (phys < RamSize)
            return m_ram[phys];
        if (phys >= ScratchBase && phys < ScratchBase + ScratchSize)
            return m_scratch[phys - ScratchBase];
        // The receive FIFO occupies the low byte only. Wider reads must not
        // consume another byte while reading the unused register lanes.
        if (phys > 0x1F808264u && phys < 0x1F808268u) return 0u;
        const uint32_t value = readHardware32(phys & ~3u);
        return static_cast<uint8_t>(value >> ((phys & 3u) * 8u));
    }

    uint16_t IopMemory::read16(uint32_t address) const
    {
        const uint32_t phys = physicalAddress(address);
        if (phys + 1u < RamSize)
        {
            uint16_t value;
            std::memcpy(&value, m_ram.data() + phys, sizeof(value));
            return value;
        }
        return static_cast<uint16_t>(read8(address) | (static_cast<uint16_t>(read8(address + 1u)) << 8u));
    }

    uint32_t IopMemory::read32(uint32_t address) const
    {
        const uint32_t phys = physicalAddress(address);
        if ((phys & 3u) == 0u && phys + 3u < RamSize)
        {
            uint32_t value;
            std::memcpy(&value, m_ram.data() + phys, sizeof(value));
            return value;
        }
        if ((phys & 3u) == 0u && phys >= ScratchBase && phys + 3u < ScratchBase + ScratchSize)
        {
            uint32_t value;
            std::memcpy(&value, m_scratch.data() + (phys - ScratchBase), sizeof(value));
            return value;
        }
        if ((phys & 3u) == 0u && isHardwareAddress(phys))
            return readHardware32(phys);

        return static_cast<uint32_t>(read8(address)) |
               (static_cast<uint32_t>(read8(address + 1u)) << 8u) |
               (static_cast<uint32_t>(read8(address + 2u)) << 16u) |
               (static_cast<uint32_t>(read8(address + 3u)) << 24u);
    }

    void IopMemory::write8(uint32_t address, uint8_t value)
    {
        const uint32_t phys = physicalAddress(address);
        if (phys < RamSize)
        {
            m_ram[phys] = value;
            markOwned(phys, sizeof(value));
            return;
        }
        if (phys >= ScratchBase && phys < ScratchBase + ScratchSize)
        {
            m_scratch[phys - ScratchBase] = value;
            return;
        }
        if (phys >= 0x1F808260u && phys < 0x1F808264u)
        {
            if (phys == 0x1F808260u) writeSio2Byte(value);
            return;
        }
        const uint32_t aligned = phys & ~3u;
        const uint32_t shift = (phys & 3u) * 8u;
        if (aligned == 0x1F808280u)
        {
            writeHardware32(aligned, static_cast<uint32_t>(value) << shift);
            return;
        }
        uint32_t current = readHardware32(aligned);
        current = (current & ~(0xFFu << shift)) | (static_cast<uint32_t>(value) << shift);
        writeHardware32(aligned, current);
    }

    void IopMemory::write16(uint32_t address, uint16_t value)
    {
        const uint32_t phys = physicalAddress(address);
        if (phys + 1u < RamSize)
        {
            std::memcpy(m_ram.data() + phys, &value, sizeof(value));
            markOwned(phys, sizeof(value));
            return;
        }
        write8(address, static_cast<uint8_t>(value));
        write8(address + 1u, static_cast<uint8_t>(value >> 8u));
    }

    void IopMemory::write32(uint32_t address, uint32_t value)
    {
        const uint32_t phys = physicalAddress(address);
        if ((phys & 3u) == 0u && phys + 3u < RamSize)
        {
            std::memcpy(m_ram.data() + phys, &value, sizeof(value));
            markOwned(phys, sizeof(value));
            return;
        }
        if ((phys & 3u) == 0u && phys >= ScratchBase && phys + 3u < ScratchBase + ScratchSize)
        {
            std::memcpy(m_scratch.data() + (phys - ScratchBase), &value, sizeof(value));
            return;
        }
        if ((phys & 3u) == 0u)
        {
            writeHardware32(phys, value);
            return;
        }
        write8(address, static_cast<uint8_t>(value));
        write8(address + 1u, static_cast<uint8_t>(value >> 8u));
        write8(address + 2u, static_cast<uint8_t>(value >> 16u));
        write8(address + 3u, static_cast<uint8_t>(value >> 24u));
    }

    bool IopMemory::readRam(uint32_t address, void *destination, size_t size) const
    {
        const uint32_t phys = physicalAddress(address);
        if ((!destination && size != 0u) || phys > RamSize || size > RamSize - phys)
            return false;
        if (size != 0u)
            std::memcpy(destination, m_ram.data() + phys, size);
        return true;
    }

    bool IopMemory::writeRam(uint32_t address, const void *source, size_t size)
    {
        const uint32_t phys = physicalAddress(address);
        if ((!source && size != 0u) || phys > RamSize || size > RamSize - phys)
            return false;
        if (size != 0u)
        {
            std::memcpy(m_ram.data() + phys, source, size);
            markOwned(phys, size);
        }
        return true;
    }

    bool IopMemory::zeroRam(uint32_t address, size_t size)
    {
        const uint32_t phys = physicalAddress(address);
        if (phys > RamSize || size > RamSize - phys)
            return false;
        if (size != 0u)
        {
            std::memset(m_ram.data() + phys, 0, size);
            markOwned(phys, size);
        }
        return true;
    }

    bool IopMemory::ownsRamRange(uint32_t address, size_t size) const
    {
        const uint32_t phys = physicalAddress(address);
        if (phys > RamSize || size > RamSize - phys)
            return false;
        return std::all_of(m_owned.begin() + phys, m_owned.begin() + phys + size,
                           [](uint8_t value)
                           { return value != 0u; });
    }

    void IopMemory::markOwned(uint32_t address, size_t size)
    {
        if (address > RamSize || size > RamSize - address)
            return;
        std::fill(m_owned.begin() + address, m_owned.begin() + address + size, uint8_t{1});
    }

    bool IopMemory::isHardwareAddress(uint32_t address) const
    {
        const uint32_t phys = physicalAddress(address);
        return (phys >= HardwareBase && phys < HardwareEnd) ||
               (phys >= Spu2Base && phys < Spu2End) ||
               (phys >= SifBase && phys < SifEnd);
    }

    uint32_t IopMemory::readHardware32(uint32_t address) const
    {
        if (address == 0x1F808264u)
        {
            if (m_sio2Output.empty()) return 0xFFu;
            const uint8_t value = m_sio2Output.front();
            m_sio2Output.pop_front();
            return value;
        }
        const auto value = m_hardware.find(address);
        if (value != m_hardware.end())
            return value->second;
        switch (address)
        {
        case 0x1F801070u:
            return m_interruptStatus;
        case 0x1F801074u:
            return m_interruptMask;
        case 0x1F801078u:
            return m_interruptControl;
        default:
            return 0u;
        }
    }

    void IopMemory::writeHardware32(uint32_t address, uint32_t value)
    {
        switch (address)
        {
        case 0x1F808260u:
            writeSio2Byte(static_cast<uint8_t>(value));
            return;
        case 0x1F808200u: // First descriptor starts a fresh command queue.
            m_sio2Input.clear();
            m_sio2Output.clear();
            m_sio2Command = 0u;
            m_hardware[0x1F80826Cu] = 0u;
            m_hardware[address] = value;
            return;
        case 0x1F808280u: // SIO2 interrupt status: write one to acknowledge.
            m_hardware[address] &= ~value;
            return;
        case 0x1F808264u: // Receive FIFO and status registers are read-only.
        case 0x1F80826Cu:
        case 0x1F808270u:
        case 0x1F808274u:
            return;
        case 0x1F808268u:
            m_hardware[address] = value;
            if ((value & 1u) != 0u && (m_hardware[0x1F808280u] & 1u) == 0u)
            {
                m_hardware[0x1F808280u] |= 1u;
                m_interruptStatus |= 1u << 17u;
            }
            return;
        case 0x1F801070u:
            m_interruptStatus &= value;
            return;
        case 0x1F801074u:
            m_interruptMask = value;
            return;
        case 0x1F801078u:
            m_interruptControl = value & 1u;
            return;
        default:
            break;
        }

        m_hardware[address] = value;
        if ((address != kDmaSpu0Chcr && address != kDmaSpu1Chcr) || (value & kDmaStart) == 0u)
            return;

        const bool secondCore = address == kDmaSpu1Chcr;
        m_hardware[address] = value & ~kDmaStart;

        const uint32_t statusAddress = 0x1F900344u + (secondCore ? 0x400u : 0u);
        const uint32_t alignedStatus = statusAddress & ~3u;
        const uint32_t shift = (statusAddress & 2u) * 8u;
        uint32_t status = 0u;
        if (const auto current = m_hardware.find(alignedStatus); current != m_hardware.end())
            status = current->second;
        status |= 0x80u << shift;
        m_hardware[alignedStatus] = status;

        const uint32_t blockControlAddress = address - sizeof(uint32_t);
        uint32_t blockControl = 0u;
        if (const auto current = m_hardware.find(blockControlAddress); current != m_hardware.end())
            blockControl = current->second;
        const uint32_t wordsPerBlock = std::max<uint32_t>(blockControl & 0xFFFFu, 1u);
        const uint32_t blockCount = std::max<uint32_t>(blockControl >> 16u, 1u);
        const uint64_t transferWords = static_cast<uint64_t>(wordsPerBlock) * blockCount;
        m_dmaStart = DmaStart{
            secondCore ? kDmaSpu1Irq : kDmaSpu0Irq,
            std::max<uint64_t>(transferWords * 2u, 64u),
        };
    }

    void IopMemory::writeSio2Byte(uint8_t value)
    {
        if (m_sio2Command >= 16u) return;
        const uint32_t descriptor = readHardware32(0x1F808200u + m_sio2Command * 4u);
        const unsigned length = (descriptor >> 8u) & 0x1FFu;
        if (length == 0u) return;
        m_sio2Input.push_back(value);
        if (m_sio2Input.size() < length) return;
        const unsigned port = descriptor & 1u;
        uint16_t buttons = 0xFFFFu;
        const uint8_t command = length >= 2u ? m_sio2Input[1] : 0u;
        const bool query = command == 0x40u || command == 0x41u || command == 0x44u || command == 0x45u ||
                           command == 0x46u || command == 0x47u || command == 0x4Cu || command == 0x4Du || command == 0x4Fu;
        const bool connected = length >= 2u && m_sio2Input[0] == 1u &&
                               (command == 0x42u || command == 0x43u || (m_sio2Config[port] && query)) &&
                               m_controllerReader && m_controllerReader(port, buttons);
        const bool wasInConfig = m_sio2Config[port];
        std::array<uint8_t, 20> payload{};
        const bool selector = length > 3u && m_sio2Input[3] != 0u;
        if (command == 0x42u)
        {
            payload[0] = static_cast<uint8_t>(buttons);
            payload[1] = static_cast<uint8_t>(buttons >> 8u);
            std::array<uint8_t, 4> axes{0x80u,0x80u,0x80u,0x80u};
            if (connected && m_sio2Mode[port] != 0x41u && m_controllerAxesReader)
                (void)m_controllerAxesReader(port, axes);
            std::copy(axes.begin(), axes.end(), payload.begin()+2);
            constexpr unsigned pressureBits[] = {5,7,4,6,12,13,14,15,10,11,8,9};
            for (unsigned i=0;i<12;++i) payload[6+i] = (buttons & (1u << pressureBits[i])) == 0u ? 0xFFu : 0u;
        }
        else if (command == 0x40u) { payload[2]=2u; payload[5]=0x5Au; }
        else if (command == 0x41u && m_sio2Mode[port] != 0x41u)
            { payload[0]=0xFFu; payload[1]=0xFFu; payload[2]=3u; payload[5]=0x5Au; }
        else if (command == 0x45u)
            { payload[0]=3u; payload[1]=2u; payload[2]=m_sio2Mode[port]!=0x41u; payload[3]=2u; payload[4]=1u; }
        else if (command == 0x46u)
            { payload[2]=1u; payload[3]=selector?1u:2u; payload[4]=selector?1u:0u; payload[5]=selector?0x14u:0x0Au; }
        else if (command == 0x47u) { payload[2]=2u; payload[4]=1u; }
        else if (command == 0x4Cu) { payload[3]=selector?7u:4u; }
        else if (command == 0x4Du) std::copy(m_sio2MotorMap[port].begin(),m_sio2MotorMap[port].end(),payload.begin());
        else if (command == 0x4Fu) payload[5]=0x5Au;
        const unsigned receiveLength = (descriptor >> 18u) & 0x1FFu;
        for (unsigned index = 0u; index < receiveLength; ++index)
        {
            uint8_t response = 0xFFu;
            if (connected)
            {
                if (index == 1u) response = wasInConfig ? 0xF3u : m_sio2Mode[port];
                else if (index == 2u) response = 0x5Au;
                else if (index >= 3u)
                {
                    const unsigned limit = command == 0x42u ? (m_sio2Mode[port]==0x41u ? 2u : m_sio2Mode[port]==0x73u ? 6u : 18u) : 6u;
                    response = index-3u < limit ? payload[index-3u] : 0u;
                }
            }
            m_sio2Output.push_back(response);
        }
        // Mode changes take effect after this packet's identifier is emitted.
        if (connected && command == 0x43u && length >= 4u)
            m_sio2Config[port] = m_sio2Input[3] != 0u;
        if (connected && command == 0x44u && length >= 4u)
            m_sio2Mode[port] = selector ? 0x73u : 0x41u;
        if (connected && command == 0x4Fu && length >= 6u)
        {
            const uint32_t mask = m_sio2Input[3] | (m_sio2Input[4] << 8u) | (m_sio2Input[5] << 16u);
            m_sio2Mode[port] = mask == 0x3FFFFu ? 0x79u : mask == 0x3Fu ? 0x73u : 0x41u;
        }
        if (connected && command == 0x4Du)
            for (unsigned i=3u;i<length && i<9u;++i) m_sio2MotorMap[port][i-3u]=m_sio2Input[i];
        uint32_t &status = m_hardware[0x1F80826Cu];
        const uint32_t ports = (status & 0x300u) == 0u ? 0x100u : 0x200u;
        status = (status & ~0x300u) | ports | 0x1000u;
        if (!connected) status |= port == 0u ? 0x1D000u : 0x2D000u;
        m_sio2Input.clear();
        ++m_sio2Command;
    }

    std::optional<IopMemory::DmaStart> IopMemory::takeDmaStart() noexcept
    {
        std::optional<DmaStart> result = m_dmaStart;
        m_dmaStart.reset();
        return result;
    }

    uint32_t IopMemory::allocate(uint32_t size, uint32_t alignment, std::optional<uint32_t> fixed)
    {
        if (size > HeapLimit - HeapBase || alignment == 0u || (alignment & (alignment - 1u)) != 0u || alignment > HeapLimit)
            return 0u;
        size = alignUp(std::max(size, 1u), 16u);
        alignment = std::max<uint32_t>(alignment, 4u);
        uint32_t candidate = 0u;
        if (fixed)
        {
            candidate = *fixed;
            if (candidate < HeapBase || candidate > HeapLimit || size > HeapLimit - candidate)
                return 0u;
            for (const auto &block : m_allocations)
                if (candidate < block.address + block.size && block.address < candidate + size)
                    return 0u;
        }
        else
        {
            // Allocate downwards, leaving low addresses available for module growth.
            auto blocks = m_allocations;
            std::sort(blocks.begin(), blocks.end(), [](const auto &a, const auto &b) { return a.address > b.address; });
            uint32_t upper = HeapLimit;
            for (size_t i = 0; ; ++i)
            {
                const uint32_t lower = i < blocks.size() ? blocks[i].address + blocks[i].size : HeapBase;
                if (upper >= lower && size <= upper - lower)
                {
                    const uint32_t aligned = (upper - size) & ~(alignment - 1u);
                    if (aligned >= lower) { candidate = aligned; break; }
                }
                if (i == blocks.size()) return 0u;
                upper = blocks[i].address;
            }
        }
        m_allocations.push_back({candidate, size});
        markOwned(candidate, size);
        return candidate;
    }

    bool IopMemory::reserveModule(uint32_t address, uint32_t size)
    {
        if (allocate(size, 16u, address) == 0u) return false;
        m_allocations.back().module = true;
        return true;
    }

    void IopMemory::releaseModule(uint32_t address)
    {
        for (auto &block : m_allocations)
            if (block.address == address && block.module) { block.module = false; (void)freeAllocation(address); return; }
    }

    bool IopMemory::freeAllocation(uint32_t address)
    {
        const auto block = std::find_if(m_allocations.begin(), m_allocations.end(),
                                        [&](const Allocation &candidate)
                                        { return candidate.address == address; });
        if (block == m_allocations.end() || block->module)
            return false;
        std::fill(m_owned.begin() + block->address,
                  m_owned.begin() + block->address + block->size,
                  uint8_t{0});
        m_allocations.erase(block);
        return true;
    }

    uint32_t IopMemory::maxFreeMemory() const
    {
        auto blocks = m_allocations;
        std::sort(blocks.begin(), blocks.end(), [](const auto &a, const auto &b) { return a.address < b.address; });
        uint32_t cursor = HeapBase, largest = 0u;
        for (const auto &block : blocks)
        {
            largest = std::max(largest, block.address - cursor);
            cursor = block.address + block.size;
        }
        return std::max(largest, HeapLimit - cursor);
    }

    uint32_t IopMemory::totalFreeMemory() const
    {
        uint32_t total = HeapLimit - HeapBase;
        for (const auto &block : m_allocations) total -= block.size;
        return total;
    }

    std::optional<IopMemory::Allocation> IopMemory::allocationContaining(uint32_t address) const
    {
        const auto block = std::find_if(m_allocations.begin(), m_allocations.end(),
                                        [&](const Allocation &candidate)
                                        {
                                            return address >= candidate.address &&
                                                   address < candidate.address + candidate.size;
                                        });
        if (block == m_allocations.end())
            return std::nullopt;
        return *block;
    }

    std::string IopMemory::readString(uint32_t address, size_t limit) const
    {
        std::string result;
        result.reserve(std::min<size_t>(limit, 64u));
        for (size_t i = 0; i < limit; ++i)
        {
            const char ch = static_cast<char>(read8(address + static_cast<uint32_t>(i)));
            if (ch == '\0')
                break;
            result.push_back(ch);
        }
        return result;
    }
}
