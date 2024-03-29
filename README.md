# Lib Mos Technology 6502 (MT6502)

Software emulation of the classic 8-bit processor used by Apple, Atari,
Commodore and Nintendo in the 1980s and 1990s.

This is a working in progress, and there are missing features and bugs.

## Dependencies

* [cmake](https://cmake.org/)
* [vcpkg](https://vcpkg.io/en/index.html)


## Build

```bash
# Create build directory
mkdir build

# Install package dependencies
${VCPKG_DIR}/vcpkg install doctest nanobench

# Path to VCPKG toolchain for cmake
export VCKPG_TOOLCHAIN_FILE=${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake

# Generate Makefile
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=${VCPKG_TOOLCHAIN_FILE} -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Unit Tests
build/mt6502_test

# Benchmark
build/mt6502_bench
```

## API

This library has two main components the CPU (mos6502::Cpu) and BUS Interface
(mos6502::IBus) where the first executes the instructions and the second
abstract the memory through 16 bit address space.

Through the address space RAM, ROM, and different devices can be accessed. It
expected mapping respect 6502 specifications for zero page, stack, interrupt
handler vector, etc are mapped in the correct address ranges.

```
+-----+          +-----+             +-----+
| CPU | <------> | BUS | <---------> | RAM |
+-----+          +-----+      |      +-----+
                              |
                              |      +-----+
                              |----> | ROM |
                              |      +-----+
                              |
                              |      +-----+
                              |----> | DEV |
                              |      +-----+
                             ...
```

The classes in this library are:

```mermaid
classDiagram
    Cpu ..* Registers : Memento
    Cpu <.. ClockSync : Trottle speed
    Cpu ..> IBus      : Access to MMU

class ClockSync {
    +elapse(uint8_t ticks)
}

class Cpu {
    +step() uint8_t
}

class IBus {
    <<Interface>>
    +read(uint16_t addr) uint8_t
    +write(uint16_t addr, uint8_t data) void
}

class Registers {
    +uint8_t  ac
    +uint8_t  xi
    +uint8_t  yi
    +uint8_t  sr
    +uint16_t sp
    +uint16_t pc
}
```

Leaving the user of this library to implement a memory mapping by inherit from mos6502::IBus.

This is done by declaring a MemoryMapper class like the one below:

```cpp
// @file memory_mapper.hpp
#include "mos6502/bus.hpp"

class MemoryMapper final : public mos6502::IBus {
public:
    MemoryMapper(/* ctor args */);

    ~MemoryMapper() override;

    std::uint8_t read(std::uint16_t addr);

    void write(std::uint16_t addr, std::uint8_t data) override;

// ommitted
}
```

Then when create the CPU pass the dependency to IBus in the constructor.


```cpp
auto mm_map = std::make_shared<MemoryMapper>(/* ctor args */);

mos6502::Cpu cpu{mm_map};
```

Run the cpu in a loop steping one instruction at time. Without any
synchronization the cpu will run faster than the target emulation
speed so use ClockSync to down speed to desired frequency and
frame rate.

```cpp
mos6502::ClockSync syncer{kClockPerSecond, kFramePerSecond};

for(;;) {
    auto cycles = cpu.step();

    // ...

    syncer.elapse(cycles);
}
```

## LICENSE

[MIT](LICENSE.md)
