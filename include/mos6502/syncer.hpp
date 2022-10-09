#pragma once
#include <chrono>
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
    /// Strategy to clock synchronization
    enum class Strategy {
        /// Use std::this_thread::sleep_until to suspend execution.
        ///
        /// Jitter of one to three milliseconds.
        Sleep,

        /// Use thread::yield spin to suspend execution but at cost of
        /// inefficient cpu usage.
        ///
        /// Jitter of tenths of microseconds
        Spin,

        /// Combination of ThreadSleep and ThreadYieldSpinLoop where
        /// thread sleep until a couple of milliseconds before it
        /// spin to homing to target time.
        ///
        /// Jitter of hundreths of microseconds.
        Hybrid
    };

    /// @brief Construct a synchronizer
    /// @pre clock_rate and frame_rate must be greater than zero
    /// @param clock_rate the target clock rate to synchronize
    /// @param frame_rate the target frame rate to synchronize
    Syncer(std::uint64_t const clock_rate, std::uint64_t const frame_rate, Strategy const strat = Strategy::Sleep);

    ~Syncer();

    /// @brief Call this method after every cpu step, it will automatically delay
    ///        when ticks goes over target rate
    /// @param ticks the ammount of ticks elapsed
    void elapse(std::uint64_t const ticks);

    std::uint64_t frame_count() const;

    std::chrono::nanoseconds busy_total_time() const;

    std::chrono::nanoseconds idle_total_time() const;

    std::chrono::nanoseconds jitter_total_time() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_pimpl;
};
}
