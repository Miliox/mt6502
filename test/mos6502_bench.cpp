#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include <memory>

#include "mos6502/bus.hpp"
#include "mos6502/cpu.hpp"

class BenchBus final : public mos6502::IBus {
public:
    BenchBus(std::uint8_t opcode);

    std::uint8_t read(std::uint16_t addr) override;

    void write(std::uint16_t addr, std::uint8_t data) override;

private:
    std::uint8_t const m_opcode{};
    std::array<std::uint8_t, 7U> m_padding{};
};


BenchBus::BenchBus(std::uint8_t opcode) : m_opcode{opcode} {
    static_cast<void>(m_padding);
}

std::uint8_t BenchBus::read(std::uint16_t addr) {
    static_cast<void>(addr);
    return m_opcode;
}

void BenchBus::write(std::uint16_t addr, std::uint8_t data) {
    static_cast<void>(addr);
    static_cast<void>(data);
}

#define INSTRUCTION_BENCHMARK(name, opcode) \
{ \
    std::shared_ptr<BenchBus> a_bus{new BenchBus{0xE0}}; \
    std::shared_ptr<mos6502::Cpu<BenchBus>> a_cpu{new mos6502::Cpu<BenchBus>{a_bus}}; \
    std::shared_ptr<mos6502::Cpu<mos6502::IBus>> a_vcpu{new mos6502::Cpu<mos6502::IBus>{a_bus}}; \
    std::stringstream title{}; \
    title << "instruction " << name << " on concrete bus"; \
    benchmark.run(title.str(), [&] { a_cpu->step(); }); \
    title = std::stringstream{}; \
    title << "instruction " << name << " on virtual bus"; \
    benchmark.run(title.str(), [&] { a_vcpu->step(); }); \
} \

int main(int argc, char** argv)
{
    static_cast<void>(argc);
    static_cast<void>(argv);
    auto benchmark = ankerl::nanobench::Bench();
    benchmark.minEpochIterations(2'000'000U);

    {
        std::shared_ptr<BenchBus> bus{new BenchBus{0xE0}};
        std::shared_ptr<mos6502::IBus> ibus{bus};
        benchmark.run("Direct Bus Read", [&] { static_cast<void>(bus->read(0x00)); });
        benchmark.run("Virtual Bus Read", [&] { static_cast<void>(ibus->read(0x00)); });
    }

    {
        std::shared_ptr<BenchBus> bus{new BenchBus{0xE0}};
        std::shared_ptr<mos6502::IBus> ibus{bus};
        benchmark.run("Direct Bus Write", [&] { static_cast<void>(bus->write(0x00, 0x00)); });
        benchmark.run("Virtual Bus Write", [&] { static_cast<void>(ibus->write(0x00, 0x00)); });
    }

    INSTRUCTION_BENCHMARK("BRK", 0x00);
    INSTRUCTION_BENCHMARK("BPL", 0x10);
    INSTRUCTION_BENCHMARK("JSR", 0x20);
    INSTRUCTION_BENCHMARK("BMI", 0x30);
    INSTRUCTION_BENCHMARK("RTI", 0x40);
    INSTRUCTION_BENCHMARK("BVC", 0x50);
    INSTRUCTION_BENCHMARK("RTS", 0x60);
    INSTRUCTION_BENCHMARK("BVS", 0x70);
    INSTRUCTION_BENCHMARK("BCC", 0x90);
    INSTRUCTION_BENCHMARK("LDY", 0xA0);
    INSTRUCTION_BENCHMARK("BCS", 0xB0);
    INSTRUCTION_BENCHMARK("CPY", 0xC0);
    INSTRUCTION_BENCHMARK("BNE", 0xD0);
    INSTRUCTION_BENCHMARK("CPX", 0xE0);
    INSTRUCTION_BENCHMARK("BEQ", 0xF0);

    INSTRUCTION_BENCHMARK("ORA_X_IND", 0x01);
    INSTRUCTION_BENCHMARK("ORA_IND_Y", 0x11);
    INSTRUCTION_BENCHMARK("AND_X_IND", 0x21);
    INSTRUCTION_BENCHMARK("AND_IND_Y", 0x31);
    INSTRUCTION_BENCHMARK("EOR_X_IND", 0x41);
    INSTRUCTION_BENCHMARK("EOR_IND_Y", 0x51);
    INSTRUCTION_BENCHMARK("ADC_X_IND", 0x61);
    INSTRUCTION_BENCHMARK("ADC_IND_Y", 0x71);
    INSTRUCTION_BENCHMARK("STA_X_IND", 0x81);
    INSTRUCTION_BENCHMARK("STA_IND_Y", 0x91);
    INSTRUCTION_BENCHMARK("LDA_X_IND", 0xA1);
    INSTRUCTION_BENCHMARK("LDA_IND_Y", 0xB1);
    INSTRUCTION_BENCHMARK("CMP_X_IND", 0xC1);
    INSTRUCTION_BENCHMARK("CMP_IND_Y", 0xD1);
    INSTRUCTION_BENCHMARK("SBC_X_IND", 0xE1);
    INSTRUCTION_BENCHMARK("SBC_IND_Y", 0xF1);

    INSTRUCTION_BENCHMARK("LDX_IMM", 0xA2);

    INSTRUCTION_BENCHMARK("BIT_ZPG",   0x14);
    INSTRUCTION_BENCHMARK("STY_ZPG",   0x84);
    INSTRUCTION_BENCHMARK("STY_ZPG_X", 0x94);
    INSTRUCTION_BENCHMARK("LDY_ZPG",   0xA4);
    INSTRUCTION_BENCHMARK("LDY_ZPG_X", 0xB4);
    INSTRUCTION_BENCHMARK("CPY_ZPG",   0xC4);
    INSTRUCTION_BENCHMARK("CPX_ZPG",   0xE4);

    INSTRUCTION_BENCHMARK("ORA_ZPG",   0x05);
    INSTRUCTION_BENCHMARK("ORA_ZPG_X", 0x15);
    INSTRUCTION_BENCHMARK("AND_ZPG",   0x25);
    INSTRUCTION_BENCHMARK("AND_ZPG_X", 0x35);
    INSTRUCTION_BENCHMARK("EOR_ZPG",   0x45);
    INSTRUCTION_BENCHMARK("EOR_ZPG_X", 0x55);
    INSTRUCTION_BENCHMARK("ADC_ZPG",   0x65);
    INSTRUCTION_BENCHMARK("ADC_ZPG_X", 0x75);
    INSTRUCTION_BENCHMARK("STA_ZPG",   0x85);
    INSTRUCTION_BENCHMARK("STA_ZPG_X", 0x95);
    INSTRUCTION_BENCHMARK("LDA_ZPG",   0xA5);
    INSTRUCTION_BENCHMARK("LDA_ZPG_X", 0xB5);
    INSTRUCTION_BENCHMARK("CMP_ZPG",   0xC5);
    INSTRUCTION_BENCHMARK("CMP_ZPG_X", 0xD5);
    INSTRUCTION_BENCHMARK("SBC_ZPG",   0xE5);
    INSTRUCTION_BENCHMARK("SBC_ZPG_X", 0xF5);

    INSTRUCTION_BENCHMARK("ASL_ZPG",   0x06);
    INSTRUCTION_BENCHMARK("ASL_ZPG_X", 0x16);
    INSTRUCTION_BENCHMARK("ROL_ZPG",   0x26);
    INSTRUCTION_BENCHMARK("ROL_ZPG_X", 0x36);
    INSTRUCTION_BENCHMARK("LSR_ZPG",   0x46);
    INSTRUCTION_BENCHMARK("LSR_ZPG_X", 0x56);
    INSTRUCTION_BENCHMARK("ROR_ZPG",   0x66);
    INSTRUCTION_BENCHMARK("ROR_ZPG_X", 0x76);
    INSTRUCTION_BENCHMARK("STX_ZPG",   0x86);
    INSTRUCTION_BENCHMARK("STX_ZPG_X", 0x96);
    INSTRUCTION_BENCHMARK("LDX_ZPG",   0xA6);
    INSTRUCTION_BENCHMARK("LDX_ZPG_X", 0xB6);
    INSTRUCTION_BENCHMARK("DEC_ZPG",   0xC6);
    INSTRUCTION_BENCHMARK("DEC_ZPG_X", 0xD6);
    INSTRUCTION_BENCHMARK("INC_ZPG",   0xE6);
    INSTRUCTION_BENCHMARK("INC_ZPG_X", 0xF6);

    INSTRUCTION_BENCHMARK("PHP", 0x08);
    INSTRUCTION_BENCHMARK("CLC", 0x18);
    INSTRUCTION_BENCHMARK("PLP", 0x28);
    INSTRUCTION_BENCHMARK("SEC", 0x38);
    INSTRUCTION_BENCHMARK("PHA", 0x48);
    INSTRUCTION_BENCHMARK("CLI", 0x58);
    INSTRUCTION_BENCHMARK("PLA", 0x68);
    INSTRUCTION_BENCHMARK("SEI", 0x78);
    INSTRUCTION_BENCHMARK("DEY", 0x88);
    INSTRUCTION_BENCHMARK("TYA", 0x98);
    INSTRUCTION_BENCHMARK("TAY", 0xA8);
    INSTRUCTION_BENCHMARK("CLV", 0xB8);
    INSTRUCTION_BENCHMARK("INY", 0xC8);
    INSTRUCTION_BENCHMARK("CLD", 0xD8);
    INSTRUCTION_BENCHMARK("INX", 0xE8);
    INSTRUCTION_BENCHMARK("SED", 0xF8);

    INSTRUCTION_BENCHMARK("ORA_IMM",   0x09);
    INSTRUCTION_BENCHMARK("ORA_ABS_Y", 0x19);
    INSTRUCTION_BENCHMARK("AND_IMM",   0x29);
    INSTRUCTION_BENCHMARK("AND_ABS_Y", 0x39);
    INSTRUCTION_BENCHMARK("EOR_IMM",   0x49);
    INSTRUCTION_BENCHMARK("EOR_ABS_Y", 0x59);
    INSTRUCTION_BENCHMARK("ADC_IMM",   0x69);
    INSTRUCTION_BENCHMARK("ADC_ABS_Y", 0x79);
    INSTRUCTION_BENCHMARK("STA_ABS_Y", 0x99);
    INSTRUCTION_BENCHMARK("LDA_IMM",   0xA9);
    INSTRUCTION_BENCHMARK("LDA_ABS_Y", 0xB9);
    INSTRUCTION_BENCHMARK("CMP_IMM",   0xC9);
    INSTRUCTION_BENCHMARK("CMP_ABS_Y", 0xD9);
    INSTRUCTION_BENCHMARK("SBC_IMM",   0xE9);
    INSTRUCTION_BENCHMARK("SBC_ABS_Y", 0xF9);

    INSTRUCTION_BENCHMARK("ASL_A", 0x0A);
    INSTRUCTION_BENCHMARK("ROL_A", 0x2A);
    INSTRUCTION_BENCHMARK("LSR_A", 0x4A);
    INSTRUCTION_BENCHMARK("ROR_A", 0x6A);
    INSTRUCTION_BENCHMARK("TXA",   0x8A);
    INSTRUCTION_BENCHMARK("TXS",   0x9A);
    INSTRUCTION_BENCHMARK("TAX",   0xAA);
    INSTRUCTION_BENCHMARK("TSX",   0xBA);
    INSTRUCTION_BENCHMARK("DEX",   0xCA);
    INSTRUCTION_BENCHMARK("NOP",   0xEA);

    INSTRUCTION_BENCHMARK("BIT_ABS", 0x2C);
    INSTRUCTION_BENCHMARK("JMP_ABS", 0x4C);
    INSTRUCTION_BENCHMARK("JMP_IND", 0x6C);
    INSTRUCTION_BENCHMARK("STY_ABS", 0x8C);
    INSTRUCTION_BENCHMARK("LDY_ABS", 0xAC);
    INSTRUCTION_BENCHMARK("LDY_ABS_X", 0xBC);
    INSTRUCTION_BENCHMARK("CPY_ABS", 0xCC);
    INSTRUCTION_BENCHMARK("CPX_ABS", 0xEC);

    INSTRUCTION_BENCHMARK("ORA_ABS",   0x0D);
    INSTRUCTION_BENCHMARK("ORA_ABS_X", 0x1D);
    INSTRUCTION_BENCHMARK("AND_ABS",   0x2D);
    INSTRUCTION_BENCHMARK("AND_ABS_X", 0x3D);
    INSTRUCTION_BENCHMARK("EOR_ABS",   0x4D);
    INSTRUCTION_BENCHMARK("EOR_ABS_X", 0x5D);
    INSTRUCTION_BENCHMARK("ADC_ABS",   0x6D);
    INSTRUCTION_BENCHMARK("ADC_ABS_X", 0x7D);
    INSTRUCTION_BENCHMARK("STA_ABS",   0x8D);
    INSTRUCTION_BENCHMARK("STA_ABS_X", 0x9D);
    INSTRUCTION_BENCHMARK("LDA_ABS",   0xAD);
    INSTRUCTION_BENCHMARK("LDA_ABS_X", 0xBD);
    INSTRUCTION_BENCHMARK("CMP_ABS",   0xCD);
    INSTRUCTION_BENCHMARK("CMP_ABS_X", 0xDD);
    INSTRUCTION_BENCHMARK("SBC_ABS",   0xED);
    INSTRUCTION_BENCHMARK("SBC_ABS_X", 0xFD);

    INSTRUCTION_BENCHMARK("ASL_ABS",   0x0E);
    INSTRUCTION_BENCHMARK("ASL_ABS_X", 0x1E);
    INSTRUCTION_BENCHMARK("ROL_ABS",   0x2E);
    INSTRUCTION_BENCHMARK("ROL_ABS_X", 0x3E);
    INSTRUCTION_BENCHMARK("LSR_ABS",   0x4E);
    INSTRUCTION_BENCHMARK("LSR_ABS_X", 0x5E);
    INSTRUCTION_BENCHMARK("ROR_ABS",   0x6E);
    INSTRUCTION_BENCHMARK("ROR_ABS_X", 0x7E);
    INSTRUCTION_BENCHMARK("STX_ABS",   0x8E);
    INSTRUCTION_BENCHMARK("LDX_ABS",   0xAE);
    INSTRUCTION_BENCHMARK("LDX_ABS_X", 0xBE);
    INSTRUCTION_BENCHMARK("DEC_ABS",   0xCE);
    INSTRUCTION_BENCHMARK("DEC_ABS_X", 0xDE);
    INSTRUCTION_BENCHMARK("INC_ABS",   0xEE);
    INSTRUCTION_BENCHMARK("INC_ABS_X", 0xFE);

    return 0;
}
