#pragma once
#include <cstdint>
#include <memory>

namespace mos6502
{
/// Syncer: a clock synchronizer utility
///
/// Emulate clock rate of original cpu by introduce sleep points when cpu ticks
/// reach ammount in one frame. Because the non real time nature of Linux, macOS
/// and Windows small jitter are expected and handled on next frame.
class Syncer final {
public:
    /// @brief Construct a synchronizer
    /// @pre clock_rate and frame_rate must be greater than zero
    /// @param clock_rate the target clock rate to synchronize
    /// @param frame_rate the target frame rate to synchronize
    Syncer(std::uint64_t const clock_rate, std::uint64_t const frame_rate);

    /// @brief Call this method after every cpu step, it will automatically delay
    ///        when ticks goes over target rate
    /// @param ticks the ammount of ticks
    void elapse(std::uint64_t const ticks);

private:
    class Impl;
    std::unique_ptr<Impl> m_pimpl;
};
}
