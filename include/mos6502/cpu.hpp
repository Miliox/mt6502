#pragma once
#include <cstdint>
#include <memory>

namespace mos6502
{
struct IBus;
struct Registers;

/// Mos Technology 6502 Microprocessor
class Cpu final {
public:
    /// Constructor
    /// @param bus the interface to access memory
    Cpu(std::shared_ptr<IBus> bus);

    /// Destructor
    ~Cpu();

    /// Retrieve registers
    /// @return reference to registers
    Registers& regs();

    /// Step current instruction
    std::uint8_t step();

    /// Signal maskable interrupt
    void signal_irq();

    /// Signal non maskable interrupt
    void signal_nmi();

    /// Signal reset
    void signal_reset();

private:
    class Impl;
    std::unique_ptr<Impl> m_pimpl;
};
}
