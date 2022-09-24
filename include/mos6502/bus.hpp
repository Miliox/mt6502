#pragma once
#include <cstdint>

namespace mos6502
{
/// Bus Interface
struct IBus {
    virtual ~IBus();

    /// Read from address
    virtual std::uint8_t read(std::uint16_t addr) = 0;

    /// Write to address
    virtual void write(std::uint16_t addr, std::uint8_t data) = 0;
};
}
