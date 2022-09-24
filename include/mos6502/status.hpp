#pragma once
#include <cstdint>

namespace mos6502
{
enum class Status : std::uint8_t {
    N = 1 << 7, /// Negative
    V = 1 << 6, /// Overflow
    U = 1 << 5, /// Unused
    B = 1 << 4, /// Break (Virtual)
    D = 1 << 3, /// Decimal
    I = 1 << 2, /// Interrupt (IRQ Disabled)
    Z = 1 << 1, /// Zero
    C = 1 << 0  /// Carry
};
}
