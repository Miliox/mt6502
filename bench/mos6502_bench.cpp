#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <cmath>
#include <memory>

#include "mos6502/bus.hpp"
#include "mos6502/cpu.hpp"
#include "mos6502/regs.hpp"
#include "mos6502/status.hpp"

class BenchBus final : public mos6502::IBus {
public:
    BenchBus(std::uint8_t opcode) : m_opcode{opcode} {
        static_cast<void>(m_padding);
    }

    ~BenchBus() override;

    std::uint8_t read(std::uint16_t addr) override {
        static_cast<void>(addr);
        return m_opcode;
    }

    void write(std::uint16_t addr, std::uint8_t data) override {
        static_cast<void>(addr);
        static_cast<void>(data);
    }

private:
    std::uint8_t const m_opcode{};
    std::array<std::uint8_t, 7U> m_padding{};
};

BenchBus::~BenchBus() = default;

static double micro_bench(std::uint8_t opcode) {
    std::shared_ptr<BenchBus> bus{new BenchBus{opcode}};
    mos6502::Cpu cpu{bus};

    auto const t0 = std::chrono::steady_clock::now();
    for (size_t i{}; i < 1'000'000'000U; ++i)
    {
        cpu.step();
    }
    auto const t1 = std::chrono::steady_clock::now();
    auto const delta = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0);
    double const average = delta.count() / 1e9;

    printf("%02x => %.18f\n", opcode, average);
    fflush(stdout);

    return average;
}

int main() {
    std::array<double, 256> instr_avg{};

    std::cout << "Instruction Average Duration:" << std::endl;
    for (size_t opcode{}; opcode < instr_avg.size(); ++opcode)
    {
        try {
            instr_avg[opcode] = micro_bench(static_cast<std::uint8_t>(opcode));
        } catch(...) {
            instr_avg[opcode] = nan("");
        }
    }

    double lowest{std::numeric_limits<double>::max()};
    for (size_t opcode{}; opcode < instr_avg.size(); ++opcode)
    {
        if (isnan(instr_avg[opcode])) {
            continue;
        }
        lowest = std::min(lowest, instr_avg[opcode]);
    }


    std::cout << "Instruction Comparative Ratio:" << std::endl;
    for (size_t opcode{}; opcode < instr_avg.size(); ++opcode)
    {
        if (isnan(instr_avg[opcode])) {
            continue;
        }
        printf("%02x => %.18f\n", static_cast<std::uint8_t>(opcode), instr_avg[opcode] / lowest);
    }

    return 0;
}
