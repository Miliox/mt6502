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
