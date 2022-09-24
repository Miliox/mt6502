#pragma once
#include <cstdint>

namespace mos6502
{
/// MOS 6502 Registers
struct Registers final {
    std::uint8_t ac;   /// Accumulator
    std::uint8_t xi;   /// X Index
    std::uint8_t yi;   /// Y Index
    std::uint8_t sr;   /// Status register
    std::uint16_t sp;  /// Stack pointer
    std::uint16_t pc;  /// Program counter
};

static_assert(sizeof(Registers) == 8, "");
}
