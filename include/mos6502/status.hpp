#pragma once
#include <cstdint>

namespace mos6502
{
constexpr std::uint8_t N{1 << 7}; // Negative
constexpr std::uint8_t V{1 << 6}; // Overflow
constexpr std::uint8_t U{1 << 5}; // Unused
constexpr std::uint8_t B{1 << 4}; // Break (Virtual)
constexpr std::uint8_t D{1 << 3}; // Decimal
constexpr std::uint8_t I{1 << 2}; // Interrupt (IRQ Disabled)
constexpr std::uint8_t Z{1 << 1}; // Zero
constexpr std::uint8_t C{1 << 0}; // Carry
}
