#include "iop_vblank.h"

#include "../core/iop_cpu.h"
#include "../iop_emulator_const.h"
#include "../core/iop_kernel.h"
#include "../services/iop_rpc.h"

#include <algorithm>

namespace ps2x::iop::detail
{
    IopVblank::IopVblank(IopKernel &kernel) noexcept
        : m_kernel(kernel)
    {
    }

    void IopVblank::reset() noexcept
    {
        m_handlers = {};
        m_nextCycle = {UINT64_MAX, UINT64_MAX};
        m_sequence = 0u;
        m_servicing = false;
    }

    bool IopVblank::dispatchImport(uint16_t ordinal, IopCpuState &cpu, uint64_t currentCycle)
    {
        switch (ordinal)
        {
        case 4: // WaitVblankStart
        case 5: // WaitVblankEnd
        case 6: // WaitVblank
        case 7: // WaitNonVblank
        {
            const bool waitForEnd = ordinal == 5u || ordinal == 7u;
            const uint64_t phase = waitForEnd ? kVblankEndPhaseCycles : 0u;
            const uint64_t fieldStart = currentCycle - (currentCycle % kVblankPeriodCycles);
            uint64_t wakeCycle = fieldStart + phase;
            if (wakeCycle <= currentCycle)
                wakeCycle += kVblankPeriodCycles;
            m_kernel.delayCurrentUntil(wakeCycle, cpu);
            cpu.gpr[2] = 0u;
            return true;
        }
        case 8: // RegisterVblankHandler(startend, priority, handler, argument)
        case 9: // ReleaseVblankHandler(startend, handler)
        {
            if (m_servicing)
            {
                cpu.gpr[2] = static_cast<uint32_t>(-100); // KE_ILLEGAL_CONTEXT
                return true;
            }
            const unsigned phase = cpu.gpr[4] != 0u ? 1u : 0u;
            const uint32_t function = cpu.gpr[ordinal == 8u ? 6u : 5u];
            const auto existing = std::find_if(m_handlers.begin(), m_handlers.end(), [&](const Handler &handler)
            {
                return handler.used && handler.phase == phase && handler.function == function;
            });
            if (ordinal == 9u)
            {
                cpu.gpr[2] = existing == m_handlers.end() ? static_cast<uint32_t>(-105) : 0u;
                if (existing != m_handlers.end())
                    existing->used = false;
                if (std::none_of(m_handlers.begin(), m_handlers.end(), [phase](const Handler &handler)
                    { return handler.used && handler.phase == phase; }))
                    m_nextCycle[phase] = UINT64_MAX;
                return true;
            }
            const auto free = std::find_if(m_handlers.begin(), m_handlers.end(), [](const Handler &handler)
            {
                return !handler.used;
            });
            if (free == m_handlers.end())
                cpu.gpr[2] = static_cast<uint32_t>(-400); // KE_NO_MEMORY
            else if (existing != m_handlers.end())
                cpu.gpr[2] = static_cast<uint32_t>(-104); // KE_FOUND_HANDLER
            else
            {
                *free = {function, cpu.gpr[7], cpu.gpr[28], static_cast<int32_t>(cpu.gpr[5]),
                         m_sequence++, phase, true};
                if (m_nextCycle[phase] == UINT64_MAX)
                {
                    uint64_t next = currentCycle - currentCycle % kVblankPeriodCycles +
                                    (phase == 0u ? 0u : kVblankEndPhaseCycles);
                    if (next <= currentCycle)
                        next += kVblankPeriodCycles;
                    m_nextCycle[phase] = next;
                }
                cpu.gpr[2] = 0u;
            }
            return true;
        }
        default:
            return false;
        }
    }

    void IopVblank::serviceDue(uint64_t currentCycle, IopGuestExecutor &executor)
    {
        if (m_servicing)
            return;
        m_servicing = true;
        struct ServiceGuard
        {
            bool &flag;
            ~ServiceGuard() { flag = false; }
        } guard{m_servicing};

        // Process phases in deadline order, including when an idle scheduler
        // jumps over both edges. Missed fields are coalesced, not replayed.
        std::array<unsigned, 2> phases{0u, 1u};
        if (m_nextCycle[1] < m_nextCycle[0])
            std::swap(phases[0], phases[1]);
        for (const unsigned phase : phases)
        {
            if (m_nextCycle[phase] > currentCycle)
                continue;
            std::array<Handler *, 14> ordered{};
            size_t count = 0u;
            for (Handler &handler : m_handlers)
                if (handler.used && handler.phase == phase)
                    ordered[count++] = &handler;
            std::sort(ordered.begin(), ordered.begin() + count, [](const Handler *left, const Handler *right)
            {
                return left->priority != right->priority ? left->priority < right->priority
                                                        : left->sequence < right->sequence;
            });
            for (size_t index = 0; index < count; ++index)
            {
                Handler &handler = *ordered[index];
                const uint32_t result = handler.function != 0u
                    ? executor.executeGuestFunctionWithBudget(handler.function, handler.argument, 0u, 0u, 0u,
                                                              handler.gp, 100000u)
                    : 0u;
                if (result == 0u)
                    handler.used = false;
            }
            m_nextCycle[phase] = UINT64_MAX;
            if (std::any_of(m_handlers.begin(), m_handlers.end(), [phase](const Handler &handler)
                { return handler.used && handler.phase == phase; }))
            {
                uint64_t next = currentCycle - currentCycle % kVblankPeriodCycles +
                                (phase == 0u ? 0u : kVblankEndPhaseCycles);
                if (next <= currentCycle)
                    next += kVblankPeriodCycles;
                m_nextCycle[phase] = next;
            }
        }
    }

    uint64_t IopVblank::nextEventCycle(uint64_t fallback) const noexcept
    {
        return std::min({fallback, m_nextCycle[0], m_nextCycle[1]});
    }

}
