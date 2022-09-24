#include "mos6502/cpu.hpp"

#include <utility>

#include "mos6502/bus.hpp"
#include "mos6502/regs.hpp"
#include "mos6502/status.hpp"

namespace mos6502
{
class Cpu::Impl {
public:
    Impl(std::shared_ptr<IBus> bus) : m_bus(std::move(bus)), m_regs{}
    {
    }

    Registers& regs()
    {
        return m_regs;
    }

    std::uint8_t step() {
        return 0;
    }

private:
    std::shared_ptr<IBus> m_bus;
    Registers m_regs;
};


Registers& Cpu::regs()
{
    return m_pimpl->regs();
}

std::uint8_t Cpu::step() {
    return m_pimpl->step();
}

}
