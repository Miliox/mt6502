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

# Path to VCPKG toolchain for cmake
export VCKPG_TOOLCHAIN_FILE=${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake

# Generate Makefile
cmake -B build -DCMAKE_TOOLCHAIN_FILE=${VCPKG_TOOLCHAIN_FILE}

# Build
make -C build

# Unit Tests
build/mt6502_test

# Benchmark 1 million runs per instruction
build/mt6502_bench 1000000
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

Implement a memory mapping is necessary to class that inherit from mos6502::IBus.

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

mos6502::Cpu cpu{mm_bap};

for(;;) {
    auto cycles = cpu.step();

    // ommitted
}
```

## LICENSE

[MIT](LICENSE.md)
