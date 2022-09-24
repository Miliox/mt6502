#include "mos6502/cpu.hpp"

#include <array>
#include <utility>

#include "mos6502/bus.hpp"
#include "mos6502/regs.hpp"
#include "mos6502/status.hpp"

// @see https://llx.com/Neil/a2/opcodes.html
struct Instruction
{
    std::uint8_t group: 2;
    std::uint8_t addr:  3;
    std::uint8_t oper:  3;
};

/// Lookup Table for Instruction Length
static std::array<std::uint8_t, 256> InstructionLength = {
//  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F  // (Low/High) Nibble
    1, 2, 0, 0, 0, 2, 2, 0, 1, 2, 1, 0, 0, 3, 3, 0, // 0
    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0, // 1
    3, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0, // 2
    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0, // 3
    1, 2, 0, 0, 0, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0, // 4
    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0, // 5
    1, 2, 0, 0, 0, 2, 2, 0, 1, 2, 0, 0, 3, 3, 3, 0, // 6
    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0, // 7
    0, 2, 0, 0, 2, 2, 2, 0, 1, 0, 1, 0, 3, 3, 3, 0, // 8
    2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 0, 3, 0, 0, // 9
    2, 2, 2, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0, // A
    2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0, // B
    2, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0, // C
    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0, // D
    2, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0, // E
    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0, // F
};

/// Lookup Table for Number of Cycles of an Instruction
static std::array<std::uint8_t, 256> InstructionCycles = {
//  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F  // (Low/High) Nibble
    7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0, // 0
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // 1
    6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0, // 2
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // 3
    6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0, // 4
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // 5
    6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0, // 6
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // 7
    0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0, // 8
    2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0, // 9
    2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0, // A
    2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0, // B
    2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0, // C
    2, 5, 0, 0, 4, 6, 0, 0, 2, 4, 0, 0, 0, 4, 7, 0, // D
    2, 2, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0, // E
    2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // F
};

namespace mos6502
{
class Cpu::Impl {
public:
    Impl(std::shared_ptr<IBus> bus) : m_bus(std::move(bus)), m_dispatch{}, m_regs{}
    {
        for (std::size_t i{0U}; i < 256; ++i)
        {
            m_dispatch[i] = &Cpu::Impl::illegal;
        }

        m_dispatch[0x18] = &Cpu::Impl::clc;
        m_dispatch[0xD8] = &Cpu::Impl::cld;
        m_dispatch[0x58] = &Cpu::Impl::cli;
        m_dispatch[0xB8] = &Cpu::Impl::clv;
        m_dispatch[0x38] = &Cpu::Impl::sec;
        m_dispatch[0xF8] = &Cpu::Impl::sed;
        m_dispatch[0x78] = &Cpu::Impl::sei;
    }

    Registers& regs()
    {
        return m_regs;
    }

    std::uint8_t step() {
        std::uint8_t const opcode{m_bus->read(m_regs.pc)};

        (*this.*m_dispatch[opcode])();

        m_regs.pc += InstructionLength[opcode];
        return InstructionCycles[opcode];
    }

private:
    std::shared_ptr<IBus> m_bus;
    std::array<void (Cpu::Impl::*)(), 256> m_dispatch;
    Registers m_regs;

    [[ noreturn ]] void illegal() {
        throw std::runtime_error("Illegal instruction hit!");
    }

    void clc() {
        m_regs.sr &= ~Status::C;
    }

    void cld() {
        m_regs.sr &= ~Status::D;
    }

    void cli() {
        m_regs.sr &= ~Status::I;
    }

    void clv() {
        m_regs.sr &= ~Status::V;
    }

    void sec() {
        m_regs.sr |= Status::C;
    }

    void sed() {
        m_regs.sr |= Status::D;
    }

    void sei() {
        m_regs.sr |= Status::I;
    }
};


Registers& Cpu::regs()
{
    return m_pimpl->regs();
}

std::uint8_t Cpu::step() {
    return m_pimpl->step();
}

}
