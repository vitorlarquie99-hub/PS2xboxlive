#pragma once

#include <array>
#include <cstdint>

namespace ps2x::iop::detail
{
    struct IopCpuState;
    class IopKernel;
    class IopGuestExecutor;

    class IopVblank
    {
    public:
        explicit IopVblank(IopKernel &kernel) noexcept;

        void reset() noexcept;
        void serviceDue(uint64_t currentCycle, IopGuestExecutor &executor);
        [[nodiscard]] uint64_t nextEventCycle(uint64_t fallback) const noexcept;

        [[nodiscard]] bool dispatchImport(uint16_t ordinal, IopCpuState &cpu, uint64_t currentCycle);

    private:
        struct Handler
        {
            uint32_t function = 0u;
            uint32_t argument = 0u;
            uint32_t gp = 0u;
            int32_t priority = 0;
            uint64_t sequence = 0u;
            unsigned phase = 0u;
            bool used = false;
        };

        // The original service has 16 slots, two occupied by its wait handlers.
        std::array<Handler, 14> m_handlers{};
        std::array<uint64_t, 2> m_nextCycle{UINT64_MAX, UINT64_MAX};
        uint64_t m_sequence = 0u;
        bool m_servicing = false;
        IopKernel &m_kernel;
    };
}
