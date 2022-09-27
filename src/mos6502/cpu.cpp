#include "mos6502/cpu.hpp"

#include <array>
#include <iomanip>
#include <sstream>
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

        for (std::size_t i{0U}; i < m_dispatch.size(); ++i)
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

        m_dispatch[0xE0] = &Cpu::Impl::cpx;
        m_dispatch[0xE4] = &Cpu::Impl::cpx;
        m_dispatch[0xEC] = &Cpu::Impl::cpx;

        m_dispatch[0xC0] = &Cpu::Impl::cpy;
        m_dispatch[0xC4] = &Cpu::Impl::cpy;
        m_dispatch[0xCC] = &Cpu::Impl::cpy;

        m_dispatch[0xD5] = &Cpu::Impl::cmp;
        m_dispatch[0xD9] = &Cpu::Impl::cmp;
        m_dispatch[0xDD] = &Cpu::Impl::cmp;

        m_dispatch[0xE8] = &Cpu::Impl::inx;
        m_dispatch[0xC8] = &Cpu::Impl::iny;

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
        m_regs.sr &= ~C;
    }

    void cld() {
        m_regs.sr &= ~D;
    }

    void cli() {
        m_regs.sr &= ~I;
    }

    void clv() {
        m_regs.sr &= ~V;
    }

    void sec() {
        m_regs.sr |= C;
    }

    void sed() {
        m_regs.sr |= D;
    }

    void sei() {
        m_regs.sr |= I;
    }

    void adc() {
        // compute with signed values to set overflow flag in native x86
        std::int8_t acc{static_cast<std::int8_t>(m_regs.ac)};
        std::int8_t mem{static_cast<std::int8_t>(read_instruction_input())};
        std::int8_t res{};

        if (static_cast<bool>(m_regs.sr & C)) {
            __asm__ __volatile__("stc");
        } else {
            __asm__ __volatile__("clc");
        }
        __asm__ __volatile__("adcb %%bl, %%al" : "=a" (res) : "a" (acc), "b" (mem));

        std::uint8_t c_out{};
        std::uint8_t n_out{};
        std::uint8_t v_out{};
        std::uint8_t z_out{};
        __asm__ __volatile__("setc %0" : "=g" (c_out));
        __asm__ __volatile__("sets %0" : "=g" (n_out));
        __asm__ __volatile__("seto %0" : "=g" (v_out));
        __asm__ __volatile__("setz %0" : "=g" (z_out));

        m_regs.ac = static_cast<std::uint8_t>(res);
        set_if(c_out, C);
        set_if(n_out, N);
        set_if(v_out, V);
        set_if(z_out, Z);
    }

    void sbc() {
        // compute with signed values to set overflow flag in native x86
        std::int8_t acc{static_cast<std::int8_t>(m_regs.ac)};
        std::int8_t mem{static_cast<std::int8_t>(read_instruction_input())};
        std::int8_t res{};

        // Borrow when carry unset
        if (static_cast<bool>(m_regs.sr & C)) {
            __asm__ __volatile__("clc");
        } else {
            __asm__ __volatile__("stc");
        }
        __asm__ __volatile__("sbbb %%bl, %%al" : "=a" (res) : "a" (acc), "b" (mem));

        std::uint8_t c_out{};
        std::uint8_t n_out{};
        std::uint8_t v_out{};
        std::uint8_t z_out{};

        __asm__ __volatile__("setnc %0" : "=g" (c_out)); // C = ~B
        __asm__ __volatile__("sets %0" : "=g" (n_out));
        __asm__ __volatile__("seto %0" : "=g" (v_out));
        __asm__ __volatile__("setz %0" : "=g" (z_out));

        m_regs.ac = static_cast<std::uint8_t>(res);
        set_if(c_out, C);
        set_if(n_out, N);
        set_if(v_out, V);
        set_if(z_out, Z);
    }

    void amd() {
        std::uint8_t acc{m_regs.ac};
        std::uint8_t mem{read_instruction_input()};
        std::uint8_t res{};

        __asm__ __volatile__("andb %%bl, %%al" : "=a" (res) : "a" (acc), "b" (mem));

        std::uint8_t n_out{};
        std::uint8_t z_out{};
        __asm__ __volatile__("sets %0" : "=g" (n_out));
        __asm__ __volatile__("setz %0" : "=g" (z_out));

        m_regs.ac = res;
        set_if(n_out, N);
        set_if(z_out, Z);
    }

    void eor() {
        std::uint8_t acc{m_regs.ac};
        std::uint8_t mem{read_instruction_input()};
        std::uint8_t res{};

        __asm__ __volatile__("xorb %%bl, %%al" : "=a" (res) : "a" (acc), "b" (mem));

        std::uint8_t n_out{};
        std::uint8_t z_out{};
        __asm__ __volatile__("sets %0" : "=g" (n_out));
        __asm__ __volatile__("setz %0" : "=g" (z_out));

        m_regs.ac = res;
        set_if(n_out, N);
        set_if(z_out, Z);
    }

    void ora() {
        std::uint8_t acc{m_regs.ac};
        std::uint8_t mem{read_instruction_input()};
        std::uint8_t res{};

        __asm__ __volatile__("orb %%bl, %%al" : "=a" (res) : "a" (acc), "b" (mem));

        std::uint8_t n_out{};
        std::uint8_t z_out{};
        __asm__ __volatile__("sets %0" : "=g" (n_out));
        __asm__ __volatile__("setz %0" : "=g" (z_out));

        m_regs.ac = res;
        set_if(n_out, N);
        set_if(z_out, Z);
    }

    void cmp() {
        std::uint8_t acc{m_regs.ac};
        std::uint8_t mem{read_instruction_input()};

        __asm__ __volatile__("cmpb %%bl, %%al" : : "a" (acc), "b" (mem));

        std::uint8_t c_out{};
        std::uint8_t n_out{};
        std::uint8_t z_out{};
        __asm__ __volatile__("setnc %0" : "=g" (c_out));
        __asm__ __volatile__("sets %0" : "=g" (n_out));
        __asm__ __volatile__("setz %0" : "=g" (z_out));

        set_if(c_out, C);
        set_if(n_out, N);
        set_if(z_out, Z);
    }

    void cpx() {
        std::uint8_t idx{m_regs.xi};
        std::uint8_t mem{read_instruction_input()};

        __asm__ __volatile__("cmpb %%bl, %%al" : : "a" (idx), "b" (mem));

        std::uint8_t c_out{};
        std::uint8_t n_out{};
        std::uint8_t z_out{};
        __asm__ __volatile__("setnc %0" : "=g" (c_out));
        __asm__ __volatile__("sets %0" : "=g" (n_out));
        __asm__ __volatile__("setz %0" : "=g" (z_out));

        set_if(c_out, C);
        set_if(n_out, N);
        set_if(z_out, Z);
    }

    void cpy() {
        std::uint8_t idy{m_regs.yi};
        std::uint8_t mem{read_instruction_input()};

        __asm__ __volatile__("cmpb %%bl, %%al" : : "a" (idy), "b" (mem));

        std::uint8_t c_out{};
        std::uint8_t n_out{};
        std::uint8_t z_out{};
        __asm__ __volatile__("setnc %0" : "=g" (c_out));
        __asm__ __volatile__("sets %0" : "=g" (n_out));
        __asm__ __volatile__("setz %0" : "=g" (z_out));

        set_if(c_out, C);
        set_if(n_out, N);
        set_if(z_out, Z);
    }

    void inx() {
        m_regs.xi += 1U;
        set_if(m_regs.xi >= 0x80, N);
        set_if(m_regs.xi == 0x00, Z);
    }

    void iny() {
        m_regs.yi += 1U;
        set_if(m_regs.yi >= 0x80, N);
        set_if(m_regs.yi == 0x00, Z);
    }

    void lda() {
        m_regs.ac = read_instruction_input();
        set_if(m_regs.ac >= 128U, N);
        set_if(m_regs.ac == 0U,   Z);
    }

    void ldx() {
        m_regs.xi = read_instruction_input();
        set_if(m_regs.xi >= 128U, N);
        set_if(m_regs.xi == 0U,   Z);
    }

    void ldy() {
        m_regs.yi = read_instruction_input();
        set_if(m_regs.yi >= 128U, N);
        set_if(m_regs.yi == 0U,   Z);
    }

    inline void set_if(bool cond, std::uint8_t status) {
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
        default: goto unsupported;
        }

        group_one:
        switch (m_instruction.parts.addr)
        {
        case 0: goto indirect_x;
        case 1: goto zero_page;
        case 2: goto immediate;
        case 3: goto absolute;
        case 4: goto indirect_y;
        case 5: goto zero_page_x;
        case 6: goto absolute_y;
        case 7: goto absolute_x;
        default: goto unsupported;
        }

        group_two:
        switch (m_instruction.parts.addr)
        {
        case 0: goto immediate;
        case 1: goto zero_page;
        case 2: goto unsupported;;
        case 3: goto absolute;
        case 4: goto unsupported;
        case 5:
            if (m_instruction.parts.oper == 4 ||
                m_instruction.parts.oper == 5) {
                goto zero_page_y;
            } else {
                goto zero_page_x;
            }
        case 6: goto unsupported;
        case 7:
            if (m_instruction.parts.oper == 5) {
                goto absolute_y;
            } else {
                goto absolute_x;
            }
        default: goto unsupported;
        }

        group_three:
        switch (m_instruction.parts.addr)
        {
        case 0: goto immediate;
        case 1: goto zero_page;
        case 2: goto unsupported;
        case 3: goto absolute;
        case 4: goto unsupported;
        case 5: goto zero_page_x;
        case 6: goto unsupported;
        case 7: goto absolute_x;
        default: goto unsupported;
        }

        immediate:
        return m_immediate8;

        absolute:
        return m_bus->read(m_immediate16);

        absolute_x:
        return m_bus->read(m_immediate16 + m_regs.xi);

        absolute_y:
        return m_bus->read(m_immediate16 + m_regs.yi);

        zero_page:
        return m_bus->read(m_immediate8);

        zero_page_x:
        return m_bus->read((m_immediate8 + m_regs.xi) & 0xFF);

        zero_page_y:
        return m_bus->read((m_immediate8 + m_regs.yi) & 0xFF);

        indirect_x:
        {
            std::uint8_t lo = m_bus->read((m_immediate8 + m_regs.xi)      & 0xFF);
            std::uint8_t hi = m_bus->read((m_immediate8 + m_regs.xi + 1U) & 0xFF);
            std::uint16_t addr = ((hi << 8) | lo) & 0xFFFF;
            return m_bus->read(addr);
        }

        indirect_y:
        {
            std::uint8_t lo = m_bus->read(m_immediate8);
            std::uint8_t hi = m_bus->read((m_immediate8 + 1U) & 0xFF);
            std::uint16_t addr = ((hi << 8) | lo) & 0xFFFF;
            return m_bus->read((addr + m_regs.yi) & 0xFFFF);
        }

        unsupported:
        std::stringstream ss{};
        ss << "illegal address mode: opcode="
           << std::hex << m_instruction.opcode
           << " (" << m_instruction.parts.group << ":" << m_instruction.parts.addr << ":" << m_instruction.parts.oper << ")";
        throw std::runtime_error(ss.str());
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
