#include "mos6502/high_precision_syncer.hpp"

#if defined(__APPLE__) || defined(__linux__)
#include <sys/time.h>
#elif defined(_WIN32)
#else
static_assert(false, "unsupported operation system");
#endif

namespace mos6502 {

#if defined(__APPLE__)
static __attribute__((always_inline)) std::uint64_t now() {
    // CLOCK_MONOTONIC_RAW permit reach hundreths of nanoseconds precision
    return clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
}
#elif defined(__linux__)
static __attribute__((always_inline)) std::uint64_t now() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec * 1'000'000'000) + static_cast<std::uint64_t>(ts.tv_nsec);
}
#elif defined(_WIN32)
#else
static_assert(false, "unsupported operation system");
#endif

HighPrecisionSyncer::HighPrecisionSyncer(
        std::uint64_t const clock_rate,
        std::uint64_t const frame_rate)
        : HighPrecisionSyncer(clock_rate, 0U, frame_rate, 0U) {}

HighPrecisionSyncer::HighPrecisionSyncer(
    std::uint64_t const clock_rate,
    std::uint64_t const clock_rate_fraction,
    std::uint64_t const frame_rate,
    std::uint64_t const frame_rate_fraction)
    : m_frame_period{1'000'000'000U / frame_rate}
    , m_frame_period_fraction{1'000'000'000U % frame_rate}
    , m_ticks_per_frame{clock_rate / frame_rate}
    , m_ticks_per_frame_fraction{clock_rate % frame_rate}
    , m_frame_count{}
    , m_frame_ticks{}
    , m_frame_ticks_fraction{}
    , m_frame_last_ts{}
    , m_busy_period{}
    , m_idle_period{}
    , m_total_ticks{}
{
    // TODO: Include fractions on calculation
    static_cast<void>(clock_rate_fraction);
    static_cast<void>(frame_rate_fraction);

    static_cast<void>(m_frame_period_fraction);
    static_cast<void>(m_ticks_per_frame_fraction);
    static_cast<void>(m_frame_ticks_fraction);
}

void HighPrecisionSyncer::elapse(std::uint8_t ticks) {
    if (m_frame_last_ts == 0U) {
        m_frame_first_ts = now();
        m_frame_last_ts = m_frame_first_ts;
    }

    m_total_ticks += ticks;

    // Fractional compensation
    if (m_frame_ticks_fraction > 0U) {
        std::uint64_t const ticks_to_subtract =
            (ticks < m_frame_ticks_fraction)
                ? ticks
                : m_frame_ticks_fraction; 

        ticks -= ticks_to_subtract;
        m_frame_ticks_fraction -= ticks_to_subtract;
    }

    m_frame_ticks += ticks;
    if (m_frame_ticks >= m_ticks_per_frame) {
        m_frame_count += 1;
        m_frame_ticks -= m_ticks_per_frame;
        m_frame_ticks_fraction = m_ticks_per_frame_fraction;

        std::uint64_t const next_frame_ts = m_frame_last_ts + m_frame_period;
        std::uint64_t ts = now();
        std::uint64_t const busy_idle_transition_ts = ts;
        while (ts < next_frame_ts) {
            ts = now();
        }
        m_busy_period += busy_idle_transition_ts - m_frame_last_ts;
        m_idle_period += next_frame_ts - busy_idle_transition_ts;
        m_frame_last_ts = next_frame_ts;
    }
}

}
