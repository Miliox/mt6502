#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "mos6502/bus.hpp"
#include "mos6502/cpu.hpp"

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
    std::unordered_map<std::uint16_t, std::uint8_t> mem_map;
};

MockBus::MockBus() = default;

MockBus::~MockBus() = default;

TEST_CASE( "Factorials are computed", "[factorial]" ) {
    auto mock_bus = std::shared_ptr<MockBus>(new MockBus{});

    mos6502::Cpu cpu{mock_bus};
}
