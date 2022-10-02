#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <cmath>
#include <memory>
#include <string_view>

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

static double micro_bench(std::uint8_t opcode, std::uint64_t number_of_runs) {
    assert(number_of_runs > 0);

    std::shared_ptr<BenchBus> bus{new BenchBus{opcode}};
    mos6502::Cpu cpu{bus};

    auto const t0 = std::chrono::steady_clock::now();
    for (std::uint64_t i{}; i < number_of_runs; ++i)
    {
        cpu.step();
    }
    auto const t1 = std::chrono::steady_clock::now();
    auto const delta = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0);
    double const average = delta.count() / static_cast<double>(number_of_runs);

    printf("%02x => %.18f ns\n", opcode, average);
    fflush(stdout);

    return average;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " number_of_runs\n";
        return 1;
    }

    std::string_view arg_1{argv[1]};
    if (!std::all_of(arg_1.begin(), arg_1.end(), [](char c) { return std::isdigit(c); }))
    {
        std::cerr << "Error: number_of_runs must be a positive number\n";
        return 2;
    }

    std::uint64_t number_of_runs = std::stoull(arg_1.data());
    if (number_of_runs == 0) {
        std::cerr << "Error: number_of_runs must be a positive number\n";
        return 2;
    }

    std::array<double, 256> instr_avg{};

    std::cout << "Instruction Average Duration:" << std::endl;
    for (size_t opcode{}; opcode < instr_avg.size(); ++opcode)
    {
        try {
            instr_avg[opcode] = micro_bench(static_cast<std::uint8_t>(opcode), number_of_runs);
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
