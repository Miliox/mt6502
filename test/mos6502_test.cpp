#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "mos6502/bus.hpp"
#include "mos6502/cpu.hpp"
#include "mos6502/regs.hpp"
#include "mos6502/status.hpp"

class MockBus final : public mos6502::IBus {
public:
    MockBus();

    ~MockBus() override;

    std::uint8_t read(std::uint16_t addr) override {
        return mem_map.at(addr);
    }

    void write(std::uint16_t addr, std::uint8_t data) override {
        static_cast<void>(addr);
        static_cast<void>(data);
    }

    void mockAddressValue(std::uint16_t addr, std::uint8_t data) {
        mem_map.insert_or_assign(addr, data);
    }

private:
    std::unordered_map<std::uint16_t, std::uint8_t> mem_map{};
};

MockBus::MockBus() = default;

MockBus::~MockBus() = default;

class CpuFixture {
public:
    CpuFixture();

protected:
    std::shared_ptr<MockBus> mock_bus{new MockBus{}};
    mos6502::Cpu cpu{mock_bus};
};

CpuFixture::CpuFixture() {
    mock_bus->mockAddressValue(0x01, 0x00);
    mock_bus->mockAddressValue(0x02, 0x00);

    cpu.regs().sp = 0x100;
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[SEC]") {
    mock_bus->mockAddressValue(0x00, 0x38);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().sr == mos6502::Status::C);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[SED]" ) {
    mock_bus->mockAddressValue(0x00, 0xF8);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().sr == mos6502::Status::D);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[SEI]" ) {
    mock_bus->mockAddressValue(0x00, 0x78);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().sr == mos6502::Status::I);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[CLC]" ) {
    mock_bus->mockAddressValue(0x00, 0x18);

    cpu.regs().sr = 0xFF;
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE((cpu.regs().sr & mos6502::Status::C) == 0);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[CLD]" ) {
    mock_bus->mockAddressValue(0x00, 0xD8);

    cpu.regs().sr = 0xFF;
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE((cpu.regs().sr & mos6502::Status::D) == 0);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[CLI]" ) {
    mock_bus->mockAddressValue(0x00, 0x58);

    cpu.regs().sr = 0xFF;
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE((cpu.regs().sr & mos6502::Status::I) == 0);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[CLV]" ) {
    mock_bus->mockAddressValue(0x00, 0xB8);

    cpu.regs().sr = 0xFF;
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE((cpu.regs().sr & mos6502::Status::V) == 0);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[LDA,LDX,LDY]" ) {
    mock_bus->mockAddressValue(0x00, 0xA9); // LDA
    mock_bus->mockAddressValue(0x01, 0x80); // IMM

    mock_bus->mockAddressValue(0x02, 0xAD); // LDA
    mock_bus->mockAddressValue(0x03, 0xEF); // ABS LO
    mock_bus->mockAddressValue(0x04, 0xBE); // ABS HI

    mock_bus->mockAddressValue(0x05, 0xA5); // LDA
    mock_bus->mockAddressValue(0x06, 0x80); // ZPG

    mock_bus->mockAddressValue(0x07, 0xA2); // LDX
    mock_bus->mockAddressValue(0x08, 0x10); // IMM

    mock_bus->mockAddressValue(0x09, 0xB5); // LDA
    mock_bus->mockAddressValue(0x0A, 0x71); // ZPG,X

    mock_bus->mockAddressValue(0x0B, 0xBD); // LDA
    mock_bus->mockAddressValue(0x0C, 0xDF); // ABS LO,X
    mock_bus->mockAddressValue(0x0D, 0xBE); // ABS HI,X

    mock_bus->mockAddressValue(0x0E, 0xA0); // LDY
    mock_bus->mockAddressValue(0x0F, 0x02); // IMM

    mock_bus->mockAddressValue(0x10, 0xB9); // LDA
    mock_bus->mockAddressValue(0x11, 0x04); // ABS LO,Y
    mock_bus->mockAddressValue(0x12, 0x00); // ABS HI,Y

    mock_bus->mockAddressValue(0x13, 0xA1); // LDA
    mock_bus->mockAddressValue(0x14, 0xF3); // (IND,X)

    mock_bus->mockAddressValue(0x15, 0xB1); // LDA
    mock_bus->mockAddressValue(0x16, 0xFF); // (IND),Y

    mock_bus->mockAddressValue(0x17, 0xA6); // LDX
    mock_bus->mockAddressValue(0x18, 0x00); // ZPG

    mock_bus->mockAddressValue(0x19, 0xB6); // LDX
    mock_bus->mockAddressValue(0x1A, 0x00); // ZPG,Y

    mock_bus->mockAddressValue(0x1B, 0xAE); // LDX
    mock_bus->mockAddressValue(0x1C, 0xEF); // ABS LO
    mock_bus->mockAddressValue(0x1D, 0xBE); // ABS HI

    mock_bus->mockAddressValue(0x1E, 0xBE); // LDX
    mock_bus->mockAddressValue(0x1F, 0x00); // ABS LO,Y
    mock_bus->mockAddressValue(0x20, 0xA9); // ABS HI,Y

    mock_bus->mockAddressValue(0x21, 0xA4); // LDY
    mock_bus->mockAddressValue(0x22, 0x80); // ZPG

    mock_bus->mockAddressValue(0x23, 0xB4); // LDY
    mock_bus->mockAddressValue(0x24, 0x41); // ZPG,X

    mock_bus->mockAddressValue(0x25, 0xAC); // LDY
    mock_bus->mockAddressValue(0x26, 0xEF); // ABS LO
    mock_bus->mockAddressValue(0x27, 0xBE); // ABS HI

    mock_bus->mockAddressValue(0x28, 0xBC); // LDY
    mock_bus->mockAddressValue(0x29, 0x40); // ABS LO,X
    mock_bus->mockAddressValue(0x2A, 0x00); // ABS HI,X

    mock_bus->mockAddressValue(0x0080, 0xFF);
    mock_bus->mockAddressValue(0x0081, 0x7F);
    mock_bus->mockAddressValue(0x00FF, 0x00);
    mock_bus->mockAddressValue(0x0100, 0x10);
    mock_bus->mockAddressValue(0xBEEF, 0x00);
    mock_bus->mockAddressValue(0x1000, 0x3F);
    mock_bus->mockAddressValue(0xA902, 0x40);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 0x02);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().sr == mos6502::Status::N);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x05);
    REQUIRE(cpu.regs().ac == 0U);
    REQUIRE(cpu.regs().sr == mos6502::Status::Z);

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 0x07);
    REQUIRE(cpu.regs().ac == 0xFF);
    REQUIRE(cpu.regs().sr == mos6502::Status::N);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 0x09);
    REQUIRE(cpu.regs().xi == 0x10);
    REQUIRE(cpu.regs().sr == 0U);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x0B);
    REQUIRE(cpu.regs().ac == 0x7F);
    REQUIRE(cpu.regs().sr == 0U);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x0E);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == mos6502::Status::Z);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 0x10);
    REQUIRE(cpu.regs().yi == 2U);
    REQUIRE(cpu.regs().sr == 0U);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x13);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().sr == mos6502::Status::N);

    REQUIRE(cpu.step() == 6U);
    REQUIRE(cpu.regs().pc == 0x15);
    REQUIRE(cpu.regs().ac == 0U);
    REQUIRE(cpu.regs().sr == mos6502::Status::Z);

    REQUIRE(cpu.step() == 5U);
    REQUIRE(cpu.regs().pc == 0x17);
    REQUIRE(cpu.regs().ac == 0x40);
    REQUIRE(cpu.regs().sr == 0U);

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 0x19);
    REQUIRE(cpu.regs().xi == 0xA9);
    REQUIRE(cpu.regs().sr == mos6502::Status::N);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x1B);
    REQUIRE(cpu.regs().xi == 0xAD);
    REQUIRE(cpu.regs().sr == mos6502::Status::N);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x1E);
    REQUIRE(cpu.regs().xi == 0x00);
    REQUIRE(cpu.regs().sr == mos6502::Status::Z);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x21);
    REQUIRE(cpu.regs().xi == 0x40);
    REQUIRE(cpu.regs().sr == 0U);

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 0x23);
    REQUIRE(cpu.regs().yi == 0xFF);
    REQUIRE(cpu.regs().sr == mos6502::Status::N);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x25);
    REQUIRE(cpu.regs().yi == 0x7F);
    REQUIRE(cpu.regs().sr == 0);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x28);
    REQUIRE(cpu.regs().yi == 0x00);
    REQUIRE(cpu.regs().sr == mos6502::Status::Z);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x2B);
    REQUIRE(cpu.regs().yi == 0xFF);
    REQUIRE(cpu.regs().sr == mos6502::Status::N);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[ADC]" ) {
    mock_bus->mockAddressValue(0x00, 0xA9); // LDA
    mock_bus->mockAddressValue(0x01, 0x50); // IMM

    mock_bus->mockAddressValue(0x02, 0x69); // ADC
    mock_bus->mockAddressValue(0x03, 0x10); // IMM

    mock_bus->mockAddressValue(0x04, 0x69); // ADC
    mock_bus->mockAddressValue(0x05, 0x20); // IMM

    mock_bus->mockAddressValue(0x06, 0x69); // ADC
    mock_bus->mockAddressValue(0x07, 0x80); // IMM

    mock_bus->mockAddressValue(0x08, 0x18);

    mock_bus->mockAddressValue(0x09, 0x69); // ADC
    mock_bus->mockAddressValue(0x0A, 0x00); // IMM

    mock_bus->mockAddressValue(0x0B, 0x00); // PAD

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0x50);
    REQUIRE(cpu.regs().sr == 0U);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().ac == 0x60);
    REQUIRE(cpu.regs().sr == 0U);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().sr == (mos6502::Status::N | mos6502::Status::V));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 8U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Status::V | mos6502::Status::Z | mos6502::Status::C));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 9U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Status::V | mos6502::Status::Z));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 0x0B);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == mos6502::Status::Z);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[SBC]" ) {
    mock_bus->mockAddressValue(0x00, 0xE9); // SBC
    mock_bus->mockAddressValue(0x01, 0x00); // IMM

    mock_bus->mockAddressValue(0x02, 0x38); // SEC (No Borrow)

    mock_bus->mockAddressValue(0x03, 0xE9); // SBC
    mock_bus->mockAddressValue(0x04, 0x80); // IMM

    mock_bus->mockAddressValue(0x05, 0x18); // CLC (Borrow)

    mock_bus->mockAddressValue(0x06, 0xE9); // SBC
    mock_bus->mockAddressValue(0x07, 0x7E); // IMM

    mock_bus->mockAddressValue(0x08, 0x00); // PAD

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::Status::N));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().ac == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::Status::N | mos6502::Status::C));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 5U);
    REQUIRE(cpu.regs().ac == 0x7F);
    REQUIRE(cpu.regs().sr == mos6502::Status::C);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().ac == 0x7F);
    REQUIRE(cpu.regs().sr == 0);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 8U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Status::Z | mos6502::Status::C));
}
