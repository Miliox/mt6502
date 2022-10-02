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
/// @note BRK (00) instruction length includes mark byte
static std::array<std::uint8_t, 256> InstructionLength = {
//  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F  // (Low/High) Nibble
    2, 2, 0, 0, 0, 2, 2, 0, 1, 2, 1, 0, 0, 3, 3, 0, // 0
    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0, // 1
    3, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0, // 2
    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0, // 3
    1, 2, 0, 0, 0, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0, // 4
    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0, // 5
    1, 2, 0, 0, 0, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0, // 6
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
    2, 5, 0, 0, 4, 6, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // D
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

        m_regs.sp = 0x1FF;
        m_regs.sr = U | B;

        for (std::size_t i{0U}; i < m_dispatch.size(); ++i)
        {
            m_dispatch[i] = &Cpu::Impl::illegal;
        }

        m_dispatch[0xEA] = &Cpu::Impl::nop;

        m_dispatch[0x00] = &Cpu::Impl::brk;
        m_dispatch[0x40] = &Cpu::Impl::rti;

        m_dispatch[0x20] = &Cpu::Impl::jsr;
        m_dispatch[0x60] = &Cpu::Impl::rts;

        m_dispatch[0x4C] = &Cpu::Impl::jmp_abs;
        m_dispatch[0x6C] = &Cpu::Impl::jmp_ind;

        m_dispatch[0x90] = &Cpu::Impl::bcc;
        m_dispatch[0xB0] = &Cpu::Impl::bcs;
        m_dispatch[0xF0] = &Cpu::Impl::beq;
        m_dispatch[0x30] = &Cpu::Impl::bmi;
        m_dispatch[0xD0] = &Cpu::Impl::bne;
        m_dispatch[0x10] = &Cpu::Impl::bpl;
        m_dispatch[0x50] = &Cpu::Impl::bvc;
        m_dispatch[0x70] = &Cpu::Impl::bvs;

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

        m_dispatch[0x24] = &Cpu::Impl::bit;
        m_dispatch[0x2C] = &Cpu::Impl::bit;

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

        m_dispatch[0xC6] = &Cpu::Impl::dec;
        m_dispatch[0xCE] = &Cpu::Impl::dec;
        m_dispatch[0xD6] = &Cpu::Impl::dec;
        m_dispatch[0xDE] = &Cpu::Impl::dec;

        m_dispatch[0xCA] = &Cpu::Impl::dex;
        m_dispatch[0x88] = &Cpu::Impl::dey;

        m_dispatch[0xE6] = &Cpu::Impl::inc;
        m_dispatch[0xEE] = &Cpu::Impl::inc;
        m_dispatch[0xF6] = &Cpu::Impl::inc;
        m_dispatch[0xFE] = &Cpu::Impl::inc;

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

        m_dispatch[0x81] = &Cpu::Impl::sta;
        m_dispatch[0x85] = &Cpu::Impl::sta;
        m_dispatch[0x8D] = &Cpu::Impl::sta;
        m_dispatch[0x91] = &Cpu::Impl::sta;
        m_dispatch[0x95] = &Cpu::Impl::sta;
        m_dispatch[0x99] = &Cpu::Impl::sta;
        m_dispatch[0x9D] = &Cpu::Impl::sta;

        m_dispatch[0x86] = &Cpu::Impl::stx;
        m_dispatch[0x8E] = &Cpu::Impl::stx;
        m_dispatch[0x96] = &Cpu::Impl::stx;

        m_dispatch[0x84] = &Cpu::Impl::sty;
        m_dispatch[0x8C] = &Cpu::Impl::sty;
        m_dispatch[0x94] = &Cpu::Impl::sty;

        m_dispatch[0xAA] = &Cpu::Impl::tax;
        m_dispatch[0xA8] = &Cpu::Impl::tay;
        m_dispatch[0xBA] = &Cpu::Impl::tsx;
        m_dispatch[0x8A] = &Cpu::Impl::txa;
        m_dispatch[0x9A] = &Cpu::Impl::txs;
        m_dispatch[0x98] = &Cpu::Impl::tya;

        m_dispatch[0x06] = &Cpu::Impl::asl;
        m_dispatch[0x0A] = &Cpu::Impl::asl;
        m_dispatch[0x0E] = &Cpu::Impl::asl;
        m_dispatch[0x16] = &Cpu::Impl::asl;
        m_dispatch[0x1E] = &Cpu::Impl::asl;

        m_dispatch[0x46] = &Cpu::Impl::lsr;
        m_dispatch[0x4A] = &Cpu::Impl::lsr;
        m_dispatch[0x4E] = &Cpu::Impl::lsr;
        m_dispatch[0x56] = &Cpu::Impl::lsr;
        m_dispatch[0x5E] = &Cpu::Impl::lsr;

        m_dispatch[0x26] = &Cpu::Impl::rol;
        m_dispatch[0x2A] = &Cpu::Impl::rol;
        m_dispatch[0x2E] = &Cpu::Impl::rol;
        m_dispatch[0x36] = &Cpu::Impl::rol;
        m_dispatch[0x3E] = &Cpu::Impl::rol;

        m_dispatch[0x66] = &Cpu::Impl::ror;
        m_dispatch[0x6A] = &Cpu::Impl::ror;
        m_dispatch[0x6E] = &Cpu::Impl::ror;
        m_dispatch[0x76] = &Cpu::Impl::ror;
        m_dispatch[0x7E] = &Cpu::Impl::ror;

        m_dispatch[0x48] = &Cpu::Impl::pha;
        m_dispatch[0x08] = &Cpu::Impl::php;
        m_dispatch[0x68] = &Cpu::Impl::pla;
        m_dispatch[0x28] = &Cpu::Impl::plp;
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
        m_regs.pc += length;

        (*this.*m_dispatch[m_instruction.opcode])();
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
        std::stringstream ss{};
        ss << "Illegal instruction: " << std::uppercase << std::hex << std::setw(2) << static_cast<std::int64_t>(m_instruction.opcode);
        throw std::runtime_error(ss.str());
    }

    void brk() {
        std::uint8_t const pc_lo = (m_regs.pc >> 0) & 0xFF;
        std::uint8_t const pc_hi = (m_regs.pc >> 8) & 0xFF;

        std::uint8_t const int_lo = m_bus->read(0xFFFE);
        std::uint8_t const int_hi = m_bus->read(0xFFFF);

        // Save current context
        push(pc_hi);
        push(pc_lo);
        push(m_regs.sr);

        // Interrupt handler
        m_regs.pc = ((int_hi << 8) | int_lo) & 0xFFFF;

        // Disable Interrupts
        m_regs.sr |= I;
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

    void nop() {
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

        if (static_cast<bool>(m_regs.sr & D))
        {
            std::uint8_t adjustment{};
            if ((m_regs.ac & 0xF) > 0x9)
            {
                adjustment += 0x6;
            }
            if (m_regs.ac > 0x99 || c_out)
            {
                adjustment += 0x60;
                c_out = 1U;
            }
            m_regs.ac += adjustment;
            z_out = (m_regs.ac == 0);
            n_out = (m_regs.ac & 0x80);
        }

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

    void bit() {
        std::uint8_t acc{m_regs.ac};
        std::uint8_t mem{read_instruction_input()};

        std::uint8_t res{};
        std::uint8_t z_out{};

        __asm__ __volatile__("andb %%bl, %%al" : "=a" (res) : "a" (acc), "b" (mem));
        __asm__ __volatile__("setz %0" : "=g" (z_out));

        m_regs.sr = (m_regs.sr & 0x3D) | (mem & 0xC0) | ((z_out << 1) & 0x02);
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

    void dec() {
        std::uint8_t mem{read_instruction_input()};

        mem -= 1U;
        set_if(mem >= 0x80, N);
        set_if(mem == 0x00, Z);

        write_instruction_output(mem);
    }

    void dex() {
        m_regs.xi -= 1U;
        set_if(m_regs.xi >= 0x80, N);
        set_if(m_regs.xi == 0x00, Z);
    }

    void dey() {
        m_regs.yi -= 1U;
        set_if(m_regs.yi >= 0x80, N);
        set_if(m_regs.yi == 0x00, Z);
    }

    void inc() {
        std::uint8_t mem{read_instruction_input()};

        mem += 1U;
        set_if(mem >= 0x80, N);
        set_if(mem == 0x00, Z);

        write_instruction_output(mem);
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

    void sta() {
        write_instruction_output(m_regs.ac);
    }

    void stx() {
        write_instruction_output(m_regs.xi);
    }

    void sty() {
        write_instruction_output(m_regs.yi);
    }

    void tax() {
        m_regs.xi = m_regs.ac;
        set_if(m_regs.xi >= 0x80, N);
        set_if(m_regs.xi == 0x00, Z);
    }

    void tay() {
        m_regs.yi = m_regs.ac;
        set_if(m_regs.yi >= 0x80, N);
        set_if(m_regs.yi == 0x00, Z);
    }

    void tsx() {
        m_regs.xi = (m_regs.sp & 0x00FF);
        set_if(m_regs.xi >= 0x80, N);
        set_if(m_regs.xi == 0x00, Z);
    }

    void txa() {
        m_regs.ac = m_regs.xi;
        set_if(m_regs.ac >= 0x80, N);
        set_if(m_regs.ac == 0x00, Z);
    }

    void txs() {
        m_regs.sp = (m_regs.sp & 0xFF00) | m_regs.xi;
        set_if(m_regs.xi >= 0x80, N);
        set_if(m_regs.xi == 0x00, Z);
    }

    void tya() {
        m_regs.ac = m_regs.yi;
        set_if(m_regs.ac >= 0x80, N);
        set_if(m_regs.ac == 0x00, Z);
    }

    void asl() {
        std::uint8_t mem{read_instruction_input()};

        set_if(mem >= 0x80, C);
        mem <<= 1;
        set_if(mem >= 0x80, N);
        set_if(mem == 0x00, Z);

        write_instruction_output(mem);
    }

    void lsr() {
        std::uint8_t mem{read_instruction_input()};

        set_if(static_cast<bool>(mem & 1), C);
        mem >>= 1;
        set_if(false,       N);
        set_if(mem == 0x00, Z);

        write_instruction_output(mem);
    }

    void rol() {
        std::uint8_t mem{read_instruction_input()};

        std::uint8_t carry_in{static_cast<bool>(m_regs.sr & C) ? std::uint8_t{1} : std::uint8_t{0}};
        std::uint8_t carry_out{static_cast<std::uint8_t>(mem >> 7)};

        mem <<= 1;
        mem += carry_in;

        set_if(carry_out, C);
        set_if(mem >= 0x80, N);
        set_if(mem == 0x00, Z);

        write_instruction_output(mem);
    }

    void ror() {
        std::uint8_t mem{read_instruction_input()};

        std::uint8_t carry_in{static_cast<bool>(m_regs.sr & C) ? std::uint8_t{0x80} : std::uint8_t{0}};
        std::uint8_t carry_out{static_cast<std::uint8_t>(mem & 1)};

        mem >>= 1;
        mem += carry_in;

        set_if(carry_out, C);
        set_if(mem >= 0x80, N);
        set_if(mem == 0x00, Z);

        write_instruction_output(mem);
    }

    void pha() {
        push(m_regs.ac);
    }

    void php() {
        push(m_regs.sr);
    }

    void pla() {
        m_regs.ac = pull();
        set_if(m_regs.ac >= 0x80, N);
        set_if(m_regs.ac == 0x00, Z);
    }

    void plp() {
        m_regs.sr = (pull() & 0xCF) | (m_regs.sr & 0x30);
    }

    void rti() {
        plp();
        rts();
    }

    void jsr() {
        std::uint8_t const pc_lo = (m_regs.pc >> 0) & 0xFF;
        std::uint8_t const pc_hi = (m_regs.pc >> 8) & 0xFF;
        push(pc_hi);
        push(pc_lo);
        m_regs.pc = m_immediate16;
    }

    void rts() {
        std::uint8_t const pc_lo = pull();
        std::uint8_t const pc_hi = pull();
        m_regs.pc = ((pc_hi << 8) & 0xFF00) | pc_lo;
    }

    void jmp_abs() {
        m_regs.pc = m_immediate16;
    }

    void jmp_ind() {
        std::uint8_t const pc_lo = m_bus->read(m_immediate16);
        std::uint8_t const pc_hi = m_bus->read(m_immediate16 + 1U);
        m_regs.pc = ((pc_hi << 8) & 0xFF00) | pc_lo;
    }

    void bcc() {
        if ((m_regs.sr & C) == 0) {
            jmp_rel();
        }
    }

    void bcs() {
        if ((m_regs.sr & C) == C) {
            jmp_rel();
        }
    }

    void beq() {
        if ((m_regs.sr & Z) == Z) {
            jmp_rel();
        }
    }

    void bne() {
        if ((m_regs.sr & Z) == 0) {
            jmp_rel();
        }
    }

    void bmi() {
        if ((m_regs.sr & N) == N) {
            jmp_rel();
        }
    }

    void bpl() {
        if ((m_regs.sr & N) == 0) {
            jmp_rel();
        }
    }

    void bvs() {
        if ((m_regs.sr & V) == V) {
            jmp_rel();
        }
    }

    void bvc() {
        if ((m_regs.sr & V) == 0) {
            jmp_rel();
        }
    }

    inline void jmp_rel() {
        *(reinterpret_cast<std::int16_t*>(&m_regs.pc)) += static_cast<std::int16_t>(static_cast<std::int8_t>(m_immediate8));
    }

    inline void push(std::uint8_t const arg) {
        m_bus->write(m_regs.sp, arg);
        m_regs.sp = (m_regs.sp & 0xFF00) | (((m_regs.sp & 0xFF) - 1U) & 0xFF);
    }

    inline std::uint8_t pull() {
        m_regs.sp = (m_regs.sp & 0xFF00) | ((m_regs.sp + 1U) & 0x00FF);
        return m_bus->read(m_regs.sp);
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
        case 2:
            if (m_instruction.parts.oper < 4) {
                goto accumulator;
            } else {
                goto unsupported;
            }
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

        accumulator:
        return m_regs.ac;

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

    void write_instruction_output(std::uint8_t data) {
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
        case 2: goto unsupported;
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
        case 0: goto unsupported;
        case 1: goto zero_page;
        case 2:
            if (m_instruction.parts.oper < 4) {
                goto accumulator;
            } else {
                goto unsupported;
            }
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
        case 0: goto unsupported;
        case 1: goto zero_page;
        case 2: goto unsupported;
        case 3: goto absolute;
        case 4: goto unsupported;
        case 5: goto zero_page_x;
        case 6: goto unsupported;
        case 7: goto absolute_x;
        default: goto unsupported;
        }

        accumulator:
        m_regs.ac = data;
        return;

        absolute:
        m_bus->write(m_immediate16, data);
        return;

        absolute_x:
        m_bus->write(m_immediate16 + m_regs.xi, data);
        return;

        absolute_y:
        m_bus->write(m_immediate16 + m_regs.yi, data);
        return;

        zero_page:
        m_bus->write(m_immediate8, data);
        return;

        zero_page_x:
        m_bus->write((m_immediate8 + m_regs.xi) & 0xFF, data);
        return;

        zero_page_y:
        m_bus->write((m_immediate8 + m_regs.yi) & 0xFF, data);
        return;

        indirect_x:
        {
            std::uint8_t lo = m_bus->read((m_immediate8 + m_regs.xi)      & 0xFF);
            std::uint8_t hi = m_bus->read((m_immediate8 + m_regs.xi + 1U) & 0xFF);
            std::uint16_t addr = ((hi << 8) | lo) & 0xFFFF;
            return m_bus->write(addr, data);
        }

        indirect_y:
        {
            std::uint8_t lo = m_bus->read(m_immediate8);
            std::uint8_t hi = m_bus->read((m_immediate8 + 1U) & 0xFF);
            std::uint16_t addr = ((hi << 8) | lo) & 0xFFFF;
            return m_bus->write((addr + m_regs.yi) & 0xFFFF, data);
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

bool Cpu::signal_irq() {
    throw std::runtime_error("not yet implemented");
}

bool Cpu::signal_nmi() {
    throw std::runtime_error("not yet implemented");
}

bool Cpu::signal_reset() {
    throw std::runtime_error("not yet implemented");
}

}
