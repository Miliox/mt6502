#pragma once
#include <cstdint>
#include <memory>

#include "mos6502/regs.hpp"
#include "mos6502/status.hpp"

namespace mos6502
{
struct IBus;
struct Registers;

#if defined(__GNUC__) || defined(__clang__)
#define FORCEINLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define FORCEINLINE __forceinline
#else
static_assert(false, "");
#endif

/// Mos Technology 6502 Microprocessor
/// @tparam Bus The concrete class that implements the bus interface for compile time polymorphism
///
/// The bus interface is composed of a read and a write function on a 16-bits address space.
/// @code
/// struct Bus {
///     std::uint8_t read(std::uint16_t addr);
///     void write(std::uint16_t addr, std::uint8_t data);
/// };
/// @endcode
template<class Bus>
class Cpu final {
public:
    /// Constructor
    /// @param bus the interface to access memory
    Cpu(std::shared_ptr<Bus> bus) : m_bus{std::move(bus)} {
        static_cast<void>(m_padding);
        m_regs.sp = 0x1FF;
        m_regs.sr = U | B;
    }

    /// Destructor
    ~Cpu() = default;

    /// Retrieve registers
    /// @return reference to registers
    Registers& regs() { return m_regs; }

    /// Signal maskable interrupt
    void signal_irq() {
        if ((m_regs.sr & I) == 0) {
            request_interrupt(0xFFFE);
        }
    }

    /// Signal non maskable interrupt
    void signal_nmi() {
        request_interrupt(0xFFFA);
    }

    /// Signal reset
    void signal_reset() {
        std::uint8_t const handler_lo = m_bus->read(0xFFFC);
        std::uint8_t const handler_hi = m_bus->read(0xFFFD);
        std::uint16_t const handler = ((handler_hi << 8) | handler_lo) & 0xFFFF;
        m_regs.pc = handler;
    }

    /// Step current instruction
    std::uint8_t step() {
        m_instruction.opcode = m_bus->read(m_regs.pc);
        m_immediate8 = m_bus->read(m_regs.pc + 1U);
        m_immediate16 = static_cast<std::uint16_t>(m_bus->read(m_regs.pc + 2U) << 8) + m_immediate8;
        m_extra_cycles = 0U;

        /// Lookup Table for Instruction Length
        /// @note BRK (00) instruction length includes mark byte
        constexpr std::array<std::uint8_t, 256> InstructionLength = {
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
        constexpr std::array<std::uint8_t, 256> InstructionCycles = {
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

        std::uint8_t length{InstructionLength[m_instruction.opcode]};
        std::uint8_t cycles{InstructionCycles[m_instruction.opcode]};
        m_regs.pc += length;

        switch (m_instruction.opcode)
        {
        case 0x61:
        case 0x65:
        case 0x69:
        case 0x6D:
        case 0x71:
        case 0x75:
        case 0x79:
        case 0x7D:
            adc();
            break;

        case 0x21:
        case 0x25:
        case 0x29:
        case 0x2D:
        case 0x31:
        case 0x35:
        case 0x39:
        case 0x3D:
            amd();
            break;

        case 0x06:
        case 0x0A:
        case 0x0E:
        case 0x16:
        case 0x1E:
            asl();
            break;

        case 0x90:
            bcc();
            break;

        case 0xB0:
            bcs();
            break;

        case 0xF0:
            beq();
            break;

        case 0x30:
            bmi();
            break;

        case 0xD0:
            bne();
            break;

        case 0x10:
            bpl();
            break;

        case 0x50:
            bvc();
            break;

        case 0x70:
            bvs();
            break;

        case 0x00:
            brk();
            break;

        case 0x24:
        case 0x2C:
            bit();
            break;

        case 0x18:
            clc();
            break;

        case 0xD8:
            cld();
            break;

        case 0x58:
            cli();
            break;

        case 0xB8:
            clv();
            break;

        case 0xC1:
        case 0xC5:
        case 0xC9:
        case 0xCD:
        case 0xD1:
        case 0xD5:
        case 0xD9:
        case 0xDD:
            cmp();
            break;

        case 0xE0:
        case 0xE4:
        case 0xEC:
            cpx();
            break;

        case 0xC0:
        case 0xC4:
        case 0xCC:
            cpy();
            break;

        case 0x41:
        case 0x45:
        case 0x49:
        case 0x4D:
        case 0x51:
        case 0x55:
        case 0x59:
        case 0x5D:
            eor();
            break;

        case 0xC6:
        case 0xCE:
        case 0xD6:
        case 0xDE:
            dec();
            break;

        case 0xCA:
            dex();
            break;

        case 0x88:
            dey();
            break;

        case 0xE6:
        case 0xEE:
        case 0xF6:
        case 0xFE:
            inc();
            break;

        case 0xE8:
            inx();
            break;

        case 0xC8:
            iny();
            break;

        case 0x20:
            jsr();
            break;

        case 0x4C:
            jmp_abs();
            break;

        case 0x6C:
            jmp_ind();
            break;

        case 0xA1:
        case 0xA5:
        case 0xA9:
        case 0xAD:
        case 0xB1:
        case 0xB5:
        case 0xB9:
        case 0xBD:
            lda();
            break;

        case 0xA2:
        case 0xA6:
        case 0xB6:
        case 0xAE:
        case 0xBE:
            ldx();
            break;

        case 0xA0:
        case 0xA4:
        case 0xAC:
        case 0xB4:
        case 0xBC:
            ldy();
            break;

        case 0x46:
        case 0x4A:
        case 0x4E:
        case 0x56:
        case 0x5E:
            lsr();
            break;

        case 0xEA:
            nop();
            break;

        case 0x01:
        case 0x05:
        case 0x09:
        case 0x0D:
        case 0x11:
        case 0x15:
        case 0x19:
        case 0x1D:
            ora();
            break;

        case 0x48:
            pha();
            break;

        case 0x08:
            php();
            break;

        case 0x68:
            pla();
            break;

        case 0x28:
            plp();
            break;

        case 0x26:
        case 0x2A:
        case 0x2E:
        case 0x36:
        case 0x3E:
            rol();
            break;

        case 0x66:
        case 0x6A:
        case 0x6E:
        case 0x76:
        case 0x7E:
            ror();
            break;

        case 0x40:
            rti();
            break;

        case 0x60:
            rts();
            break;

        case 0xE1:
        case 0xE5:
        case 0xE9:
        case 0xED:
        case 0xF1:
        case 0xF5:
        case 0xF9:
        case 0xFD:
            sbc();
            break;

        case 0x38:
            sec();
            break;

        case 0xF8:
            sed();
            break;

        case 0x78:
            sei();
            break;

        case 0x81:
        case 0x85:
        case 0x8D:
        case 0x91:
        case 0x95:
        case 0x99:
        case 0x9D:
            sta();
            break;

        case 0x86:
        case 0x8E:
        case 0x96:
            stx();
            break;

        case 0x84:
        case 0x8C:
        case 0x94:
            sty();
            break;

        case 0xAA:
            tax();
            break;

        case 0xA8:
            tay();
            break;

        case 0xBA:
            tsx();
            break;

        case 0x8A:
            txa();
            break;

        case 0x9A:
            txs();
            break;

        case 0x98:
            tya();
            break;

        default:
            illegal();
            break;
        }

        return cycles + m_extra_cycles;
    }

private:
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

    std::shared_ptr<Bus> m_bus;

    Registers m_regs{};

    Instruction m_instruction{};

    std::uint8_t m_immediate8{};

    std::uint16_t m_immediate16{};

    std::uint8_t m_extra_cycles{};

    std::array<std::uint8_t, 3> const m_padding{};

    [[ noreturn ]] void illegal() {
        char upper_half = static_cast<char>(m_instruction.opcode & 0xF0);
        if (upper_half < 10) {
            upper_half += '0';
        } else {
            upper_half += 'A';
            upper_half -= 10;
        }

        char lower_half = static_cast<char>(m_instruction.opcode & 0x0F);
        if (lower_half < 10) {
            lower_half += '0';
        } else {
            lower_half += 'A';
            lower_half -= 10;
        }

        std::string reason{"Illegal instruction: 0x"};
        reason += upper_half;
        reason += lower_half;
        std::cerr << "panic!: " << reason << std::endl; 
        std::abort();
    }

    void brk() FORCEINLINE {
        request_interrupt(0xFFFE, true);
    }

    void clc() FORCEINLINE {
        m_regs.sr &= ~C;
    }

    void cld() FORCEINLINE {
        m_regs.sr &= ~D;
    }

    void cli() FORCEINLINE {
        m_regs.sr &= ~I;
    }

    void clv() FORCEINLINE {
        m_regs.sr &= ~V;
    }

    void sec() FORCEINLINE {
        m_regs.sr |= C;
    }

    void sed() FORCEINLINE {
        m_regs.sr |= D;
    }

    void sei() FORCEINLINE {
        m_regs.sr |= I;
    }

    void nop() FORCEINLINE {
    }

    void adc() FORCEINLINE {
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

    void sbc() FORCEINLINE {
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

    void amd() FORCEINLINE {
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

    void bit() FORCEINLINE {
        std::uint8_t acc{m_regs.ac};
        std::uint8_t mem{read_instruction_input()};

        std::uint8_t res{};
        std::uint8_t z_out{};

        __asm__ __volatile__("andb %%bl, %%al" : "=a" (res) : "a" (acc), "b" (mem));
        __asm__ __volatile__("setz %0" : "=g" (z_out));

        m_regs.sr = (m_regs.sr & 0x3D) | (mem & 0xC0) | ((z_out << 1) & 0x02);
    }

    void eor() FORCEINLINE {
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

    void ora() FORCEINLINE {
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

    void cmp() FORCEINLINE {
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

    void cpx() FORCEINLINE {
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

    void cpy() FORCEINLINE {
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

    void dec() FORCEINLINE {
        std::uint8_t mem{read_instruction_input()};

        mem -= 1U;
        set_if(mem >= 0x80, N);
        set_if(mem == 0x00, Z);

        write_instruction_output(mem);
    }

    void dex() FORCEINLINE {
        m_regs.xi -= 1U;
        set_if(m_regs.xi >= 0x80, N);
        set_if(m_regs.xi == 0x00, Z);
    }

    void dey() FORCEINLINE {
        m_regs.yi -= 1U;
        set_if(m_regs.yi >= 0x80, N);
        set_if(m_regs.yi == 0x00, Z);
    }

    void inc() FORCEINLINE {
        std::uint8_t mem{read_instruction_input()};

        mem += 1U;
        set_if(mem >= 0x80, N);
        set_if(mem == 0x00, Z);

        write_instruction_output(mem);
    }

    void inx() FORCEINLINE {
        m_regs.xi += 1U;
        set_if(m_regs.xi >= 0x80, N);
        set_if(m_regs.xi == 0x00, Z);
    }

    void iny() FORCEINLINE {
        m_regs.yi += 1U;
        set_if(m_regs.yi >= 0x80, N);
        set_if(m_regs.yi == 0x00, Z);
    }

    void lda() FORCEINLINE {
        m_regs.ac = read_instruction_input();
        set_if(m_regs.ac >= 128U, N);
        set_if(m_regs.ac == 0U,   Z);
    }

    void ldx() FORCEINLINE {
        m_regs.xi = read_instruction_input();
        set_if(m_regs.xi >= 128U, N);
        set_if(m_regs.xi == 0U,   Z);
    }

    void ldy() FORCEINLINE {
        m_regs.yi = read_instruction_input();
        set_if(m_regs.yi >= 128U, N);
        set_if(m_regs.yi == 0U,   Z);
    }

    void sta() FORCEINLINE {
        write_instruction_output(m_regs.ac);
    }

    void stx() FORCEINLINE {
        write_instruction_output(m_regs.xi);
    }

    void sty() FORCEINLINE {
        write_instruction_output(m_regs.yi);
    }

    void tax()FORCEINLINE {
        m_regs.xi = m_regs.ac;
        set_if(m_regs.xi >= 0x80, N);
        set_if(m_regs.xi == 0x00, Z);
    }

    void tay() FORCEINLINE {
        m_regs.yi = m_regs.ac;
        set_if(m_regs.yi >= 0x80, N);
        set_if(m_regs.yi == 0x00, Z);
    }

    void tsx() FORCEINLINE {
        m_regs.xi = (m_regs.sp & 0x00FF);
        set_if(m_regs.xi >= 0x80, N);
        set_if(m_regs.xi == 0x00, Z);
    }

    void txa() FORCEINLINE {
        m_regs.ac = m_regs.xi;
        set_if(m_regs.ac >= 0x80, N);
        set_if(m_regs.ac == 0x00, Z);
    }

    void txs() FORCEINLINE {
        m_regs.sp = (m_regs.sp & 0xFF00) | m_regs.xi;
        set_if(m_regs.xi >= 0x80, N);
        set_if(m_regs.xi == 0x00, Z);
    }

    void tya()FORCEINLINE {
        m_regs.ac = m_regs.yi;
        set_if(m_regs.ac >= 0x80, N);
        set_if(m_regs.ac == 0x00, Z);
    }

    void asl() FORCEINLINE {
        std::uint8_t mem{read_instruction_input()};

        set_if(mem >= 0x80, C);
        mem <<= 1;
        set_if(mem >= 0x80, N);
        set_if(mem == 0x00, Z);

        write_instruction_output(mem);
    }

    void lsr() FORCEINLINE {
        std::uint8_t mem{read_instruction_input()};

        set_if(static_cast<bool>(mem & 1), C);
        mem >>= 1;
        set_if(false,       N);
        set_if(mem == 0x00, Z);

        write_instruction_output(mem);
    }

    void rol() FORCEINLINE {
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

    void ror()FORCEINLINE {
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

    void pha() FORCEINLINE {
        push(m_regs.ac);
    }

    void php() FORCEINLINE {
        push(m_regs.sr);
    }

    void pla() FORCEINLINE {
        m_regs.ac = pull();
        set_if(m_regs.ac >= 0x80, N);
        set_if(m_regs.ac == 0x00, Z);
    }

    void plp() FORCEINLINE {
        m_regs.sr = (pull() & 0xCF) | (m_regs.sr & 0x30);
    }

    void rti() FORCEINLINE {
        plp();
        rts();
    }

    void jsr() FORCEINLINE {
        std::uint8_t const pc_lo = (m_regs.pc >> 0) & 0xFF;
        std::uint8_t const pc_hi = (m_regs.pc >> 8) & 0xFF;
        push(pc_hi);
        push(pc_lo);
        m_regs.pc = m_immediate16;
    }

    void rts() FORCEINLINE {
        std::uint8_t const pc_lo = pull();
        std::uint8_t const pc_hi = pull();
        m_regs.pc = ((pc_hi << 8) & 0xFF00) | pc_lo;
    }

    void jmp_abs() FORCEINLINE {
        m_regs.pc = m_immediate16;
    }

    void jmp_ind() FORCEINLINE {
        std::uint8_t const pc_lo = m_bus->read(m_immediate16);
        std::uint8_t const pc_hi = m_bus->read(m_immediate16 + 1U);
        m_regs.pc = ((pc_hi << 8) & 0xFF00) | pc_lo;
    }

    void bcc() FORCEINLINE {
        if ((m_regs.sr & C) == 0) {
            jmp_rel();
        }
    }

    void bcs() FORCEINLINE {
        if ((m_regs.sr & C) == C) {
            jmp_rel();
        }
    }

    void beq() FORCEINLINE {
        if ((m_regs.sr & Z) == Z) {
            jmp_rel();
        }
    }

    void bne() FORCEINLINE {
        if ((m_regs.sr & Z) == 0) {
            jmp_rel();
        }
    }

    void bmi() FORCEINLINE {
        if ((m_regs.sr & N) == N) {
            jmp_rel();
        }
    }

    void bpl() FORCEINLINE {
        if ((m_regs.sr & N) == 0) {
            jmp_rel();
        }
    }

    void bvs() FORCEINLINE {
        if ((m_regs.sr & V) == V) {
            jmp_rel();
        }
    }

    void bvc() FORCEINLINE {
        if ((m_regs.sr & V) == 0) {
            jmp_rel();
        }
    }

    void request_interrupt(std::uint16_t addr, bool software = false) FORCEINLINE {
        std::uint8_t const pc_lo = (m_regs.pc >> 0) & 0xFF;
        std::uint8_t const pc_hi = (m_regs.pc >> 8) & 0xFF;

        std::uint8_t const handler_lo = m_bus->read(addr);
        std::uint8_t const handler_hi = m_bus->read(addr + 1U);
        std::uint16_t const handler = ((handler_hi << 8) | handler_lo) & 0xFFFF;

        std::uint8_t status = m_regs.sr;
        if (software) {
            status |= B;
        } else {
            status &= ~B;
        }

        // Save current context
        push(pc_hi);
        push(pc_lo);
        push(status);

        // Interrupt handler
        m_regs.pc = handler;

        // Disable Interrupts
        m_regs.sr |= I;
    }

    void jmp_rel() FORCEINLINE {
        *(reinterpret_cast<std::int16_t*>(&m_regs.pc)) += static_cast<std::int16_t>(static_cast<std::int8_t>(m_immediate8));
    }

    void push(std::uint8_t const arg) FORCEINLINE {
        m_bus->write(m_regs.sp, arg);
        m_regs.sp = (m_regs.sp & 0xFF00) | (((m_regs.sp & 0xFF) - 1U) & 0xFF);
    }

    std::uint8_t pull() FORCEINLINE {
        m_regs.sp = (m_regs.sp & 0xFF00) | ((m_regs.sp + 1U) & 0x00FF);
        return m_bus->read(m_regs.sp);
    }

    void set_if(bool cond, std::uint8_t status) FORCEINLINE {
        if (cond) {
            m_regs.sr |= status;
        } else {
            m_regs.sr &= ~status;
        }
    }

    std::uint8_t read_instruction_input() FORCEINLINE {
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
        illegal();
    }

    void write_instruction_output(std::uint8_t data) FORCEINLINE {
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
        illegal();
    }
};
}
