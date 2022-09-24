#pragma once
#include <cstdint>
#include <memory>

namespace mos6502
{
struct IBus;
struct Registers;

class Cpu final {
public:
    Cpu(std::shared_ptr<IBus> bus);

    ~Cpu();

    /// Retrieve registers
    Registers& regs();

    /// Step current instruction
    std::uint8_t step();

private:
    class Impl;
    std::unique_ptr<Impl> m_pimpl;
};
}
