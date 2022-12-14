#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <thread>

#include "mos6502/bus.hpp"
#include "mos6502/clock_sync.hpp"
#include "mos6502/cpu.hpp"
#include "mos6502/regs.hpp"
#include "mos6502/status.hpp"

class MockBus final : public mos6502::IBus {
public:
    MockBus();

    ~MockBus() override;

    std::uint8_t read(std::uint16_t addr) override {
        try {
            return read_map.at(addr);
        } catch(...) {
            std::stringstream ss{};
            ss << "Error: no data mapped at " << std::uppercase << std::hex << std::setw(2) << addr;
            throw ss.str();
        }
    }

    void write(std::uint16_t addr, std::uint8_t data) override {
        write_map.insert_or_assign(addr, data);
    }

    void mockAddressValue(std::uint16_t addr, std::uint8_t data) {
        read_map.insert_or_assign(addr, data);
    }

    std::uint8_t readWrittenValue(std::uint16_t addr) {
        return write_map.at(addr);
    }

private:
    std::unordered_map<std::uint16_t, std::uint8_t> read_map{};
    std::unordered_map<std::uint16_t, std::uint8_t> write_map{};
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
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[SEC]") {
    mock_bus->mockAddressValue(0x00, 0x38);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().sr == (mos6502::C | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[SED]" ) {
    mock_bus->mockAddressValue(0x00, 0xF8);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().sr == (mos6502::D | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[SEI]" ) {
    mock_bus->mockAddressValue(0x00, 0x78);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().sr == (mos6502::I | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[CLC]" ) {
    mock_bus->mockAddressValue(0x00, 0x18);

    cpu.regs().sr = 0xFF;
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE((cpu.regs().sr & mos6502::C) == 0);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[CLD]" ) {
    mock_bus->mockAddressValue(0x00, 0xD8);

    cpu.regs().sr = 0xFF;
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE((cpu.regs().sr & mos6502::D) == 0);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[CLI]" ) {
    mock_bus->mockAddressValue(0x00, 0x58);

    cpu.regs().sr = 0xFF;
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE((cpu.regs().sr & mos6502::I) == 0);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[CLV]" ) {
    mock_bus->mockAddressValue(0x00, 0xB8);

    cpu.regs().sr = 0xFF;
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE((cpu.regs().sr & mos6502::V) == 0);
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
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x05);
    REQUIRE(cpu.regs().ac == 0U);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 0x07);
    REQUIRE(cpu.regs().ac == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 0x09);
    REQUIRE(cpu.regs().xi == 0x10);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x0B);
    REQUIRE(cpu.regs().ac == 0x7F);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x0E);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 0x10);
    REQUIRE(cpu.regs().yi == 2U);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x13);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 6U);
    REQUIRE(cpu.regs().pc == 0x15);
    REQUIRE(cpu.regs().ac == 0U);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 5U);
    REQUIRE(cpu.regs().pc == 0x17);
    REQUIRE(cpu.regs().ac == 0x40);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 0x19);
    REQUIRE(cpu.regs().xi == 0xA9);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x1B);
    REQUIRE(cpu.regs().xi == 0xAD);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x1E);
    REQUIRE(cpu.regs().xi == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x21);
    REQUIRE(cpu.regs().xi == 0x40);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 0x23);
    REQUIRE(cpu.regs().yi == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x25);
    REQUIRE(cpu.regs().yi == 0x7F);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x28);
    REQUIRE(cpu.regs().yi == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x2B);
    REQUIRE(cpu.regs().yi == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));
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
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().ac == 0x60);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::V | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 8U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::V | mos6502::Z | mos6502::C | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 9U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::V | mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 0x0B);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[Decimal ADC]" ) {
    mock_bus->mockAddressValue(0x00, 0xF8); // SED

    mock_bus->mockAddressValue(0x01, 0xA9); // LDA
    mock_bus->mockAddressValue(0x02, 0x10); // IMM

    mock_bus->mockAddressValue(0x03, 0x69); // ADC
    mock_bus->mockAddressValue(0x04, 0x20); // IMM

    mock_bus->mockAddressValue(0x05, 0x69); // ADC
    mock_bus->mockAddressValue(0x06, 0x50); // IMM

    mock_bus->mockAddressValue(0x07, 0x69); // ADC
    mock_bus->mockAddressValue(0x08, 0x19); // IMM

    mock_bus->mockAddressValue(0x09, 0x69); // ADC
    mock_bus->mockAddressValue(0x0A, 0x01); // IMM

    mock_bus->mockAddressValue(0x0B, 0x69); // ADC
    mock_bus->mockAddressValue(0x0C, 0xAA); // IMM

    mock_bus->mockAddressValue(0x0D, 0x00); // PAD

    REQUIRE(cpu.step() == 2U); // SED
    REQUIRE(cpu.step() == 2U); // LDA
    REQUIRE(cpu.step() == 2U); // ADC

    REQUIRE(cpu.regs().pc == 5U);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::D));
    REQUIRE(cpu.regs().ac == 0x30);

    REQUIRE(cpu.step() == 2U); // ADC

    REQUIRE(cpu.regs().pc == 7U);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::D | mos6502::N | mos6502::V));
    REQUIRE(cpu.regs().ac == 0x80);

    REQUIRE(cpu.step() == 2U); // ADC

    REQUIRE(cpu.regs().pc == 9U);
    REQUIRE(int{cpu.regs().sr} == (mos6502::U | mos6502::B | mos6502::D | mos6502::N));
    REQUIRE(cpu.regs().ac == 0x99);

    REQUIRE(cpu.step() == 2U); // ADC

    REQUIRE(cpu.regs().pc == 11U);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::D | mos6502::Z | mos6502::C));
    REQUIRE(cpu.regs().ac == 0x00);

    REQUIRE(cpu.step() == 2U); // ADC

    REQUIRE(cpu.regs().pc == 13U);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::D | mos6502::C));
    REQUIRE(cpu.regs().ac == 0x11);
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
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().ac == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::C | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 5U);
    REQUIRE(cpu.regs().ac == 0x7F);
    REQUIRE(cpu.regs().sr == (mos6502::C | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().ac == 0x7F);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 8U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::C | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[CMP]" ) {
    mock_bus->mockAddressValue(0x00, 0xA9); // LDA
    mock_bus->mockAddressValue(0x01, 0x80); // IMM

    mock_bus->mockAddressValue(0x02, 0xC9); // CMP
    mock_bus->mockAddressValue(0x03, 0x80); // IMM

    mock_bus->mockAddressValue(0x04, 0xC9); // CMP
    mock_bus->mockAddressValue(0x05, 0x81); // IMM

    mock_bus->mockAddressValue(0x06, 0xC9); // CMP
    mock_bus->mockAddressValue(0x07, 0x7F); // IMM

    mock_bus->mockAddressValue(0x08, 0x00); // PAD

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().ac == 0x80U);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::C | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().ac == 0x80U);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 8U);
    REQUIRE(cpu.regs().ac == 0x80U);
    REQUIRE(cpu.regs().sr == (mos6502::C | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[CPX]" ) {
    mock_bus->mockAddressValue(0x00, 0xA2); // LDX
    mock_bus->mockAddressValue(0x01, 0x80); // IMM

    mock_bus->mockAddressValue(0x02, 0xE0); // CPX
    mock_bus->mockAddressValue(0x03, 0x80); // IMM

    mock_bus->mockAddressValue(0x04, 0xE4); // CPX
    mock_bus->mockAddressValue(0x05, 0x80); // ZPG

    mock_bus->mockAddressValue(0x06, 0xEC); // CPX
    mock_bus->mockAddressValue(0x07, 0x81); // ABS LO
    mock_bus->mockAddressValue(0x08, 0x00); // ABS HI

    mock_bus->mockAddressValue(0x0080, 0x81);
    mock_bus->mockAddressValue(0x0081, 0x7F);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().xi == 0x80);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().xi == 0x80U);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::C | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().xi == 0x80U);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 9U);
    REQUIRE(cpu.regs().xi == 0x80U);
    REQUIRE(cpu.regs().sr == (mos6502::C | mos6502::U | mos6502::B));
}


TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[CPY]" ) {
    mock_bus->mockAddressValue(0x00, 0xA0); // LDY
    mock_bus->mockAddressValue(0x01, 0x80); // IMM

    mock_bus->mockAddressValue(0x02, 0xC0); // CPY
    mock_bus->mockAddressValue(0x03, 0x80); // IMM

    mock_bus->mockAddressValue(0x04, 0xC4); // CPY
    mock_bus->mockAddressValue(0x05, 0x80); // ZPG

    mock_bus->mockAddressValue(0x06, 0xCC); // CPY
    mock_bus->mockAddressValue(0x07, 0x81); // ABS LO
    mock_bus->mockAddressValue(0x08, 0x00); // ABS HI

    mock_bus->mockAddressValue(0x0080, 0x81);
    mock_bus->mockAddressValue(0x0081, 0x7F);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().yi == 0x80);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().yi == 0x80U);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::C | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().yi == 0x80U);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 9U);
    REQUIRE(cpu.regs().yi == 0x80U);
    REQUIRE(cpu.regs().sr == (mos6502::C | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[AND]" ) {
    mock_bus->mockAddressValue(0x00, 0xA9); // LDA
    mock_bus->mockAddressValue(0x01, 0xFF); // IMM

    mock_bus->mockAddressValue(0x02, 0x29); // AND
    mock_bus->mockAddressValue(0x03, 0xA5); // IMM

    mock_bus->mockAddressValue(0x04, 0x29); // AND
    mock_bus->mockAddressValue(0x05, 0x7F); // IMM

    mock_bus->mockAddressValue(0x06, 0x29); // AND
    mock_bus->mockAddressValue(0x07, 0x5A); // IMM

    mock_bus->mockAddressValue(0x08, 0x00); // PAD

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().ac == 0xA5);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().ac == 0x25);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 8U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[BIT]" ) {
    mock_bus->mockAddressValue(0x00, 0x24); // LDA
    mock_bus->mockAddressValue(0x01, 0xFF); // ZPG

    mock_bus->mockAddressValue(0x02, 0x2C); // LDA
    mock_bus->mockAddressValue(0x03, 0xEF); // ABS LO
    mock_bus->mockAddressValue(0x04, 0xBE); // ABS HI

    mock_bus->mockAddressValue(0x00FF, 0xFF);
    mock_bus->mockAddressValue(0xBEEF, 0x00);

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::V | mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 5U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[ORA]" ) {
    mock_bus->mockAddressValue(0x00, 0x09); // ORA
    mock_bus->mockAddressValue(0x01, 0x00); // IMM

    mock_bus->mockAddressValue(0x02, 0x09); // ORA
    mock_bus->mockAddressValue(0x03, 0x0F); // IMM

    mock_bus->mockAddressValue(0x04, 0x09); // ORA
    mock_bus->mockAddressValue(0x05, 0xF0); // IMM

    mock_bus->mockAddressValue(0x06, 0x00); // PAD

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().ac == 0x0F);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().ac == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[EOR]" ) {
    mock_bus->mockAddressValue(0x00, 0x49); // EOR
    mock_bus->mockAddressValue(0x01, 0x0F); // IMM

    mock_bus->mockAddressValue(0x02, 0x49); // EOR
    mock_bus->mockAddressValue(0x03, 0xF0); // IMM

    mock_bus->mockAddressValue(0x04, 0x49); // EOR
    mock_bus->mockAddressValue(0x05, 0xFF); // IMM

    mock_bus->mockAddressValue(0x06, 0x00); // PAD

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0x0F);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().ac == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[INX]" ) {
    mock_bus->mockAddressValue(0x00, 0xA2); // LDX
    mock_bus->mockAddressValue(0x01, 0xFE); // IMM

    mock_bus->mockAddressValue(0x02, 0xE8); // INX
    mock_bus->mockAddressValue(0x03, 0xE8); // INX
    mock_bus->mockAddressValue(0x04, 0xE8); // INX

    mock_bus->mockAddressValue(0x05, 0x00); // PAD
    mock_bus->mockAddressValue(0x06, 0x00); // PAD

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().xi == 0xFE);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().xi == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().xi == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[INY]" ) {
    mock_bus->mockAddressValue(0x00, 0xA0); // LDY
    mock_bus->mockAddressValue(0x01, 0xFE); // IMM

    mock_bus->mockAddressValue(0x02, 0xC8); // INY
    mock_bus->mockAddressValue(0x03, 0xC8); // INY
    mock_bus->mockAddressValue(0x04, 0xC8); // INY

    mock_bus->mockAddressValue(0x05, 0x00); // PAD
    mock_bus->mockAddressValue(0x06, 0x00); // PAD

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().yi == 0xFE);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().yi == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().yi == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[INC]" ) {
    mock_bus->mockAddressValue(0x00, 0xE6); // INC
    mock_bus->mockAddressValue(0x01, 0x80); // ZPG

    mock_bus->mockAddressValue(0x02, 0xEE); // INC
    mock_bus->mockAddressValue(0x03, 0x80); // ABS LO
    mock_bus->mockAddressValue(0x04, 0x80); // ABS HI

    mock_bus->mockAddressValue(0x05, 0xA2); // LDX
    mock_bus->mockAddressValue(0x06, 0x20); // IMM

    mock_bus->mockAddressValue(0x07, 0xFE); // INC
    mock_bus->mockAddressValue(0x08, 0x80); // ABS LO,X
    mock_bus->mockAddressValue(0x09, 0x80); // ABS HI,X

    mock_bus->mockAddressValue(0x0A, 0xF6); // INC
    mock_bus->mockAddressValue(0x0B, 0x80); // ZPG,X

    mock_bus->mockAddressValue(0x0C, 0x00); // PAD

    mock_bus->mockAddressValue(0x0080, 0xFF);
    mock_bus->mockAddressValue(0x00A0, 0x01);
    mock_bus->mockAddressValue(0x8080, 0x7F);
    mock_bus->mockAddressValue(0x80A0, 0x40);

    REQUIRE(cpu.step() == 5U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(mock_bus->readWrittenValue(0x80) == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 6U);
    REQUIRE(cpu.regs().pc == 5U);
    REQUIRE(mock_bus->readWrittenValue(0x8080) == 0x80);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 7U);
    REQUIRE(cpu.regs().xi == 0x20);

    REQUIRE(cpu.step() == 7U);
    REQUIRE(cpu.regs().pc == 0x0A);
    REQUIRE(mock_bus->readWrittenValue(0x80A0) == 0x41);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 6U);
    REQUIRE(cpu.regs().pc == 0x0C);
    REQUIRE(mock_bus->readWrittenValue(0x00A0) == 0x02);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[DEC]" ) {
    mock_bus->mockAddressValue(0x00, 0xC6); // DEC
    mock_bus->mockAddressValue(0x01, 0x80); // ZPG

    mock_bus->mockAddressValue(0x02, 0xCE); // DEC
    mock_bus->mockAddressValue(0x03, 0x80); // ABS LO
    mock_bus->mockAddressValue(0x04, 0x80); // ABS HI

    mock_bus->mockAddressValue(0x05, 0xA2); // LDX
    mock_bus->mockAddressValue(0x06, 0x20); // IMM

    mock_bus->mockAddressValue(0x07, 0xDE); // DEC
    mock_bus->mockAddressValue(0x08, 0x80); // ABS LO,X
    mock_bus->mockAddressValue(0x09, 0x80); // ABS HI,X

    mock_bus->mockAddressValue(0x0A, 0xD6); // DEC
    mock_bus->mockAddressValue(0x0B, 0x80); // ZPG,X

    mock_bus->mockAddressValue(0x0C, 0x00); // PAD

    mock_bus->mockAddressValue(0x0080, 0xFF);
    mock_bus->mockAddressValue(0x00A0, 0x01);
    mock_bus->mockAddressValue(0x8080, 0x7F);
    mock_bus->mockAddressValue(0x80A0, 0x40);

    REQUIRE(cpu.step() == 5U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(mock_bus->readWrittenValue(0x80) == 0xFE);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 6U);
    REQUIRE(cpu.regs().pc == 5U);
    REQUIRE(mock_bus->readWrittenValue(0x8080) == 0x7E);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 7U);
    REQUIRE(cpu.regs().xi == 0x20);

    REQUIRE(cpu.step() == 7U);
    REQUIRE(cpu.regs().pc == 0x0A);
    REQUIRE(mock_bus->readWrittenValue(0x80A0) == 0x3F);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 6U);
    REQUIRE(cpu.regs().pc == 0x0C);
    REQUIRE(mock_bus->readWrittenValue(0x00A0) == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[DEX]" ) {
    mock_bus->mockAddressValue(0x00, 0xA2); // LDX
    mock_bus->mockAddressValue(0x01, 0x01); // IMM

    mock_bus->mockAddressValue(0x02, 0xCA); // DEX
    mock_bus->mockAddressValue(0x03, 0xCA); // DEX
    mock_bus->mockAddressValue(0x04, 0xCA); // DEX

    mock_bus->mockAddressValue(0x05, 0x00); // PAD
    mock_bus->mockAddressValue(0x06, 0x00); // PAD

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().xi == 0x01);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().xi == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().xi == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[DEY]" ) {
    mock_bus->mockAddressValue(0x00, 0xA0); // LDY
    mock_bus->mockAddressValue(0x01, 0x01); // IMM

    mock_bus->mockAddressValue(0x02, 0x88); // DEY
    mock_bus->mockAddressValue(0x03, 0x88); // DEY
    mock_bus->mockAddressValue(0x04, 0x88); // DEY

    mock_bus->mockAddressValue(0x05, 0x00); // PAD
    mock_bus->mockAddressValue(0x06, 0x00); // PAD

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().yi == 0x01);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().yi == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().yi == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[STA]" ) {
    mock_bus->mockAddressValue(0x00, 0x85); // STA
    mock_bus->mockAddressValue(0x01, 0x20); // ZPG

    mock_bus->mockAddressValue(0x02, 0x95); // STA
    mock_bus->mockAddressValue(0x03, 0x40); // ZPG,X

    mock_bus->mockAddressValue(0x04, 0x8D); // STA
    mock_bus->mockAddressValue(0x05, 0xEF); // ABS LO
    mock_bus->mockAddressValue(0x06, 0xBE); // ABS HI

    mock_bus->mockAddressValue(0x07, 0x9D); // STA
    mock_bus->mockAddressValue(0x08, 0xEF); // ABS LO,X
    mock_bus->mockAddressValue(0x09, 0xBE); // ABS HI,X

    mock_bus->mockAddressValue(0x0A, 0x99); // STA
    mock_bus->mockAddressValue(0x0B, 0xEF); // ABS LO,Y
    mock_bus->mockAddressValue(0x0C, 0xBE); // ABS HI,Y

    mock_bus->mockAddressValue(0x0D, 0x81); // STA
    mock_bus->mockAddressValue(0x0E, 0x80); // (IND,X)

    mock_bus->mockAddressValue(0x0F, 0x91); // STA
    mock_bus->mockAddressValue(0x10, 0x80); // (IND),Y

    mock_bus->mockAddressValue(0x11, 0xEA); // NOP

    cpu.regs().ac = 0xAB;

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(mock_bus->readWrittenValue(0x20) == 0xAB);

    cpu.regs().ac = 0x55;
    cpu.regs().xi = 0x20;

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(mock_bus->readWrittenValue(0x60) == 0x55);

    cpu.regs().ac = 0x11;

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 7U);
    REQUIRE(mock_bus->readWrittenValue(0xBEEF) == 0x11);

    cpu.regs().ac = 0x0F;
    cpu.regs().xi = 0x10;

    REQUIRE(cpu.step() == 5U);
    REQUIRE(cpu.regs().pc == 0x0A);

    REQUIRE(mock_bus->readWrittenValue(0xBEEF + 0x10) == 0x0F);

    cpu.regs().ac = 0xF0;
    cpu.regs().yi = 0x30;

    REQUIRE(cpu.step() == 5U);
    REQUIRE(cpu.regs().pc == 0x0D);

    REQUIRE(mock_bus->readWrittenValue(0xBEEF + 0x30) == 0xF0);

    cpu.regs().ac = 0xBB;
    cpu.regs().xi = 0x20;

    mock_bus->mockAddressValue(0xA0, 0x80); // IND LO
    mock_bus->mockAddressValue(0xA1, 0x80); // IND HI

    REQUIRE(cpu.step() == 6U);
    REQUIRE(cpu.regs().pc == 0x0F);

    REQUIRE(mock_bus->readWrittenValue(0x8080) == 0xBB);

    mock_bus->mockAddressValue(0x80, 0x40); // IND LO
    mock_bus->mockAddressValue(0x81, 0x90); // IND HI

    cpu.regs().ac = 0xCC;
    cpu.regs().yi = 0x10;

    REQUIRE(cpu.step() == 6U);
    REQUIRE(cpu.regs().pc == 0x11);

    REQUIRE(mock_bus->readWrittenValue(0x9050) == 0xCC);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[STX]" ) {
    mock_bus->mockAddressValue(0x00, 0xA2); // LDX
    mock_bus->mockAddressValue(0x01, 0x01); // IMM

    mock_bus->mockAddressValue(0x02, 0x86); // STX
    mock_bus->mockAddressValue(0x03, 0x80); // ZPG

    mock_bus->mockAddressValue(0x04, 0xA0); // LDY
    mock_bus->mockAddressValue(0x05, 0x01); // IMM

    mock_bus->mockAddressValue(0x06, 0x96); // STX
    mock_bus->mockAddressValue(0x07, 0x80); // ZPG,Y

    mock_bus->mockAddressValue(0x08, 0x8E); // STX
    mock_bus->mockAddressValue(0x09, 0x82); // ABS LO
    mock_bus->mockAddressValue(0x0A, 0x40); // ABS HI

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().xi == 0x01);
    REQUIRE(cpu.regs().yi == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().xi == 0x01);
    REQUIRE(cpu.regs().yi == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(mock_bus->readWrittenValue(0x0080) == 0x01);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().xi == 0x01);
    REQUIRE(cpu.regs().yi == 0x01);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 8U);
    REQUIRE(cpu.regs().xi == 0x01);
    REQUIRE(cpu.regs().yi == 0x01);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(mock_bus->readWrittenValue(0x0081) == 0x01);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x0B);
    REQUIRE(cpu.regs().xi == 0x01);
    REQUIRE(cpu.regs().yi == 0x01);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(mock_bus->readWrittenValue(0x4082) == 0x01);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[STY]" ) {
    mock_bus->mockAddressValue(0x00, 0xA0); // LDY
    mock_bus->mockAddressValue(0x01, 0x01); // IMM

    mock_bus->mockAddressValue(0x02, 0x84); // STY
    mock_bus->mockAddressValue(0x03, 0x80); // ZPG

    mock_bus->mockAddressValue(0x04, 0xA2); // LDX
    mock_bus->mockAddressValue(0x05, 0x01); // IMM

    mock_bus->mockAddressValue(0x06, 0x94); // STY
    mock_bus->mockAddressValue(0x07, 0x80); // ZPG,Y

    mock_bus->mockAddressValue(0x08, 0x8C); // STY
    mock_bus->mockAddressValue(0x09, 0x82); // ABS LO
    mock_bus->mockAddressValue(0x0A, 0x40); // ABS HI

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().xi == 0x00);
    REQUIRE(cpu.regs().yi == 0x01);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().xi == 0x00);
    REQUIRE(cpu.regs().yi == 0x01);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(mock_bus->readWrittenValue(0x0080) == 0x01);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().xi == 0x01);
    REQUIRE(cpu.regs().yi == 0x01);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 8U);
    REQUIRE(cpu.regs().xi == 0x01);
    REQUIRE(cpu.regs().yi == 0x01);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(mock_bus->readWrittenValue(0x0081) == 0x01);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 0x0B);
    REQUIRE(cpu.regs().xi == 0x01);
    REQUIRE(cpu.regs().yi == 0x01);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));

    REQUIRE(mock_bus->readWrittenValue(0x4082) == 0x01);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[TAX]" ) {
    mock_bus->mockAddressValue(0x00, 0xA9); // LDA
    mock_bus->mockAddressValue(0x01, 0x80); // IMM

    mock_bus->mockAddressValue(0x02, 0xAA); // TAX
    mock_bus->mockAddressValue(0x03, 0xAA); // TAX
    mock_bus->mockAddressValue(0x04, 0xAA); // TAX

    mock_bus->mockAddressValue(0x05, 0xEA); // NOP
    mock_bus->mockAddressValue(0x06, 0xEA); // NOP

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().xi == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    cpu.regs().sr = (mos6502::U | mos6502::B);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().xi == 0x80);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    cpu.regs().ac = 0x00;
    cpu.regs().sr = (mos6502::U | mos6502::B);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().xi == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    cpu.regs().ac = 0x7F;
    cpu.regs().sr = mos6502::Z | mos6502::N | mos6502::U | mos6502::B;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 5U);
    REQUIRE(cpu.regs().ac == 0x7F);
    REQUIRE(cpu.regs().xi == 0x07F);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[TAY]" ) {
    mock_bus->mockAddressValue(0x00, 0xA9); // LDA
    mock_bus->mockAddressValue(0x01, 0x80); // IMM

    mock_bus->mockAddressValue(0x02, 0xA8); // TAY
    mock_bus->mockAddressValue(0x03, 0xA8); // TAY
    mock_bus->mockAddressValue(0x04, 0xA8); // TAY

    mock_bus->mockAddressValue(0x05, 0xEA); // NOP
    mock_bus->mockAddressValue(0x06, 0xEA); // NOP

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().yi == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    cpu.regs().sr = (mos6502::U | mos6502::B);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().yi == 0x80);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    cpu.regs().ac = 0x00;
    cpu.regs().sr = (mos6502::U | mos6502::B);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().yi == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    cpu.regs().ac = 0x7F;
    cpu.regs().sr = (mos6502::Z | mos6502::N | mos6502::U | mos6502::B);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 5U);
    REQUIRE(cpu.regs().ac == 0x7F);
    REQUIRE(cpu.regs().yi == 0x07F);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[TSX]" ) {
    mock_bus->mockAddressValue(0x00, 0xBA); // TSX
    mock_bus->mockAddressValue(0x01, 0xBA); // TSX
    mock_bus->mockAddressValue(0x02, 0xBA); // TSX

    mock_bus->mockAddressValue(0x03, 0xEA); // NOP
    mock_bus->mockAddressValue(0x04, 0xEA); // NOP

    cpu.regs().sp = 0x01FF;
    cpu.regs().sr = (mos6502::U | mos6502::B);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().sp == 0x01FF);
    REQUIRE(cpu.regs().xi == 0xFF);
    REQUIRE(cpu.regs().sr == (mos6502::N | mos6502::U | mos6502::B));

    cpu.regs().sp = 0x0100;
    cpu.regs().sr = (mos6502::U | mos6502::B);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().sp == 0x0100);
    REQUIRE(cpu.regs().xi == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::Z | mos6502::U | mos6502::B));

    cpu.regs().sp = 0x017F;
    cpu.regs().sr = (mos6502::N | mos6502::Z | mos6502::U | mos6502::B);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().sp == 0x017F);
    REQUIRE(cpu.regs().xi == 0x7F);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[TXA]" ) {
    mock_bus->mockAddressValue(0x00, 0x8A); // TXA
    mock_bus->mockAddressValue(0x01, 0x8A); // TXA
    mock_bus->mockAddressValue(0x02, 0x8A); // TXA

    mock_bus->mockAddressValue(0x03, 0xEA); // NOP
    mock_bus->mockAddressValue(0x04, 0xEA); // NOP

    cpu.regs().xi = 0x01;
    cpu.regs().sr = mos6502::N | mos6502::Z;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().ac == 0x01);
    REQUIRE(cpu.regs().xi == 0x01);
    REQUIRE(cpu.regs().sr == 0x00);

    cpu.regs().xi = 0x00;
    cpu.regs().sr = 0x00;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().xi == 0x00);
    REQUIRE(cpu.regs().sr == mos6502::Z);

    cpu.regs().xi = 0x80;
    cpu.regs().sr = mos6502::N;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().xi == 0x80);
    REQUIRE(cpu.regs().sr == mos6502::N);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[TXS]" ) {
    mock_bus->mockAddressValue(0x00, 0x9A); // TXS
    mock_bus->mockAddressValue(0x01, 0x9A); // TXS
    mock_bus->mockAddressValue(0x02, 0x9A); // TXS

    mock_bus->mockAddressValue(0x03, 0xEA); // NOP
    mock_bus->mockAddressValue(0x04, 0xEA); // NOP

    cpu.regs().xi = 0x80;
    cpu.regs().sp = 0x0100;
    cpu.regs().sr = 0;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().sp == 0x0180);
    REQUIRE(cpu.regs().xi == 0x80);
    REQUIRE(cpu.regs().sr == mos6502::N);

    cpu.regs().xi = 0x7E;
    cpu.regs().sp = 0x02FF;
    cpu.regs().sr = mos6502::N | mos6502::Z;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().sp == 0x027E);
    REQUIRE(cpu.regs().xi == 0x7E);
    REQUIRE(cpu.regs().sr == 0x00);

    cpu.regs().xi = 0x00;
    cpu.regs().sr = 0x00;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().sp == 0x0200);
    REQUIRE(cpu.regs().xi == 0x00);
    REQUIRE(cpu.regs().sr == mos6502::Z);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[TYA]" ) {
    mock_bus->mockAddressValue(0x00, 0x98); // TYA
    mock_bus->mockAddressValue(0x01, 0x98); // TYA
    mock_bus->mockAddressValue(0x02, 0x98); // TYA

    mock_bus->mockAddressValue(0x03, 0xEA); // NOP
    mock_bus->mockAddressValue(0x04, 0xEA); // NOP

    cpu.regs().yi = 0x01;
    cpu.regs().sr = mos6502::N | mos6502::Z;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().ac == 0x01);
    REQUIRE(cpu.regs().yi == 0x01);
    REQUIRE(cpu.regs().sr == 0x00);

    cpu.regs().yi = 0x00;
    cpu.regs().sr = 0x00;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().yi == 0x00);
    REQUIRE(cpu.regs().sr == mos6502::Z);

    cpu.regs().yi = 0x80;
    cpu.regs().sr = mos6502::N;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().yi == 0x80);
    REQUIRE(cpu.regs().sr == mos6502::N);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[ASL]" ) {
    mock_bus->mockAddressValue(0x00, 0x0A); // ASL
    mock_bus->mockAddressValue(0x01, 0x0A); // ASL
    mock_bus->mockAddressValue(0x02, 0x0A); // ASL
    mock_bus->mockAddressValue(0x03, 0x0A); // ASL
    mock_bus->mockAddressValue(0x04, 0x0A); // ASL
    mock_bus->mockAddressValue(0x05, 0x0A); // ASL
    mock_bus->mockAddressValue(0x06, 0x0A); // ASL
    mock_bus->mockAddressValue(0x07, 0x0A); // ASL
    mock_bus->mockAddressValue(0x08, 0xEA); // NOP
    mock_bus->mockAddressValue(0x09, 0xEA); // NOP

    cpu.regs().ac = 0x01;
    cpu.regs().sr = mos6502::C;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().ac == 0x02);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0x04);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().ac == 0x08);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().ac == 0x10);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 5U);
    REQUIRE(cpu.regs().ac == 0x20);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().ac == 0x40);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 7U);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().sr == mos6502::N);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 8U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::C | mos6502::Z));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[LSR]" ) {
    mock_bus->mockAddressValue(0x00, 0x4A); // LSR
    mock_bus->mockAddressValue(0x01, 0x4A); // LSR
    mock_bus->mockAddressValue(0x02, 0x4A); // LSR
    mock_bus->mockAddressValue(0x03, 0x4A); // LSR
    mock_bus->mockAddressValue(0x04, 0x4A); // LSR
    mock_bus->mockAddressValue(0x05, 0x4A); // LSR
    mock_bus->mockAddressValue(0x06, 0x4A); // LSR
    mock_bus->mockAddressValue(0x07, 0x4A); // LSR
    mock_bus->mockAddressValue(0x08, 0xEA); // NOP
    mock_bus->mockAddressValue(0x09, 0xEA); // NOP

    cpu.regs().ac = 0x80;
    cpu.regs().sr = mos6502::C;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().ac == 0x40);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0x20);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().ac == 0x10);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().ac == 0x08);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 5U);
    REQUIRE(cpu.regs().ac == 0x04);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().ac == 0x02);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 7U);
    REQUIRE(cpu.regs().ac == 0x01);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 8U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::C | mos6502::Z));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[ROL]" ) {
    mock_bus->mockAddressValue(0x00, 0x2A); // ROL
    mock_bus->mockAddressValue(0x01, 0x2A); // ROL
    mock_bus->mockAddressValue(0x02, 0x2A); // ROL
    mock_bus->mockAddressValue(0x03, 0x2A); // ROL
    mock_bus->mockAddressValue(0x04, 0x2A); // ROL
    mock_bus->mockAddressValue(0x05, 0x2A); // ROL
    mock_bus->mockAddressValue(0x06, 0x2A); // ROL
    mock_bus->mockAddressValue(0x07, 0x2A); // ROL
    mock_bus->mockAddressValue(0x08, 0xEA); // NOP
    mock_bus->mockAddressValue(0x09, 0xEA); // NOP

    cpu.regs().ac = 0x01;
    cpu.regs().sr = mos6502::C;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().ac == 0x03);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0x06);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().ac == 0x0C);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().ac == 0x18);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 5U);
    REQUIRE(cpu.regs().ac == 0x30);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().ac == 0x60);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 7U);
    REQUIRE(cpu.regs().ac == 0xC0);
    REQUIRE(cpu.regs().sr == mos6502::N);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 8U);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().sr == (mos6502::C | mos6502::N));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[ROR]" ) {
    mock_bus->mockAddressValue(0x00, 0x6A); // ROR
    mock_bus->mockAddressValue(0x01, 0x6A); // ROR
    mock_bus->mockAddressValue(0x02, 0x6A); // ROR
    mock_bus->mockAddressValue(0x03, 0x6A); // ROR
    mock_bus->mockAddressValue(0x04, 0x6A); // ROR
    mock_bus->mockAddressValue(0x05, 0x6A); // ROR
    mock_bus->mockAddressValue(0x06, 0x6A); // ROR
    mock_bus->mockAddressValue(0x07, 0x6A); // ROR
    mock_bus->mockAddressValue(0x08, 0x6A); // ROR
    mock_bus->mockAddressValue(0x09, 0xEA); // NOP
    mock_bus->mockAddressValue(0x0A, 0xEA); // NOP

    cpu.regs().ac = 0x01;
    cpu.regs().sr = mos6502::C;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().sr == (mos6502::C | mos6502::N));

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0xC0);
    REQUIRE(cpu.regs().sr == mos6502::N);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().ac == 0x60);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 4U);
    REQUIRE(cpu.regs().ac == 0x30);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 5U);
    REQUIRE(cpu.regs().ac == 0x18);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 6U);
    REQUIRE(cpu.regs().ac == 0x0C);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 7U);
    REQUIRE(cpu.regs().ac == 0x06);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 8U);
    REQUIRE(cpu.regs().ac == 0x03);
    REQUIRE(cpu.regs().sr == 0x00);

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 9U);
    REQUIRE(cpu.regs().ac == 0x01);
    REQUIRE(cpu.regs().sr == mos6502::C);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[NOP]" ) {
    mock_bus->mockAddressValue(0x00, 0xEA); // NOP
    mock_bus->mockAddressValue(0x01, 0xEA); // NOP
    mock_bus->mockAddressValue(0x02, 0xEA); // NOP

    auto regs = cpu.regs();
    regs.pc = 0x01;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(regs == cpu.regs());
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[PHA]" ) {
    mock_bus->mockAddressValue(0x00, 0x48); // PHA
    mock_bus->mockAddressValue(0x01, 0xEA); // NOP
    mock_bus->mockAddressValue(0x02, 0xEA); // NOP

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().sp == 0x1FE);
    REQUIRE(mock_bus->readWrittenValue(0x1FF) == 0x00);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[PHP]" ) {
    mock_bus->mockAddressValue(0x00, 0x08); // PHP
    mock_bus->mockAddressValue(0x01, 0xEA); // NOP
    mock_bus->mockAddressValue(0x02, 0xEA); // NOP

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().sp == 0x1FE);
    REQUIRE(mock_bus->readWrittenValue(0x1FF) == (mos6502::U | mos6502::B));
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[PLA]" ) {
    mock_bus->mockAddressValue(0x00, 0x68); // PLA
    mock_bus->mockAddressValue(0x01, 0x68); // PLA
    mock_bus->mockAddressValue(0x02, 0x68); // PLA

    mock_bus->mockAddressValue(0x03, 0xEA); // NOP
    mock_bus->mockAddressValue(0x04, 0xEA); // NOP

    cpu.regs().sp = 0x1FC;
    mock_bus->mockAddressValue(0x1FD, 0x00);
    mock_bus->mockAddressValue(0x1FE, 0x80);
    mock_bus->mockAddressValue(0x1FF, 0x7F);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::Z));
    REQUIRE(cpu.regs().sp == 0x1FD);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().ac == 0x80);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::N));
    REQUIRE(cpu.regs().sp == 0x1FE);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().ac == 0x7F);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));
    REQUIRE(cpu.regs().sp == 0x1FF);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[PLP]" ) {
    mock_bus->mockAddressValue(0x00, 0x28); // PLP
    mock_bus->mockAddressValue(0x01, 0x28); // PLP
    mock_bus->mockAddressValue(0x02, 0x28); // PLP

    mock_bus->mockAddressValue(0x03, 0xEA); // NOP
    mock_bus->mockAddressValue(0x04, 0xEA); // NOP

    cpu.regs().sp = 0x1FC;
    mock_bus->mockAddressValue(0x1FD, 0xFF);
    mock_bus->mockAddressValue(0x1FE, 0x00);
    mock_bus->mockAddressValue(0x1FF, 0x0F);

    REQUIRE(cpu.step() == 4U);

    REQUIRE(cpu.regs().pc == 1U);
    REQUIRE(cpu.regs().ac == 0x00);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::Z | mos6502::N | mos6502::C | mos6502::D | mos6502::V | mos6502::I));
    REQUIRE(cpu.regs().sp == 0x1FD);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 2U);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));
    REQUIRE(cpu.regs().sp == 0x1FE);

    REQUIRE(cpu.step() == 4U);
    REQUIRE(cpu.regs().pc == 3U);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::D | mos6502::I | mos6502::Z | mos6502::C));
    REQUIRE(cpu.regs().sp == 0x1FF);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[BRK,RTI]" ) {
    mock_bus->mockAddressValue(0x00, 0x00); // BRK
    mock_bus->mockAddressValue(0x01, 0xFF); // #MARK
    mock_bus->mockAddressValue(0x02, 0xEA); // NOP
    mock_bus->mockAddressValue(0x03, 0xEA); // NOP
    mock_bus->mockAddressValue(0x04, 0xEA); // NOP

    mock_bus->mockAddressValue(0xBEEF, 0x40); // RTI
    mock_bus->mockAddressValue(0xBEF0, 0xEA); // NOP
    mock_bus->mockAddressValue(0xBEF1, 0xEA); // NOP

    mock_bus->mockAddressValue(0xFFFE, 0xEF); // LO
    mock_bus->mockAddressValue(0xFFFF, 0xBE); // HI

    REQUIRE(cpu.step() == 7U);
    REQUIRE(cpu.regs().pc == 0xBEEF);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B | mos6502::I));
    REQUIRE(cpu.regs().sp == 0x1FC);

    mock_bus->mockAddressValue(0x1FF, mock_bus->readWrittenValue(0x1FF));
    mock_bus->mockAddressValue(0x1FE, mock_bus->readWrittenValue(0x1FE));
    mock_bus->mockAddressValue(0x1FD, mock_bus->readWrittenValue(0x1FD));

    REQUIRE(cpu.step() == 6U);
    REQUIRE(cpu.regs().pc == 0x02);
    REQUIRE(cpu.regs().sr == (mos6502::U | mos6502::B));
    REQUIRE(cpu.regs().sp == 0x1FF);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[JSR,RTS]" ) {
    mock_bus->mockAddressValue(0x00, 0x20); // JSR
    mock_bus->mockAddressValue(0x01, 0x40); // ABS,LO
    mock_bus->mockAddressValue(0x02, 0x80); // ABS,HI

    mock_bus->mockAddressValue(0x03, 0xEA); // NOP
    mock_bus->mockAddressValue(0x04, 0xEA); // NOP
    mock_bus->mockAddressValue(0x05, 0xEA); // NOP

    mock_bus->mockAddressValue(0x8040, 0x60); // RTS
    mock_bus->mockAddressValue(0x8041, 0xEA); // NOP
    mock_bus->mockAddressValue(0x8042, 0xEA); // NOP

    REQUIRE(cpu.step() == 6U);
    REQUIRE(cpu.regs().pc == 0x8040);

    mock_bus->mockAddressValue(0x1FF, mock_bus->readWrittenValue(0x1FF));
    mock_bus->mockAddressValue(0x1FE, mock_bus->readWrittenValue(0x1FE));

    REQUIRE(cpu.step() == 6U);
    REQUIRE(cpu.regs().pc == 0x03);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[JMP]" ) {
    mock_bus->mockAddressValue(0x00, 0x4C); // JSR
    mock_bus->mockAddressValue(0x01, 0xEF); // ABS,LO
    mock_bus->mockAddressValue(0x02, 0xBE); // ABS,HI

    mock_bus->mockAddressValue(0xBEEF, 0x6C); // JSR
    mock_bus->mockAddressValue(0xBEF0, 0xAB); // IND,LO
    mock_bus->mockAddressValue(0xBEF1, 0xCA); // IND,HI

    mock_bus->mockAddressValue(0xCAAB, 0x10); // ABS,LO
    mock_bus->mockAddressValue(0xCAAC, 0x20); // ABS,HI

    REQUIRE(cpu.step() == 3U);
    REQUIRE(cpu.regs().pc == 0xBEEF);

    REQUIRE(cpu.step() == 5U);
    REQUIRE(cpu.regs().pc == 0x2010);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[BCC,BCS]" ) {
    mock_bus->mockAddressValue(0x00, 0xB0); // BCS
    mock_bus->mockAddressValue(0x01, 0x01); // REL (+1)
    mock_bus->mockAddressValue(0x02, 0xEA); // NOP

    mock_bus->mockAddressValue(0x03, 0x90); // BCC
    mock_bus->mockAddressValue(0x04, 0xFD); // REL (-3)
    mock_bus->mockAddressValue(0x05, 0xEA); // NOP

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);

    cpu.regs().sr |= mos6502::C;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 5U);

    cpu.regs().pc = 0x00;
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);

    cpu.regs().sr &= ~mos6502::C;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[BEQ,BNE]" ) {
    mock_bus->mockAddressValue(0x00, 0xF0); // BEQ
    mock_bus->mockAddressValue(0x01, 0x01); // REL (+1)
    mock_bus->mockAddressValue(0x02, 0xEA); // NOP

    mock_bus->mockAddressValue(0x03, 0xD0); // BNE
    mock_bus->mockAddressValue(0x04, 0xFD); // REL (-3)
    mock_bus->mockAddressValue(0x05, 0xEA); // NOP

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);

    cpu.regs().sr |= mos6502::Z;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 5U);

    cpu.regs().pc = 0x00;
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);

    cpu.regs().sr &= ~mos6502::Z;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[BMI,BPL]" ) {
    mock_bus->mockAddressValue(0x00, 0x30); // BMI
    mock_bus->mockAddressValue(0x01, 0x01); // REL (+1)
    mock_bus->mockAddressValue(0x02, 0xEA); // NOP

    mock_bus->mockAddressValue(0x03, 0x10); // BPL
    mock_bus->mockAddressValue(0x04, 0xFD); // REL (-3)
    mock_bus->mockAddressValue(0x05, 0xEA); // NOP

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);

    cpu.regs().sr |= mos6502::N;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 5U);

    cpu.regs().pc = 0x00;
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);

    cpu.regs().sr &= ~mos6502::N;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
}

TEST_CASE_METHOD(CpuFixture, "Instruction Test", "[BVS,BVC]" ) {
    mock_bus->mockAddressValue(0x00, 0x70); // BVS
    mock_bus->mockAddressValue(0x01, 0x01); // REL (+1)
    mock_bus->mockAddressValue(0x02, 0xEA); // NOP

    mock_bus->mockAddressValue(0x03, 0x50); // BVC
    mock_bus->mockAddressValue(0x04, 0xFD); // REL (-3)
    mock_bus->mockAddressValue(0x05, 0xEA); // NOP

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);

    cpu.regs().sr |= mos6502::V;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 5U);

    cpu.regs().pc = 0x00;
    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 3U);

    cpu.regs().sr &= ~mos6502::V;

    REQUIRE(cpu.step() == 2U);
    REQUIRE(cpu.regs().pc == 2U);
}

TEST_CASE_METHOD(CpuFixture, "Emulate clock rate", "[HighPrecisionSyncer]") {
    mock_bus->mockAddressValue(0x00, 0xA9); // LDA
    mock_bus->mockAddressValue(0x01, 0x50); // IMM

    mock_bus->mockAddressValue(0x02, 0x69); // ADC
    mock_bus->mockAddressValue(0x03, 0x10); // IMM

    mock_bus->mockAddressValue(0x04, 0x4C); // JMP
    mock_bus->mockAddressValue(0x05, 0x00); // ABS,LO
    mock_bus->mockAddressValue(0x06, 0x00); // ABS,HI

    std::uint64_t const ntsc_clock_rate = 3'579'545;
    std::uint64_t const atari_clock_rate = ntsc_clock_rate / 3U;
    mos6502::ClockSync syncer{atari_clock_rate, 60U, mos6502::ClockSync::SyncPrecision::High};

    timespec thread_cpu_time_before{};
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &thread_cpu_time_before);

    auto const start_time_point = std::chrono::steady_clock::now();
    syncer.elapse(0U);
    do {
        std::uint8_t const ticks = cpu.step();
        syncer.elapse(ticks);
    } while (syncer.frame_count() < 600);
    auto const stop_time_point = std::chrono::steady_clock::now();

    timespec thread_cpu_time_after{};
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &thread_cpu_time_after);

    std::uint64_t const thread_cpu_time = (
        static_cast<std::uint64_t>(thread_cpu_time_after.tv_sec * 1'000'000'000U) +
        static_cast<std::uint64_t>(thread_cpu_time_after.tv_nsec)
        ) - (
        static_cast<std::uint64_t>(thread_cpu_time_before.tv_sec * 1'000'000'000U) +
        static_cast<std::uint64_t>(thread_cpu_time_before.tv_nsec)
    );

    auto const delta_time = stop_time_point - start_time_point;

    printf("ClockSync (High):\n\tdeviation=%lldns\n\tlast_ts=%lf\n\tthread_cpu_time=%lf\n",
        delta_time.count() - 10'000'000'000,
        (syncer.timestamp_of_last_frame() - syncer.timestamp_of_first_frame()) / 1e9,
        thread_cpu_time / 1e9);

    CHECK(delta_time > (std::chrono::seconds{10} - std::chrono::microseconds{2}));
    CHECK(delta_time < (std::chrono::seconds{10} + std::chrono::microseconds{2}));
    REQUIRE((syncer.total_ticks() / atari_clock_rate) == 10);
}

TEST_CASE_METHOD(CpuFixture, "Emulate clock rate", "[LowPrecisionSyncer]") {
    mock_bus->mockAddressValue(0x00, 0xA9); // LDA
    mock_bus->mockAddressValue(0x01, 0x50); // IMM

    mock_bus->mockAddressValue(0x02, 0x69); // ADC
    mock_bus->mockAddressValue(0x03, 0x10); // IMM

    mock_bus->mockAddressValue(0x04, 0x4C); // JMP
    mock_bus->mockAddressValue(0x05, 0x00); // ABS,LO
    mock_bus->mockAddressValue(0x06, 0x00); // ABS,HI

    std::uint64_t const ntsc_clock_rate = 3'579'545;
    std::uint64_t const atari_clock_rate = ntsc_clock_rate / 3U;
    mos6502::ClockSync syncer{atari_clock_rate, 60U, mos6502::ClockSync::SyncPrecision::Low};

    timespec thread_cpu_time_before{};
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &thread_cpu_time_before);

    auto const start_time_point = std::chrono::steady_clock::now();
    syncer.elapse(0U);
    do {
        std::uint8_t const ticks = cpu.step();
        syncer.elapse(ticks);
    } while (syncer.frame_count() < 600);
    auto const stop_time_point = std::chrono::steady_clock::now();

    timespec thread_cpu_time_after{};
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &thread_cpu_time_after);

    std::uint64_t const thread_cpu_time = (
        static_cast<std::uint64_t>(thread_cpu_time_after.tv_sec * 1'000'000'000U) +
        static_cast<std::uint64_t>(thread_cpu_time_after.tv_nsec)
        ) - (
        static_cast<std::uint64_t>(thread_cpu_time_before.tv_sec * 1'000'000'000U) +
        static_cast<std::uint64_t>(thread_cpu_time_before.tv_nsec)
    );

    auto const delta_time = stop_time_point - start_time_point;

    printf("ClockSync (Low):\n\tdeviation=%lld\n\tlast_ts=%lf\n\tthread_cpu_time=%lf\n",
        delta_time.count() - 10'000'000'000,
        (syncer.timestamp_of_last_frame() - syncer.timestamp_of_first_frame()) / 1e9,
        thread_cpu_time / 1e9);

    CHECK(delta_time > (std::chrono::seconds{10} - std::chrono::milliseconds{15}));
    CHECK(delta_time < (std::chrono::seconds{10} + std::chrono::milliseconds{15}));
    REQUIRE((syncer.total_ticks() / atari_clock_rate) == 10);
}


TEST_CASE_METHOD(CpuFixture, "Emulate clock rate", "[MedPrecisionSyncer]") {
    mock_bus->mockAddressValue(0x00, 0xA9); // LDA
    mock_bus->mockAddressValue(0x01, 0x50); // IMM

    mock_bus->mockAddressValue(0x02, 0x69); // ADC
    mock_bus->mockAddressValue(0x03, 0x10); // IMM

    mock_bus->mockAddressValue(0x04, 0x4C); // JMP
    mock_bus->mockAddressValue(0x05, 0x00); // ABS,LO
    mock_bus->mockAddressValue(0x06, 0x00); // ABS,HI

    std::uint64_t const ntsc_clock_rate = 3'579'545;
    std::uint64_t const atari_clock_rate = ntsc_clock_rate / 3U;
    mos6502::ClockSync syncer{atari_clock_rate, 60U, mos6502::ClockSync::SyncPrecision::Medium};

    timespec thread_cpu_time_before{};
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &thread_cpu_time_before);

    auto const start_time_point = std::chrono::steady_clock::now();
    syncer.elapse(0U);
    do {
        std::uint8_t const ticks = cpu.step();
        syncer.elapse(ticks);
    } while (syncer.frame_count() < 600);
    auto const stop_time_point = std::chrono::steady_clock::now();

    timespec thread_cpu_time_after{};
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &thread_cpu_time_after);

    std::uint64_t const thread_cpu_time = (
        static_cast<std::uint64_t>(thread_cpu_time_after.tv_sec * 1'000'000'000U) +
        static_cast<std::uint64_t>(thread_cpu_time_after.tv_nsec)
        ) - (
        static_cast<std::uint64_t>(thread_cpu_time_before.tv_sec * 1'000'000'000U) +
        static_cast<std::uint64_t>(thread_cpu_time_before.tv_nsec)
    );

    auto const delta_time = stop_time_point - start_time_point;

    printf("ClockSync (Med):\n\tdeviation=%lld\n\tlast_ts=%lf\n\tthread_cpu_time=%lf\n",
        delta_time.count() - 10'000'000'000,
        (syncer.timestamp_of_last_frame() - syncer.timestamp_of_first_frame()) / 1e9,
        thread_cpu_time / 1e9);

    CHECK(delta_time > (std::chrono::seconds{10} - std::chrono::microseconds{900}));
    CHECK(delta_time < (std::chrono::seconds{10} + std::chrono::microseconds{900}));
    REQUIRE((syncer.total_ticks() / atari_clock_rate) == 10);
}
