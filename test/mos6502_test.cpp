#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <atomic>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <thread>

#include "mos6502/bus.hpp"
#include "mos6502/cpu.hpp"
#include "mos6502/regs.hpp"
#include "mos6502/status.hpp"

class MockBus final : public mos6502::IBus {
public:
    MockBus() = default;
    ~MockBus() override = default;

    std::uint8_t read(std::uint16_t addr) override;

    void write(std::uint16_t addr, std::uint8_t data) override;

    void mockAddressValue(std::uint16_t addr, std::uint8_t data);

    std::uint8_t readWrittenValue(std::uint16_t addr);

private:
    std::unordered_map<std::uint16_t, std::uint8_t> read_map{};
    std::unordered_map<std::uint16_t, std::uint8_t> write_map{};
};

std::uint8_t MockBus::read(std::uint16_t addr) {
    return read_map.at(addr);
}

void MockBus::write(std::uint16_t addr, std::uint8_t data) {
    write_map.insert_or_assign(addr, data);
}

void MockBus::mockAddressValue(std::uint16_t addr, std::uint8_t data) {
    read_map.insert_or_assign(addr, data);
}

std::uint8_t MockBus::readWrittenValue(std::uint16_t addr) {
    return write_map.at(addr);
}

class CpuFixture {
public:
    CpuFixture();

protected:
    std::shared_ptr<MockBus> m_bus{new MockBus{}};
    mos6502::Cpu<MockBus> m_cpu{m_bus};
};

CpuFixture::CpuFixture() {
    m_bus->mockAddressValue(0x01, 0x00);
    m_bus->mockAddressValue(0x02, 0x00);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction SEC") {
    m_bus->mockAddressValue(0x00, 0x38);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().sr == (mos6502::C | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction SED" ) {
    m_bus->mockAddressValue(0x00, 0xF8);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().sr == (mos6502::D | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction SEI" ) {
    m_bus->mockAddressValue(0x00, 0x78);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().sr == (mos6502::I | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction CLC" ) {
    m_bus->mockAddressValue(0x00, 0x18);

    m_cpu.regs().sr = 0xFF;
    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE((m_cpu.regs().sr & mos6502::C) == 0);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction CLD" ) {
    m_bus->mockAddressValue(0x00, 0xD8);

    m_cpu.regs().sr = 0xFF;
    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE((m_cpu.regs().sr & mos6502::D) == 0);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction CLI" ) {
    m_bus->mockAddressValue(0x00, 0x58);

    m_cpu.regs().sr = 0xFF;
    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE((m_cpu.regs().sr & mos6502::I) == 0);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction CLV" ) {
    m_bus->mockAddressValue(0x00, 0xB8);

    m_cpu.regs().sr = 0xFF;
    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE((m_cpu.regs().sr & mos6502::V) == 0);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction LDA,LDX,LDY" ) {
    m_bus->mockAddressValue(0x00, 0xA9); // LDA
    m_bus->mockAddressValue(0x01, 0x80); // IMM

    m_bus->mockAddressValue(0x02, 0xAD); // LDA
    m_bus->mockAddressValue(0x03, 0xEF); // ABS LO
    m_bus->mockAddressValue(0x04, 0xBE); // ABS HI

    m_bus->mockAddressValue(0x05, 0xA5); // LDA
    m_bus->mockAddressValue(0x06, 0x80); // ZPG

    m_bus->mockAddressValue(0x07, 0xA2); // LDX
    m_bus->mockAddressValue(0x08, 0x10); // IMM

    m_bus->mockAddressValue(0x09, 0xB5); // LDA
    m_bus->mockAddressValue(0x0A, 0x71); // ZPG,X

    m_bus->mockAddressValue(0x0B, 0xBD); // LDA
    m_bus->mockAddressValue(0x0C, 0xDF); // ABS LO,X
    m_bus->mockAddressValue(0x0D, 0xBE); // ABS HI,X

    m_bus->mockAddressValue(0x0E, 0xA0); // LDY
    m_bus->mockAddressValue(0x0F, 0x02); // IMM

    m_bus->mockAddressValue(0x10, 0xB9); // LDA
    m_bus->mockAddressValue(0x11, 0x04); // ABS LO,Y
    m_bus->mockAddressValue(0x12, 0x00); // ABS HI,Y

    m_bus->mockAddressValue(0x13, 0xA1); // LDA
    m_bus->mockAddressValue(0x14, 0xF3); // (IND,X)

    m_bus->mockAddressValue(0x15, 0xB1); // LDA
    m_bus->mockAddressValue(0x16, 0xFF); // (IND),Y

    m_bus->mockAddressValue(0x17, 0xA6); // LDX
    m_bus->mockAddressValue(0x18, 0x00); // ZPG

    m_bus->mockAddressValue(0x19, 0xB6); // LDX
    m_bus->mockAddressValue(0x1A, 0x00); // ZPG,Y

    m_bus->mockAddressValue(0x1B, 0xAE); // LDX
    m_bus->mockAddressValue(0x1C, 0xEF); // ABS LO
    m_bus->mockAddressValue(0x1D, 0xBE); // ABS HI

    m_bus->mockAddressValue(0x1E, 0xBE); // LDX
    m_bus->mockAddressValue(0x1F, 0x00); // ABS LO,Y
    m_bus->mockAddressValue(0x20, 0xA9); // ABS HI,Y

    m_bus->mockAddressValue(0x21, 0xA4); // LDY
    m_bus->mockAddressValue(0x22, 0x80); // ZPG

    m_bus->mockAddressValue(0x23, 0xB4); // LDY
    m_bus->mockAddressValue(0x24, 0x41); // ZPG,X

    m_bus->mockAddressValue(0x25, 0xAC); // LDY
    m_bus->mockAddressValue(0x26, 0xEF); // ABS LO
    m_bus->mockAddressValue(0x27, 0xBE); // ABS HI

    m_bus->mockAddressValue(0x28, 0xBC); // LDY
    m_bus->mockAddressValue(0x29, 0x40); // ABS LO,X
    m_bus->mockAddressValue(0x2A, 0x00); // ABS HI,X

    m_bus->mockAddressValue(0x0080, 0xFF);
    m_bus->mockAddressValue(0x0081, 0x7F);
    m_bus->mockAddressValue(0x00FF, 0x00);
    m_bus->mockAddressValue(0x0100, 0x10);
    m_bus->mockAddressValue(0xBEEF, 0x00);
    m_bus->mockAddressValue(0x1000, 0x3F);
    m_bus->mockAddressValue(0xA902, 0x40);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 0x02);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 0x05);
    REQUIRE(m_cpu.regs().ac == 0U);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 3U);
    REQUIRE(m_cpu.regs().pc == 0x07);
    REQUIRE(m_cpu.regs().ac == 0xFF);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 0x09);
    REQUIRE(m_cpu.regs().xi == 0x10);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 0x0B);
    REQUIRE(m_cpu.regs().ac == 0x7F);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 0x0E);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 0x10);
    REQUIRE(m_cpu.regs().yi == 2U);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 0x13);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 6U);
    REQUIRE(m_cpu.regs().pc == 0x15);
    REQUIRE(m_cpu.regs().ac == 0U);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 5U);
    REQUIRE(m_cpu.regs().pc == 0x17);
    REQUIRE(m_cpu.regs().ac == 0x40);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 3U);
    REQUIRE(m_cpu.regs().pc == 0x19);
    REQUIRE(m_cpu.regs().xi == 0xA9);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 0x1B);
    REQUIRE(m_cpu.regs().xi == 0xAD);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 0x1E);
    REQUIRE(m_cpu.regs().xi == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 0x21);
    REQUIRE(m_cpu.regs().xi == 0x40);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 3U);
    REQUIRE(m_cpu.regs().pc == 0x23);
    REQUIRE(m_cpu.regs().yi == 0xFF);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 0x25);
    REQUIRE(m_cpu.regs().yi == 0x7F);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 0x28);
    REQUIRE(m_cpu.regs().yi == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 0x2B);
    REQUIRE(m_cpu.regs().yi == 0xFF);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction ADC" ) {
    m_bus->mockAddressValue(0x00, 0xA9); // LDA
    m_bus->mockAddressValue(0x01, 0x50); // IMM

    m_bus->mockAddressValue(0x02, 0x69); // ADC
    m_bus->mockAddressValue(0x03, 0x10); // IMM

    m_bus->mockAddressValue(0x04, 0x69); // ADC
    m_bus->mockAddressValue(0x05, 0x20); // IMM

    m_bus->mockAddressValue(0x06, 0x69); // ADC
    m_bus->mockAddressValue(0x07, 0x80); // IMM

    m_bus->mockAddressValue(0x08, 0x18);

    m_bus->mockAddressValue(0x09, 0x69); // ADC
    m_bus->mockAddressValue(0x0A, 0x00); // IMM

    m_bus->mockAddressValue(0x0B, 0x00); // PAD

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0x50);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().ac == 0x60);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::V | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 8U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::V | mos6502::Z | mos6502::C | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 9U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::V | mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 0x0B);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction Decimal ADC" ) {
    m_bus->mockAddressValue(0x00, 0xF8); // SED

    m_bus->mockAddressValue(0x01, 0xA9); // LDA
    m_bus->mockAddressValue(0x02, 0x10); // IMM

    m_bus->mockAddressValue(0x03, 0x69); // ADC
    m_bus->mockAddressValue(0x04, 0x20); // IMM

    m_bus->mockAddressValue(0x05, 0x69); // ADC
    m_bus->mockAddressValue(0x06, 0x50); // IMM

    m_bus->mockAddressValue(0x07, 0x69); // ADC
    m_bus->mockAddressValue(0x08, 0x19); // IMM

    m_bus->mockAddressValue(0x09, 0x69); // ADC
    m_bus->mockAddressValue(0x0A, 0x01); // IMM

    m_bus->mockAddressValue(0x0B, 0x69); // ADC
    m_bus->mockAddressValue(0x0C, 0xAA); // IMM

    m_bus->mockAddressValue(0x0D, 0x00); // PAD

    REQUIRE(m_cpu.step() == 2U); // SED
    REQUIRE(m_cpu.step() == 2U); // LDA
    REQUIRE(m_cpu.step() == 2U); // ADC

    REQUIRE(m_cpu.regs().pc == 5U);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::D));
    REQUIRE(m_cpu.regs().ac == 0x30);

    REQUIRE(m_cpu.step() == 2U); // ADC

    REQUIRE(m_cpu.regs().pc == 7U);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::D | mos6502::N | mos6502::V));
    REQUIRE(m_cpu.regs().ac == 0x80);

    REQUIRE(m_cpu.step() == 2U); // ADC

    REQUIRE(m_cpu.regs().pc == 9U);
    REQUIRE(int{m_cpu.regs().sr} == (mos6502::U | mos6502::B | mos6502::D | mos6502::N));
    REQUIRE(m_cpu.regs().ac == 0x99);

    REQUIRE(m_cpu.step() == 2U); // ADC

    REQUIRE(m_cpu.regs().pc == 11U);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::D | mos6502::Z | mos6502::C));
    REQUIRE(m_cpu.regs().ac == 0x00);

    REQUIRE(m_cpu.step() == 2U); // ADC

    REQUIRE(m_cpu.regs().pc == 13U);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::D | mos6502::C));
    REQUIRE(m_cpu.regs().ac == 0x11);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction SBC" ) {
    m_bus->mockAddressValue(0x00, 0xE9); // SBC
    m_bus->mockAddressValue(0x01, 0x00); // IMM

    m_bus->mockAddressValue(0x02, 0x38); // SEC (No Borrow)

    m_bus->mockAddressValue(0x03, 0xE9); // SBC
    m_bus->mockAddressValue(0x04, 0x80); // IMM

    m_bus->mockAddressValue(0x05, 0x18); // CLC (Borrow)

    m_bus->mockAddressValue(0x06, 0xE9); // SBC
    m_bus->mockAddressValue(0x07, 0x7E); // IMM

    m_bus->mockAddressValue(0x08, 0x00); // PAD

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0xFF);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().ac == 0xFF);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::C | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 5U);
    REQUIRE(m_cpu.regs().ac == 0x7F);
    REQUIRE(m_cpu.regs().sr == (mos6502::C | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().ac == 0x7F);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 8U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::C | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction CMP" ) {
    m_bus->mockAddressValue(0x00, 0xA9); // LDA
    m_bus->mockAddressValue(0x01, 0x80); // IMM

    m_bus->mockAddressValue(0x02, 0xC9); // CMP
    m_bus->mockAddressValue(0x03, 0x80); // IMM

    m_bus->mockAddressValue(0x04, 0xC9); // CMP
    m_bus->mockAddressValue(0x05, 0x81); // IMM

    m_bus->mockAddressValue(0x06, 0xC9); // CMP
    m_bus->mockAddressValue(0x07, 0x7F); // IMM

    m_bus->mockAddressValue(0x08, 0x00); // PAD

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().ac == 0x80U);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::C | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().ac == 0x80U);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 8U);
    REQUIRE(m_cpu.regs().ac == 0x80U);
    REQUIRE(m_cpu.regs().sr == (mos6502::C | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction CPX" ) {
    m_bus->mockAddressValue(0x00, 0xA2); // LDX
    m_bus->mockAddressValue(0x01, 0x80); // IMM

    m_bus->mockAddressValue(0x02, 0xE0); // CPX
    m_bus->mockAddressValue(0x03, 0x80); // IMM

    m_bus->mockAddressValue(0x04, 0xE4); // CPX
    m_bus->mockAddressValue(0x05, 0x80); // ZPG

    m_bus->mockAddressValue(0x06, 0xEC); // CPX
    m_bus->mockAddressValue(0x07, 0x81); // ABS LO
    m_bus->mockAddressValue(0x08, 0x00); // ABS HI

    m_bus->mockAddressValue(0x0080, 0x81);
    m_bus->mockAddressValue(0x0081, 0x7F);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().xi == 0x80);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().xi == 0x80U);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::C | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 3U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().xi == 0x80U);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 9U);
    REQUIRE(m_cpu.regs().xi == 0x80U);
    REQUIRE(m_cpu.regs().sr == (mos6502::C | mos6502::U | mos6502::B));
}


TEST_CASE_FIXTURE(CpuFixture, "Instruction CPY" ) {
    m_bus->mockAddressValue(0x00, 0xA0); // LDY
    m_bus->mockAddressValue(0x01, 0x80); // IMM

    m_bus->mockAddressValue(0x02, 0xC0); // CPY
    m_bus->mockAddressValue(0x03, 0x80); // IMM

    m_bus->mockAddressValue(0x04, 0xC4); // CPY
    m_bus->mockAddressValue(0x05, 0x80); // ZPG

    m_bus->mockAddressValue(0x06, 0xCC); // CPY
    m_bus->mockAddressValue(0x07, 0x81); // ABS LO
    m_bus->mockAddressValue(0x08, 0x00); // ABS HI

    m_bus->mockAddressValue(0x0080, 0x81);
    m_bus->mockAddressValue(0x0081, 0x7F);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().yi == 0x80);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().yi == 0x80U);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::C | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 3U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().yi == 0x80U);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 9U);
    REQUIRE(m_cpu.regs().yi == 0x80U);
    REQUIRE(m_cpu.regs().sr == (mos6502::C | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction AND" ) {
    m_bus->mockAddressValue(0x00, 0xA9); // LDA
    m_bus->mockAddressValue(0x01, 0xFF); // IMM

    m_bus->mockAddressValue(0x02, 0x29); // AND
    m_bus->mockAddressValue(0x03, 0xA5); // IMM

    m_bus->mockAddressValue(0x04, 0x29); // AND
    m_bus->mockAddressValue(0x05, 0x7F); // IMM

    m_bus->mockAddressValue(0x06, 0x29); // AND
    m_bus->mockAddressValue(0x07, 0x5A); // IMM

    m_bus->mockAddressValue(0x08, 0x00); // PAD

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0xFF);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().ac == 0xA5);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().ac == 0x25);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 8U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction BIT" ) {
    m_bus->mockAddressValue(0x00, 0x24); // LDA
    m_bus->mockAddressValue(0x01, 0xFF); // ZPG

    m_bus->mockAddressValue(0x02, 0x2C); // LDA
    m_bus->mockAddressValue(0x03, 0xEF); // ABS LO
    m_bus->mockAddressValue(0x04, 0xBE); // ABS HI

    m_bus->mockAddressValue(0x00FF, 0xFF);
    m_bus->mockAddressValue(0xBEEF, 0x00);

    REQUIRE(m_cpu.step() == 3U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::V | mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 5U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction ORA" ) {
    m_bus->mockAddressValue(0x00, 0x09); // ORA
    m_bus->mockAddressValue(0x01, 0x00); // IMM

    m_bus->mockAddressValue(0x02, 0x09); // ORA
    m_bus->mockAddressValue(0x03, 0x0F); // IMM

    m_bus->mockAddressValue(0x04, 0x09); // ORA
    m_bus->mockAddressValue(0x05, 0xF0); // IMM

    m_bus->mockAddressValue(0x06, 0x00); // PAD

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().ac == 0x0F);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().ac == 0xFF);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction EOR" ) {
    m_bus->mockAddressValue(0x00, 0x49); // EOR
    m_bus->mockAddressValue(0x01, 0x0F); // IMM

    m_bus->mockAddressValue(0x02, 0x49); // EOR
    m_bus->mockAddressValue(0x03, 0xF0); // IMM

    m_bus->mockAddressValue(0x04, 0x49); // EOR
    m_bus->mockAddressValue(0x05, 0xFF); // IMM

    m_bus->mockAddressValue(0x06, 0x00); // PAD

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0x0F);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().ac == 0xFF);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction INX" ) {
    m_bus->mockAddressValue(0x00, 0xA2); // LDX
    m_bus->mockAddressValue(0x01, 0xFE); // IMM

    m_bus->mockAddressValue(0x02, 0xE8); // INX
    m_bus->mockAddressValue(0x03, 0xE8); // INX
    m_bus->mockAddressValue(0x04, 0xE8); // INX

    m_bus->mockAddressValue(0x05, 0x00); // PAD
    m_bus->mockAddressValue(0x06, 0x00); // PAD

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().xi == 0xFE);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().xi == 0xFF);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().xi == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction INY" ) {
    m_bus->mockAddressValue(0x00, 0xA0); // LDY
    m_bus->mockAddressValue(0x01, 0xFE); // IMM

    m_bus->mockAddressValue(0x02, 0xC8); // INY
    m_bus->mockAddressValue(0x03, 0xC8); // INY
    m_bus->mockAddressValue(0x04, 0xC8); // INY

    m_bus->mockAddressValue(0x05, 0x00); // PAD
    m_bus->mockAddressValue(0x06, 0x00); // PAD

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().yi == 0xFE);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().yi == 0xFF);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().yi == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction INC" ) {
    m_bus->mockAddressValue(0x00, 0xE6); // INC
    m_bus->mockAddressValue(0x01, 0x80); // ZPG

    m_bus->mockAddressValue(0x02, 0xEE); // INC
    m_bus->mockAddressValue(0x03, 0x80); // ABS LO
    m_bus->mockAddressValue(0x04, 0x80); // ABS HI

    m_bus->mockAddressValue(0x05, 0xA2); // LDX
    m_bus->mockAddressValue(0x06, 0x20); // IMM

    m_bus->mockAddressValue(0x07, 0xFE); // INC
    m_bus->mockAddressValue(0x08, 0x80); // ABS LO,X
    m_bus->mockAddressValue(0x09, 0x80); // ABS HI,X

    m_bus->mockAddressValue(0x0A, 0xF6); // INC
    m_bus->mockAddressValue(0x0B, 0x80); // ZPG,X

    m_bus->mockAddressValue(0x0C, 0x00); // PAD

    m_bus->mockAddressValue(0x0080, 0xFF);
    m_bus->mockAddressValue(0x00A0, 0x01);
    m_bus->mockAddressValue(0x8080, 0x7F);
    m_bus->mockAddressValue(0x80A0, 0x40);

    REQUIRE(m_cpu.step() == 5U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_bus->readWrittenValue(0x80) == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 6U);
    REQUIRE(m_cpu.regs().pc == 5U);
    REQUIRE(m_bus->readWrittenValue(0x8080) == 0x80);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 7U);
    REQUIRE(m_cpu.regs().xi == 0x20);

    REQUIRE(m_cpu.step() == 7U);
    REQUIRE(m_cpu.regs().pc == 0x0A);
    REQUIRE(m_bus->readWrittenValue(0x80A0) == 0x41);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 6U);
    REQUIRE(m_cpu.regs().pc == 0x0C);
    REQUIRE(m_bus->readWrittenValue(0x00A0) == 0x02);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction DEC" ) {
    m_bus->mockAddressValue(0x00, 0xC6); // DEC
    m_bus->mockAddressValue(0x01, 0x80); // ZPG

    m_bus->mockAddressValue(0x02, 0xCE); // DEC
    m_bus->mockAddressValue(0x03, 0x80); // ABS LO
    m_bus->mockAddressValue(0x04, 0x80); // ABS HI

    m_bus->mockAddressValue(0x05, 0xA2); // LDX
    m_bus->mockAddressValue(0x06, 0x20); // IMM

    m_bus->mockAddressValue(0x07, 0xDE); // DEC
    m_bus->mockAddressValue(0x08, 0x80); // ABS LO,X
    m_bus->mockAddressValue(0x09, 0x80); // ABS HI,X

    m_bus->mockAddressValue(0x0A, 0xD6); // DEC
    m_bus->mockAddressValue(0x0B, 0x80); // ZPG,X

    m_bus->mockAddressValue(0x0C, 0x00); // PAD

    m_bus->mockAddressValue(0x0080, 0xFF);
    m_bus->mockAddressValue(0x00A0, 0x01);
    m_bus->mockAddressValue(0x8080, 0x7F);
    m_bus->mockAddressValue(0x80A0, 0x40);

    REQUIRE(m_cpu.step() == 5U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_bus->readWrittenValue(0x80) == 0xFE);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 6U);
    REQUIRE(m_cpu.regs().pc == 5U);
    REQUIRE(m_bus->readWrittenValue(0x8080) == 0x7E);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 7U);
    REQUIRE(m_cpu.regs().xi == 0x20);

    REQUIRE(m_cpu.step() == 7U);
    REQUIRE(m_cpu.regs().pc == 0x0A);
    REQUIRE(m_bus->readWrittenValue(0x80A0) == 0x3F);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 6U);
    REQUIRE(m_cpu.regs().pc == 0x0C);
    REQUIRE(m_bus->readWrittenValue(0x00A0) == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction DEX" ) {
    m_bus->mockAddressValue(0x00, 0xA2); // LDX
    m_bus->mockAddressValue(0x01, 0x01); // IMM

    m_bus->mockAddressValue(0x02, 0xCA); // DEX
    m_bus->mockAddressValue(0x03, 0xCA); // DEX
    m_bus->mockAddressValue(0x04, 0xCA); // DEX

    m_bus->mockAddressValue(0x05, 0x00); // PAD
    m_bus->mockAddressValue(0x06, 0x00); // PAD

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().xi == 0x01);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().xi == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().xi == 0xFF);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction DEY" ) {
    m_bus->mockAddressValue(0x00, 0xA0); // LDY
    m_bus->mockAddressValue(0x01, 0x01); // IMM

    m_bus->mockAddressValue(0x02, 0x88); // DEY
    m_bus->mockAddressValue(0x03, 0x88); // DEY
    m_bus->mockAddressValue(0x04, 0x88); // DEY

    m_bus->mockAddressValue(0x05, 0x00); // PAD
    m_bus->mockAddressValue(0x06, 0x00); // PAD

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().yi == 0x01);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().yi == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().yi == 0xFF);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction STA" ) {
    m_bus->mockAddressValue(0x00, 0x85); // STA
    m_bus->mockAddressValue(0x01, 0x20); // ZPG

    m_bus->mockAddressValue(0x02, 0x95); // STA
    m_bus->mockAddressValue(0x03, 0x40); // ZPG,X

    m_bus->mockAddressValue(0x04, 0x8D); // STA
    m_bus->mockAddressValue(0x05, 0xEF); // ABS LO
    m_bus->mockAddressValue(0x06, 0xBE); // ABS HI

    m_bus->mockAddressValue(0x07, 0x9D); // STA
    m_bus->mockAddressValue(0x08, 0xEF); // ABS LO,X
    m_bus->mockAddressValue(0x09, 0xBE); // ABS HI,X

    m_bus->mockAddressValue(0x0A, 0x99); // STA
    m_bus->mockAddressValue(0x0B, 0xEF); // ABS LO,Y
    m_bus->mockAddressValue(0x0C, 0xBE); // ABS HI,Y

    m_bus->mockAddressValue(0x0D, 0x81); // STA
    m_bus->mockAddressValue(0x0E, 0x80); // (IND,X)

    m_bus->mockAddressValue(0x0F, 0x91); // STA
    m_bus->mockAddressValue(0x10, 0x80); // (IND),Y

    m_bus->mockAddressValue(0x11, 0xEA); // NOP

    m_cpu.regs().ac = 0xAB;

    REQUIRE(m_cpu.step() == 3U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_bus->readWrittenValue(0x20) == 0xAB);

    m_cpu.regs().ac = 0x55;
    m_cpu.regs().xi = 0x20;

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_bus->readWrittenValue(0x60) == 0x55);

    m_cpu.regs().ac = 0x11;

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 7U);
    REQUIRE(m_bus->readWrittenValue(0xBEEF) == 0x11);

    m_cpu.regs().ac = 0x0F;
    m_cpu.regs().xi = 0x10;

    REQUIRE(m_cpu.step() == 5U);
    REQUIRE(m_cpu.regs().pc == 0x0A);

    REQUIRE(m_bus->readWrittenValue(0xBEEF + 0x10) == 0x0F);

    m_cpu.regs().ac = 0xF0;
    m_cpu.regs().yi = 0x30;

    REQUIRE(m_cpu.step() == 5U);
    REQUIRE(m_cpu.regs().pc == 0x0D);

    REQUIRE(m_bus->readWrittenValue(0xBEEF + 0x30) == 0xF0);

    m_cpu.regs().ac = 0xBB;
    m_cpu.regs().xi = 0x20;

    m_bus->mockAddressValue(0xA0, 0x80); // IND LO
    m_bus->mockAddressValue(0xA1, 0x80); // IND HI

    REQUIRE(m_cpu.step() == 6U);
    REQUIRE(m_cpu.regs().pc == 0x0F);

    REQUIRE(m_bus->readWrittenValue(0x8080) == 0xBB);

    m_bus->mockAddressValue(0x80, 0x40); // IND LO
    m_bus->mockAddressValue(0x81, 0x90); // IND HI

    m_cpu.regs().ac = 0xCC;
    m_cpu.regs().yi = 0x10;

    REQUIRE(m_cpu.step() == 6U);
    REQUIRE(m_cpu.regs().pc == 0x11);

    REQUIRE(m_bus->readWrittenValue(0x9050) == 0xCC);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction STX" ) {
    m_bus->mockAddressValue(0x00, 0xA2); // LDX
    m_bus->mockAddressValue(0x01, 0x01); // IMM

    m_bus->mockAddressValue(0x02, 0x86); // STX
    m_bus->mockAddressValue(0x03, 0x80); // ZPG

    m_bus->mockAddressValue(0x04, 0xA0); // LDY
    m_bus->mockAddressValue(0x05, 0x01); // IMM

    m_bus->mockAddressValue(0x06, 0x96); // STX
    m_bus->mockAddressValue(0x07, 0x80); // ZPG,Y

    m_bus->mockAddressValue(0x08, 0x8E); // STX
    m_bus->mockAddressValue(0x09, 0x82); // ABS LO
    m_bus->mockAddressValue(0x0A, 0x40); // ABS HI

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().xi == 0x01);
    REQUIRE(m_cpu.regs().yi == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 3U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().xi == 0x01);
    REQUIRE(m_cpu.regs().yi == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_bus->readWrittenValue(0x0080) == 0x01);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().xi == 0x01);
    REQUIRE(m_cpu.regs().yi == 0x01);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 8U);
    REQUIRE(m_cpu.regs().xi == 0x01);
    REQUIRE(m_cpu.regs().yi == 0x01);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_bus->readWrittenValue(0x0081) == 0x01);

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 0x0B);
    REQUIRE(m_cpu.regs().xi == 0x01);
    REQUIRE(m_cpu.regs().yi == 0x01);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_bus->readWrittenValue(0x4082) == 0x01);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction STY" ) {
    m_bus->mockAddressValue(0x00, 0xA0); // LDY
    m_bus->mockAddressValue(0x01, 0x01); // IMM

    m_bus->mockAddressValue(0x02, 0x84); // STY
    m_bus->mockAddressValue(0x03, 0x80); // ZPG

    m_bus->mockAddressValue(0x04, 0xA2); // LDX
    m_bus->mockAddressValue(0x05, 0x01); // IMM

    m_bus->mockAddressValue(0x06, 0x94); // STY
    m_bus->mockAddressValue(0x07, 0x80); // ZPG,Y

    m_bus->mockAddressValue(0x08, 0x8C); // STY
    m_bus->mockAddressValue(0x09, 0x82); // ABS LO
    m_bus->mockAddressValue(0x0A, 0x40); // ABS HI

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().xi == 0x00);
    REQUIRE(m_cpu.regs().yi == 0x01);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 3U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().xi == 0x00);
    REQUIRE(m_cpu.regs().yi == 0x01);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_bus->readWrittenValue(0x0080) == 0x01);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().xi == 0x01);
    REQUIRE(m_cpu.regs().yi == 0x01);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 8U);
    REQUIRE(m_cpu.regs().xi == 0x01);
    REQUIRE(m_cpu.regs().yi == 0x01);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_bus->readWrittenValue(0x0081) == 0x01);

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 0x0B);
    REQUIRE(m_cpu.regs().xi == 0x01);
    REQUIRE(m_cpu.regs().yi == 0x01);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(m_bus->readWrittenValue(0x4082) == 0x01);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction TAX" ) {
    m_bus->mockAddressValue(0x00, 0xA9); // LDA
    m_bus->mockAddressValue(0x01, 0x80); // IMM

    m_bus->mockAddressValue(0x02, 0xAA); // TAX
    m_bus->mockAddressValue(0x03, 0xAA); // TAX
    m_bus->mockAddressValue(0x04, 0xAA); // TAX

    m_bus->mockAddressValue(0x05, 0xEA); // NOP
    m_bus->mockAddressValue(0x06, 0xEA); // NOP

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().xi == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    m_cpu.regs().sr = (mos6502::U | mos6502::B);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().xi == 0x80);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    m_cpu.regs().ac = 0x00;
    m_cpu.regs().sr = (mos6502::U | mos6502::B);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().xi == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    m_cpu.regs().ac = 0x7F;
    m_cpu.regs().sr = mos6502::Z | mos6502::N | mos6502::U | mos6502::B;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 5U);
    REQUIRE(m_cpu.regs().ac == 0x7F);
    REQUIRE(m_cpu.regs().xi == 0x07F);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction TAY" ) {
    m_bus->mockAddressValue(0x00, 0xA9); // LDA
    m_bus->mockAddressValue(0x01, 0x80); // IMM

    m_bus->mockAddressValue(0x02, 0xA8); // TAY
    m_bus->mockAddressValue(0x03, 0xA8); // TAY
    m_bus->mockAddressValue(0x04, 0xA8); // TAY

    m_bus->mockAddressValue(0x05, 0xEA); // NOP
    m_bus->mockAddressValue(0x06, 0xEA); // NOP

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().yi == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    m_cpu.regs().sr = (mos6502::U | mos6502::B);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().yi == 0x80);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    m_cpu.regs().ac = 0x00;
    m_cpu.regs().sr = (mos6502::U | mos6502::B);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().yi == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    m_cpu.regs().ac = 0x7F;
    m_cpu.regs().sr = (mos6502::Z | mos6502::N | mos6502::U | mos6502::B);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 5U);
    REQUIRE(m_cpu.regs().ac == 0x7F);
    REQUIRE(m_cpu.regs().yi == 0x07F);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction TSX" ) {
    m_bus->mockAddressValue(0x00, 0xBA); // TSX
    m_bus->mockAddressValue(0x01, 0xBA); // TSX
    m_bus->mockAddressValue(0x02, 0xBA); // TSX

    m_bus->mockAddressValue(0x03, 0xEA); // NOP
    m_bus->mockAddressValue(0x04, 0xEA); // NOP

    m_cpu.regs().sp = 0x01FF;
    m_cpu.regs().sr = (mos6502::U | mos6502::B);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().sp == 0x01FF);
    REQUIRE(m_cpu.regs().xi == 0xFF);
    REQUIRE(m_cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    m_cpu.regs().sp = 0x0100;
    m_cpu.regs().sr = (mos6502::U | mos6502::B);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().sp == 0x0100);
    REQUIRE(m_cpu.regs().xi == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    m_cpu.regs().sp = 0x017F;
    m_cpu.regs().sr = (mos6502::N | mos6502::Z | mos6502::U | mos6502::B);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().sp == 0x017F);
    REQUIRE(m_cpu.regs().xi == 0x7F);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction TXA" ) {
    m_bus->mockAddressValue(0x00, 0x8A); // TXA
    m_bus->mockAddressValue(0x01, 0x8A); // TXA
    m_bus->mockAddressValue(0x02, 0x8A); // TXA

    m_bus->mockAddressValue(0x03, 0xEA); // NOP
    m_bus->mockAddressValue(0x04, 0xEA); // NOP

    m_cpu.regs().xi = 0x01;
    m_cpu.regs().sr = mos6502::N | mos6502::Z;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().ac == 0x01);
    REQUIRE(m_cpu.regs().xi == 0x01);
    REQUIRE(m_cpu.regs().sr == 0x00);

    m_cpu.regs().xi = 0x00;
    m_cpu.regs().sr = 0x00;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().xi == 0x00);
    REQUIRE(m_cpu.regs().sr == mos6502::Z);

    m_cpu.regs().xi = 0x80;
    m_cpu.regs().sr = mos6502::N;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().xi == 0x80);
    REQUIRE(m_cpu.regs().sr == mos6502::N);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction TXS" ) {
    m_bus->mockAddressValue(0x00, 0x9A); // TXS
    m_bus->mockAddressValue(0x01, 0x9A); // TXS
    m_bus->mockAddressValue(0x02, 0x9A); // TXS

    m_bus->mockAddressValue(0x03, 0xEA); // NOP
    m_bus->mockAddressValue(0x04, 0xEA); // NOP

    m_cpu.regs().xi = 0x80;
    m_cpu.regs().sp = 0x0100;
    m_cpu.regs().sr = 0;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().sp == 0x0180);
    REQUIRE(m_cpu.regs().xi == 0x80);
    REQUIRE(m_cpu.regs().sr == mos6502::N);

    m_cpu.regs().xi = 0x7E;
    m_cpu.regs().sp = 0x02FF;
    m_cpu.regs().sr = mos6502::N | mos6502::Z;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().sp == 0x027E);
    REQUIRE(m_cpu.regs().xi == 0x7E);
    REQUIRE(m_cpu.regs().sr == 0x00);

    m_cpu.regs().xi = 0x00;
    m_cpu.regs().sr = 0x00;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().sp == 0x0200);
    REQUIRE(m_cpu.regs().xi == 0x00);
    REQUIRE(m_cpu.regs().sr == mos6502::Z);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction TYA" ) {
    m_bus->mockAddressValue(0x00, 0x98); // TYA
    m_bus->mockAddressValue(0x01, 0x98); // TYA
    m_bus->mockAddressValue(0x02, 0x98); // TYA

    m_bus->mockAddressValue(0x03, 0xEA); // NOP
    m_bus->mockAddressValue(0x04, 0xEA); // NOP

    m_cpu.regs().yi = 0x01;
    m_cpu.regs().sr = mos6502::N | mos6502::Z;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().ac == 0x01);
    REQUIRE(m_cpu.regs().yi == 0x01);
    REQUIRE(m_cpu.regs().sr == 0x00);

    m_cpu.regs().yi = 0x00;
    m_cpu.regs().sr = 0x00;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().yi == 0x00);
    REQUIRE(m_cpu.regs().sr == mos6502::Z);

    m_cpu.regs().yi = 0x80;
    m_cpu.regs().sr = mos6502::N;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().yi == 0x80);
    REQUIRE(m_cpu.regs().sr == mos6502::N);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction ASL" ) {
    m_bus->mockAddressValue(0x00, 0x0A); // ASL
    m_bus->mockAddressValue(0x01, 0x0A); // ASL
    m_bus->mockAddressValue(0x02, 0x0A); // ASL
    m_bus->mockAddressValue(0x03, 0x0A); // ASL
    m_bus->mockAddressValue(0x04, 0x0A); // ASL
    m_bus->mockAddressValue(0x05, 0x0A); // ASL
    m_bus->mockAddressValue(0x06, 0x0A); // ASL
    m_bus->mockAddressValue(0x07, 0x0A); // ASL
    m_bus->mockAddressValue(0x08, 0xEA); // NOP
    m_bus->mockAddressValue(0x09, 0xEA); // NOP

    m_cpu.regs().ac = 0x01;
    m_cpu.regs().sr = mos6502::C;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().ac == 0x02);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0x04);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().ac == 0x08);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().ac == 0x10);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 5U);
    REQUIRE(m_cpu.regs().ac == 0x20);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().ac == 0x40);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 7U);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().sr == mos6502::N);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 8U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::C | mos6502::Z));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction LSR" ) {
    m_bus->mockAddressValue(0x00, 0x4A); // LSR
    m_bus->mockAddressValue(0x01, 0x4A); // LSR
    m_bus->mockAddressValue(0x02, 0x4A); // LSR
    m_bus->mockAddressValue(0x03, 0x4A); // LSR
    m_bus->mockAddressValue(0x04, 0x4A); // LSR
    m_bus->mockAddressValue(0x05, 0x4A); // LSR
    m_bus->mockAddressValue(0x06, 0x4A); // LSR
    m_bus->mockAddressValue(0x07, 0x4A); // LSR
    m_bus->mockAddressValue(0x08, 0xEA); // NOP
    m_bus->mockAddressValue(0x09, 0xEA); // NOP

    m_cpu.regs().ac = 0x80;
    m_cpu.regs().sr = mos6502::C;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().ac == 0x40);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0x20);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().ac == 0x10);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().ac == 0x08);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 5U);
    REQUIRE(m_cpu.regs().ac == 0x04);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().ac == 0x02);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 7U);
    REQUIRE(m_cpu.regs().ac == 0x01);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 8U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::C | mos6502::Z));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction Test ROL" ) {
    m_bus->mockAddressValue(0x00, 0x2A); // ROL
    m_bus->mockAddressValue(0x01, 0x2A); // ROL
    m_bus->mockAddressValue(0x02, 0x2A); // ROL
    m_bus->mockAddressValue(0x03, 0x2A); // ROL
    m_bus->mockAddressValue(0x04, 0x2A); // ROL
    m_bus->mockAddressValue(0x05, 0x2A); // ROL
    m_bus->mockAddressValue(0x06, 0x2A); // ROL
    m_bus->mockAddressValue(0x07, 0x2A); // ROL
    m_bus->mockAddressValue(0x08, 0xEA); // NOP
    m_bus->mockAddressValue(0x09, 0xEA); // NOP

    m_cpu.regs().ac = 0x01;
    m_cpu.regs().sr = mos6502::C;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().ac == 0x03);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0x06);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().ac == 0x0C);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().ac == 0x18);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 5U);
    REQUIRE(m_cpu.regs().ac == 0x30);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().ac == 0x60);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 7U);
    REQUIRE(m_cpu.regs().ac == 0xC0);
    REQUIRE(m_cpu.regs().sr == mos6502::N);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 8U);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().sr == (mos6502::C | mos6502::N));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction ROR" ) {
    m_bus->mockAddressValue(0x00, 0x6A); // ROR
    m_bus->mockAddressValue(0x01, 0x6A); // ROR
    m_bus->mockAddressValue(0x02, 0x6A); // ROR
    m_bus->mockAddressValue(0x03, 0x6A); // ROR
    m_bus->mockAddressValue(0x04, 0x6A); // ROR
    m_bus->mockAddressValue(0x05, 0x6A); // ROR
    m_bus->mockAddressValue(0x06, 0x6A); // ROR
    m_bus->mockAddressValue(0x07, 0x6A); // ROR
    m_bus->mockAddressValue(0x08, 0x6A); // ROR
    m_bus->mockAddressValue(0x09, 0xEA); // NOP
    m_bus->mockAddressValue(0x0A, 0xEA); // NOP

    m_cpu.regs().ac = 0x01;
    m_cpu.regs().sr = mos6502::C;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().sr == (mos6502::C | mos6502::N));

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0xC0);
    REQUIRE(m_cpu.regs().sr == mos6502::N);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().ac == 0x60);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 4U);
    REQUIRE(m_cpu.regs().ac == 0x30);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 5U);
    REQUIRE(m_cpu.regs().ac == 0x18);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 6U);
    REQUIRE(m_cpu.regs().ac == 0x0C);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 7U);
    REQUIRE(m_cpu.regs().ac == 0x06);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 8U);
    REQUIRE(m_cpu.regs().ac == 0x03);
    REQUIRE(m_cpu.regs().sr == 0x00);

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 9U);
    REQUIRE(m_cpu.regs().ac == 0x01);
    REQUIRE(m_cpu.regs().sr == mos6502::C);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction NOP" ) {
    m_bus->mockAddressValue(0x00, 0xEA); // NOP
    m_bus->mockAddressValue(0x01, 0xEA); // NOP
    m_bus->mockAddressValue(0x02, 0xEA); // NOP

    auto regs = m_cpu.regs();
    regs.pc = 0x01;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(regs == m_cpu.regs());
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction PHA" ) {
    m_bus->mockAddressValue(0x00, 0x48); // PHA
    m_bus->mockAddressValue(0x01, 0xEA); // NOP
    m_bus->mockAddressValue(0x02, 0xEA); // NOP

    REQUIRE(m_cpu.step() == 3U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().sp == 0x1FE);
    REQUIRE(m_bus->readWrittenValue(0x1FF) == 0x00);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction PHP" ) {
    m_bus->mockAddressValue(0x00, 0x08); // PHP
    m_bus->mockAddressValue(0x01, 0xEA); // NOP
    m_bus->mockAddressValue(0x02, 0xEA); // NOP

    REQUIRE(m_cpu.step() == 3U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().sp == 0x1FE);
    REQUIRE(m_bus->readWrittenValue(0x1FF) == (mos6502::U | mos6502::B));
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction PLA" ) {
    m_bus->mockAddressValue(0x00, 0x68); // PLA
    m_bus->mockAddressValue(0x01, 0x68); // PLA
    m_bus->mockAddressValue(0x02, 0x68); // PLA

    m_bus->mockAddressValue(0x03, 0xEA); // NOP
    m_bus->mockAddressValue(0x04, 0xEA); // NOP

    m_cpu.regs().sp = 0x1FC;
    m_bus->mockAddressValue(0x1FD, 0x00);
    m_bus->mockAddressValue(0x1FE, 0x80);
    m_bus->mockAddressValue(0x1FF, 0x7F);

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::Z));
    REQUIRE(m_cpu.regs().sp == 0x1FD);

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().ac == 0x80);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::N));
    REQUIRE(m_cpu.regs().sp == 0x1FE);

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().ac == 0x7F);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));
    REQUIRE(m_cpu.regs().sp == 0x1FF);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction PLP" ) {
    m_bus->mockAddressValue(0x00, 0x28); // PLP
    m_bus->mockAddressValue(0x01, 0x28); // PLP
    m_bus->mockAddressValue(0x02, 0x28); // PLP

    m_bus->mockAddressValue(0x03, 0xEA); // NOP
    m_bus->mockAddressValue(0x04, 0xEA); // NOP

    m_cpu.regs().sp = 0x1FC;
    m_bus->mockAddressValue(0x1FD, 0xFF);
    m_bus->mockAddressValue(0x1FE, 0x00);
    m_bus->mockAddressValue(0x1FF, 0x0F);

    REQUIRE(m_cpu.step() == 4U);

    REQUIRE(m_cpu.regs().pc == 1U);
    REQUIRE(m_cpu.regs().ac == 0x00);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::Z | mos6502::N | mos6502::C | mos6502::D | mos6502::V | mos6502::I));
    REQUIRE(m_cpu.regs().sp == 0x1FD);

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 2U);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));
    REQUIRE(m_cpu.regs().sp == 0x1FE);

    REQUIRE(m_cpu.step() == 4U);
    REQUIRE(m_cpu.regs().pc == 3U);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::D | mos6502::I | mos6502::Z | mos6502::C));
    REQUIRE(m_cpu.regs().sp == 0x1FF);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction BRK,RTI" ) {
    m_bus->mockAddressValue(0x00, 0x00); // BRK
    m_bus->mockAddressValue(0x01, 0xFF); // #MARK
    m_bus->mockAddressValue(0x02, 0xEA); // NOP
    m_bus->mockAddressValue(0x03, 0xEA); // NOP
    m_bus->mockAddressValue(0x04, 0xEA); // NOP

    m_bus->mockAddressValue(0xBEEF, 0x40); // RTI
    m_bus->mockAddressValue(0xBEF0, 0xEA); // NOP
    m_bus->mockAddressValue(0xBEF1, 0xEA); // NOP

    m_bus->mockAddressValue(0xFFFE, 0xEF); // LO
    m_bus->mockAddressValue(0xFFFF, 0xBE); // HI

    REQUIRE(m_cpu.step() == 7U);
    REQUIRE(m_cpu.regs().pc == 0xBEEF);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::I));
    REQUIRE(m_cpu.regs().sp == 0x1FC);

    m_bus->mockAddressValue(0x1FF, m_bus->readWrittenValue(0x1FF));
    m_bus->mockAddressValue(0x1FE, m_bus->readWrittenValue(0x1FE));
    m_bus->mockAddressValue(0x1FD, m_bus->readWrittenValue(0x1FD));

    REQUIRE(m_cpu.step() == 6U);
    REQUIRE(m_cpu.regs().pc == 0x02);
    REQUIRE(m_cpu.regs().sr == (mos6502::U | mos6502::B));
    REQUIRE(m_cpu.regs().sp == 0x1FF);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction JSR,RTS" ) {
    m_bus->mockAddressValue(0x00, 0x20); // JSR
    m_bus->mockAddressValue(0x01, 0x40); // ABS,LO
    m_bus->mockAddressValue(0x02, 0x80); // ABS,HI

    m_bus->mockAddressValue(0x03, 0xEA); // NOP
    m_bus->mockAddressValue(0x04, 0xEA); // NOP
    m_bus->mockAddressValue(0x05, 0xEA); // NOP

    m_bus->mockAddressValue(0x8040, 0x60); // RTS
    m_bus->mockAddressValue(0x8041, 0xEA); // NOP
    m_bus->mockAddressValue(0x8042, 0xEA); // NOP

    REQUIRE(m_cpu.step() == 6U);
    REQUIRE(m_cpu.regs().pc == 0x8040);

    m_bus->mockAddressValue(0x1FF, m_bus->readWrittenValue(0x1FF));
    m_bus->mockAddressValue(0x1FE, m_bus->readWrittenValue(0x1FE));

    REQUIRE(m_cpu.step() == 6U);
    REQUIRE(m_cpu.regs().pc == 0x03);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction JMP" ) {
    m_bus->mockAddressValue(0x00, 0x4C); // JSR
    m_bus->mockAddressValue(0x01, 0xEF); // ABS,LO
    m_bus->mockAddressValue(0x02, 0xBE); // ABS,HI

    m_bus->mockAddressValue(0xBEEF, 0x6C); // JSR
    m_bus->mockAddressValue(0xBEF0, 0xAB); // IND,LO
    m_bus->mockAddressValue(0xBEF1, 0xCA); // IND,HI

    m_bus->mockAddressValue(0xCAAB, 0x10); // ABS,LO
    m_bus->mockAddressValue(0xCAAC, 0x20); // ABS,HI

    REQUIRE(m_cpu.step() == 3U);
    REQUIRE(m_cpu.regs().pc == 0xBEEF);

    REQUIRE(m_cpu.step() == 5U);
    REQUIRE(m_cpu.regs().pc == 0x2010);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction BCC,BCS" ) {
    m_bus->mockAddressValue(0x00, 0xB0); // BCS
    m_bus->mockAddressValue(0x01, 0x01); // REL (+1)
    m_bus->mockAddressValue(0x02, 0xEA); // NOP

    m_bus->mockAddressValue(0x03, 0x90); // BCC
    m_bus->mockAddressValue(0x04, 0xFD); // REL (-3)
    m_bus->mockAddressValue(0x05, 0xEA); // NOP

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);

    m_cpu.regs().sr |= mos6502::C;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 5U);

    m_cpu.regs().pc = 0x00;
    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);

    m_cpu.regs().sr &= ~mos6502::C;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction BEQ,BNE" ) {
    m_bus->mockAddressValue(0x00, 0xF0); // BEQ
    m_bus->mockAddressValue(0x01, 0x01); // REL (+1)
    m_bus->mockAddressValue(0x02, 0xEA); // NOP

    m_bus->mockAddressValue(0x03, 0xD0); // BNE
    m_bus->mockAddressValue(0x04, 0xFD); // REL (-3)
    m_bus->mockAddressValue(0x05, 0xEA); // NOP

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);

    m_cpu.regs().sr |= mos6502::Z;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 5U);

    m_cpu.regs().pc = 0x00;
    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);

    m_cpu.regs().sr &= ~mos6502::Z;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction BMI,BPL" ) {
    m_bus->mockAddressValue(0x00, 0x30); // BMI
    m_bus->mockAddressValue(0x01, 0x01); // REL (+1)
    m_bus->mockAddressValue(0x02, 0xEA); // NOP

    m_bus->mockAddressValue(0x03, 0x10); // BPL
    m_bus->mockAddressValue(0x04, 0xFD); // REL (-3)
    m_bus->mockAddressValue(0x05, 0xEA); // NOP

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);

    m_cpu.regs().sr |= mos6502::N;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 5U);

    m_cpu.regs().pc = 0x00;
    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);

    m_cpu.regs().sr &= ~mos6502::N;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
}

TEST_CASE_FIXTURE(CpuFixture, "Instruction BVS,BVC" ) {
    m_bus->mockAddressValue(0x00, 0x70); // BVS
    m_bus->mockAddressValue(0x01, 0x01); // REL (+1)
    m_bus->mockAddressValue(0x02, 0xEA); // NOP

    m_bus->mockAddressValue(0x03, 0x50); // BVC
    m_bus->mockAddressValue(0x04, 0xFD); // REL (-3)
    m_bus->mockAddressValue(0x05, 0xEA); // NOP

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);

    m_cpu.regs().sr |= mos6502::V;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 5U);

    m_cpu.regs().pc = 0x00;
    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 3U);

    m_cpu.regs().sr &= ~mos6502::V;

    REQUIRE(m_cpu.step() == 2U);
    REQUIRE(m_cpu.regs().pc == 2U);
}
