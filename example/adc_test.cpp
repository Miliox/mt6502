#include <cstdint>
#include <iostream>
#include <x86intrin.h>

void adc(bool carry, std::uint8_t x, std::uint8_t y) {
    std::uint8_t carry_in{carry ? std::uint8_t{1} : std::uint8_t{0}};
    std::uint8_t carry_out{};

    std::uint8_t r = __builtin_addcb(x, y, carry_in, &carry_out);
    std::cout << static_cast<int>(x) << " + " << static_cast<int>(y) << " + " << static_cast<int>(carry_in)
              << " => " << static_cast<int>(r) << ", " << static_cast<int>(carry_out) << std::endl;
}

int main()
{
    adc(false, 0x00, 0x00);
    adc(false, 0x7E, 0x01);
    adc(false, 0x80, 0x7F);

    adc(true, 0x00, 0x00);
    adc(true, 0x7E, 0x01);
    adc(true, 0x80, 0x7F);
    return 0;
}