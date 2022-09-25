#include "mos6502/cpu.hpp"

#include <array>
#include <utility>
#include <x86intrin.h>

#include "mos6502/bus.hpp"
#include "mos6502/regs.hpp"
#include "mos6502/status.hpp"

// @see https://llx.com/Neil/a2/opcodes.html
union Instruction
{
    struct {
        std::uint8_t group: 2;
        std::uint8_t addr:  3;
        std::uint8_t oper:  3;
    } parts;
    std::uint8_t opcode;
};

static_assert(sizeof(Instruction) == 1);

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
    Impl(std::shared_ptr<IBus> bus) : m_bus(std::move(bus))
    {
        static_cast<void>(m_padding);

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

        m_dispatch[0x61] = &Cpu::Impl::adc;
        m_dispatch[0x65] = &Cpu::Impl::adc;
        m_dispatch[0x69] = &Cpu::Impl::adc;
        m_dispatch[0x6D] = &Cpu::Impl::adc;
        m_dispatch[0x71] = &Cpu::Impl::adc;
        m_dispatch[0x75] = &Cpu::Impl::adc;
        m_dispatch[0x79] = &Cpu::Impl::adc;
        m_dispatch[0x7D] = &Cpu::Impl::adc;

        m_dispatch[0xE1] = &Cpu::Impl::sbc;
        m_dispatch[0xE5] = &Cpu::Impl::sbc;
        m_dispatch[0xE9] = &Cpu::Impl::sbc;
        m_dispatch[0xED] = &Cpu::Impl::sbc;
        m_dispatch[0xF1] = &Cpu::Impl::sbc;
        m_dispatch[0xF5] = &Cpu::Impl::sbc;
        m_dispatch[0xF9] = &Cpu::Impl::sbc;
        m_dispatch[0xFD] = &Cpu::Impl::sbc;

        m_dispatch[0x21] = &Cpu::Impl::amd;
        m_dispatch[0x25] = &Cpu::Impl::amd;
        m_dispatch[0x29] = &Cpu::Impl::amd;
        m_dispatch[0x2D] = &Cpu::Impl::amd;
        m_dispatch[0x31] = &Cpu::Impl::amd;
        m_dispatch[0x35] = &Cpu::Impl::amd;
        m_dispatch[0x39] = &Cpu::Impl::amd;
        m_dispatch[0x3D] = &Cpu::Impl::amd;

        m_dispatch[0x01] = &Cpu::Impl::ora;
        m_dispatch[0x05] = &Cpu::Impl::ora;
        m_dispatch[0x09] = &Cpu::Impl::ora;
        m_dispatch[0x0D] = &Cpu::Impl::ora;
        m_dispatch[0x11] = &Cpu::Impl::ora;
        m_dispatch[0x15] = &Cpu::Impl::ora;
        m_dispatch[0x19] = &Cpu::Impl::ora;
        m_dispatch[0x1D] = &Cpu::Impl::ora;

        m_dispatch[0x41] = &Cpu::Impl::eor;
        m_dispatch[0x45] = &Cpu::Impl::eor;
        m_dispatch[0x49] = &Cpu::Impl::eor;
        m_dispatch[0x4D] = &Cpu::Impl::eor;
        m_dispatch[0x51] = &Cpu::Impl::eor;
        m_dispatch[0x55] = &Cpu::Impl::eor;
        m_dispatch[0x59] = &Cpu::Impl::eor;
        m_dispatch[0x5D] = &Cpu::Impl::eor;

        m_dispatch[0xC1] = &Cpu::Impl::cmp;
        m_dispatch[0xC5] = &Cpu::Impl::cmp;
        m_dispatch[0xC9] = &Cpu::Impl::cmp;
        m_dispatch[0xCD] = &Cpu::Impl::cmp;
        m_dispatch[0xD1] = &Cpu::Impl::cmp;
        m_dispatch[0xD5] = &Cpu::Impl::cmp;
        m_dispatch[0xD9] = &Cpu::Impl::cmp;
        m_dispatch[0xDD] = &Cpu::Impl::cmp;

        m_dispatch[0xA1] = &Cpu::Impl::lda;
        m_dispatch[0xA5] = &Cpu::Impl::lda;
        m_dispatch[0xA9] = &Cpu::Impl::lda;
        m_dispatch[0xAD] = &Cpu::Impl::lda;
        m_dispatch[0xB1] = &Cpu::Impl::lda;
        m_dispatch[0xB5] = &Cpu::Impl::lda;
        m_dispatch[0xB9] = &Cpu::Impl::lda;
        m_dispatch[0xBD] = &Cpu::Impl::lda;

        m_dispatch[0xA2] = &Cpu::Impl::ldx;
        m_dispatch[0xA6] = &Cpu::Impl::ldx;
        m_dispatch[0xB6] = &Cpu::Impl::ldx;
        m_dispatch[0xAE] = &Cpu::Impl::ldx;
        m_dispatch[0xBE] = &Cpu::Impl::ldx;

        m_dispatch[0xA0] = &Cpu::Impl::ldy;
        m_dispatch[0xA4] = &Cpu::Impl::ldy;
        m_dispatch[0xAC] = &Cpu::Impl::ldy;
        m_dispatch[0xB4] = &Cpu::Impl::ldy;
        m_dispatch[0xBC] = &Cpu::Impl::ldy;
    }

    Registers& regs()
    {
        return m_regs;
    }

    std::uint8_t step() {
        m_instruction.opcode = m_bus->read(m_regs.pc);
        m_immediate8 = m_bus->read(m_regs.pc + 1U);
        m_immediate16 = static_cast<std::uint16_t>(m_bus->read(m_regs.pc + 2U) << 8) + m_immediate8;
        m_extra_cycles = 0U;

        std::uint8_t length{InstructionLength[m_instruction.opcode]};
        std::uint8_t cycles{InstructionCycles[m_instruction.opcode]};

        (*this.*m_dispatch[m_instruction.opcode])();

        m_regs.pc += length;
        return cycles + m_extra_cycles;
    }

private:
    std::shared_ptr<IBus> m_bus;

    Registers m_regs{};

    Instruction m_instruction{};

    std::uint8_t m_immediate8{};

    std::uint16_t m_immediate16{};

    std::uint8_t m_extra_cycles{};

    std::array<std::uint8_t, 3> const m_padding{};

    std::array<void (Cpu::Impl::*)(), 256> m_dispatch{};

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

    void adc() {
        // compute with signed values to set overflow flag in native x86
        std::int8_t acc{static_cast<std::int8_t>(m_regs.ac)};
        std::int8_t mem{static_cast<std::int8_t>(read_instruction_input())};
        std::int8_t res{};

        if (static_cast<bool>(m_regs.sr & Status::C)) {
            __asm__ volatile("stc");
        } else {
            __asm__ volatile("clc");
        }
        __asm__ volatile("adcb %%bl, %%al" : "=a" (res) : "a" (acc), "b" (mem));

        std::uint8_t c_out{};
        std::uint8_t n_out{};
        std::uint8_t v_out{};
        std::uint8_t z_out{};
        __asm__ volatile("setc %0" : "=g" (c_out));
        __asm__ volatile("sets %0" : "=g" (n_out));
        __asm__ volatile("seto %0" : "=g" (v_out));
        __asm__ volatile("setz %0" : "=g" (z_out));

        m_regs.ac = static_cast<std::uint8_t>(res);
        set_if(c_out, Status::C);
        set_if(n_out, Status::N);
        set_if(v_out, Status::V);
        set_if(z_out, Status::Z);
    }

    void sbc() {
        std::uint8_t acc{};
        std::uint8_t mem{};

        std::uint8_t carry_in{static_cast<bool>(m_regs.sr & Status::C) ? std::uint8_t{1} : std::uint8_t{}};
        std::uint8_t carry_out{};

        std::uint8_t res = __builtin_subcb(acc, mem, carry_in, &carry_out);

        set_if(static_cast<bool>(res & 0x80), Status::N);
        set_if(res == std::uint8_t{},         Status::Z);
        set_if(static_cast<bool>(carry_out),  Status::C);
    }

    void amd() {

    }

    void eor() {

    }

    void ora() {

    }

    void cmp() {

    }

    void lda() {
        m_regs.ac = read_instruction_input();
        set_if(m_regs.ac >= 128U, Status::N);
        set_if(m_regs.ac == 0U,   Status::Z);
    }

    void ldx() {
        m_regs.xi = read_instruction_input();
        set_if(m_regs.xi >= 128U, Status::N);
        set_if(m_regs.xi == 0U,   Status::Z);
    }

    void ldy() {
        m_regs.yi = read_instruction_input();
        set_if(m_regs.yi >= 128U, Status::N);
        set_if(m_regs.yi == 0U,   Status::Z);
    }

    inline void set_if(bool cond, Status status) {
        if (cond) {
            m_regs.sr |= status;
        } else {
            m_regs.sr &= ~status;
        }
    }

    std::uint8_t read_instruction_input() {
        // Declutter switch inside switch using goto
        switch (m_instruction.parts.group)
        {
        case 1: goto group_one;
        case 2: goto group_two;
        case 0: goto group_three;
        default: throw std::runtime_error("illegal instruction group");
        }

        group_one:
        switch (m_instruction.parts.addr)
        {
        case 0:
        {
            std::uint8_t lo = m_bus->read((m_immediate8 + m_regs.xi)      & 0xFF);
            std::uint8_t hi = m_bus->read((m_immediate8 + m_regs.xi + 1U) & 0xFF);
            std::uint16_t addr = ((hi << 8) | lo) & 0xFFFF;
            return m_bus->read(addr);
        }
        case 1: return m_bus->read(m_immediate8);
        case 2: return m_immediate8;
        case 3: return m_bus->read(m_immediate16);
        case 4:
        {
            std::uint8_t lo = m_bus->read(m_immediate8);
            std::uint8_t hi = m_bus->read((m_immediate8 + 1U) & 0xFF);
            std::uint16_t addr = ((hi << 8) | lo) & 0xFFFF;
            return m_bus->read((addr + m_regs.yi) & 0xFFFF);
        }
        case 5: return m_bus->read((m_immediate8 + m_regs.xi) & 0xFF);
        case 6: return m_bus->read(m_immediate16 + m_regs.yi);
        case 7: return m_bus->read(m_immediate16 + m_regs.xi);
        default: throw std::runtime_error("illegal address mode");
        }

        group_two:
        switch (m_instruction.parts.addr)
        {
        case 0: return m_immediate8;
        case 1: return m_bus->read(m_immediate8);
        case 2: throw std::runtime_error("not implemented yet");
        case 3: return m_bus->read(m_immediate16);
        case 4: throw std::runtime_error("not implemented yet");
        case 5:
        {
            if (m_instruction.parts.oper == 4 || m_instruction.parts.oper == 5) {
                return m_bus->read((m_immediate8 + m_regs.yi) & 0xFF);
            } else {
                return m_bus->read((m_immediate8 + m_regs.xi) & 0xFF);
            }
        }
        case 6: throw std::runtime_error("not implemented yet");
        case 7:
        {
            if (m_instruction.parts.oper == 5) {
                return m_bus->read(m_immediate16 + m_regs.yi);
            } else {
                return m_bus->read(m_immediate16 + m_regs.xi);
            }
        }
        default: throw std::runtime_error("illegal address mode");
        }

        group_three:
        switch (m_instruction.parts.addr)
        {
        case 0: return m_immediate8;
        case 1: return m_bus->read(m_immediate8);
        case 2: throw std::runtime_error("not implemented yet");
        case 3: return m_bus->read(m_immediate16);
        case 4: throw std::runtime_error("not implemented yet");
        case 5: return m_bus->read((m_immediate8 + m_regs.xi) & 0xFF);
        case 6: throw std::runtime_error("not implemented yet");
        case 7: return m_bus->read(m_immediate16 + m_regs.xi);
        default: throw std::runtime_error("illegal address mode");
        }
    }
};

Cpu::Cpu(std::shared_ptr<IBus> bus) : m_pimpl{std::make_unique<Cpu::Impl>(std::move(bus))} {

}

Cpu::~Cpu() = default;

Registers& Cpu::regs()
{
    return m_pimpl->regs();
}

std::uint8_t Cpu::step() {
    return m_pimpl->step();
}

}
