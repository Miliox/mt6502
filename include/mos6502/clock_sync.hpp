#pragma once
#include <cstdint>

namespace mos6502
{

class ClockSync final {
public:
    enum class SyncPrecision : std::uint64_t {
        // Few milliseconds precision with low cpu usage.
        Low,
        // Under millisecond precision with slightly more cpu usage (~10%) than SyncPrecision::Low.
        Medium,
        /// Under microsecond precision with full cpu usage.
        High
    };

    ClockSync(
        std::uint64_t const clock_rate,
        std::uint64_t const frame_rate,
        SyncPrecision const sync_precision = SyncPrecision::Low);

    ClockSync(
        std::uint64_t const clock_rate,
        std::uint64_t const clock_rate_fraction,
        std::uint64_t const frame_rate,
        std::uint64_t const frame_rate_fraction,
        SyncPrecision const sync_precision = SyncPrecision::Low);

    void elapse(std::uint8_t ticks);

    inline std::uint64_t frame_count() const {
        return m_frame_count;
    }

    inline std::uint64_t busy_period() const {
        return m_busy_period;
    }

    inline std::uint64_t idle_period() const {
        return m_idle_period;
    }

    inline std::uint64_t total_ticks() const {
        return m_total_ticks;
    }

    inline std::uint64_t timestamp_of_first_frame() const {
        return m_frame_first_ts;
    }

    inline std::uint64_t timestamp_of_last_frame() const {
        return m_frame_last_ts;
    }

private:
    std::uint64_t const m_frame_period;
    std::uint64_t const m_frame_period_fraction;
    std::uint64_t const m_ticks_per_frame;
    std::uint64_t const m_ticks_per_frame_fraction;
    SyncPrecision const m_sync_precision;

    std::uint64_t m_frame_count;
    std::uint64_t m_frame_ticks;
    std::uint64_t m_frame_ticks_fraction;
    std::uint64_t m_frame_first_ts;
    std::uint64_t m_frame_next_ts;
    std::uint64_t m_frame_last_ts;

    std::uint64_t m_busy_period;
    std::uint64_t m_idle_period;
    std::uint64_t m_total_ticks;
};

}
