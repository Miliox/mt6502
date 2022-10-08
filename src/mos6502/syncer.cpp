#include "mos6502/syncer.hpp"

#include <chrono>
#include <optional>
#include <thread>

using nanoseconds = std::chrono::nanoseconds;
using steady_clock = std::chrono::steady_clock;
using time_point = std::chrono::time_point<std::chrono::steady_clock>;

namespace mos6502
{
class Syncer::Impl {
public:
    Impl(std::uint64_t const clock_rate, std::uint64_t const frame_rate)
        : m_clock_rate(clock_rate)
        , m_frame_rate(frame_rate)
        , m_frame_period(1'000'000'000U / frame_rate)
        , m_rem_frame_period(1'000'000'000U % frame_rate)
        , m_ticks_per_frame{clock_rate / frame_rate}
        , m_rem_ticks_per_frame{clock_rate % frame_rate}
        , m_frame_ticks{}
        , m_overslept_period{}
        , m_last_frame_point{}
        {
            static_cast<void>(m_clock_rate);
            static_cast<void>(m_frame_rate);
            static_cast<void>(m_rem_ticks_per_frame);
        }

    void elapse(std::uint64_t const ticks) {
        m_frame_ticks += ticks;

        // First step initialization
        if (!m_last_frame_point.has_value()) {
            m_last_frame_point.emplace(steady_clock::now());
        }

        // Frame sync
        if (m_frame_ticks >= m_ticks_per_frame) {
            m_frame_ticks -= m_ticks_per_frame;

            time_point const pre_sync_point = steady_clock::now();

            nanoseconds const busy_period = pre_sync_point - m_last_frame_point.value();
            nanoseconds const idle_period = m_frame_period - busy_period - m_overslept_period;

            time_point post_sync_point = pre_sync_point;

            if (idle_period.count() > 0) {
                std::this_thread::sleep_for(idle_period);

                post_sync_point = steady_clock::now();

                m_overslept_period = post_sync_point - pre_sync_point - idle_period;
            } else {
                m_overslept_period = nanoseconds{};
            }

            m_last_frame_point.emplace(post_sync_point);
        }
    }

private:
    /// @brief Target clock rate
    std::uint64_t const m_clock_rate;

    /// @brief Target frame rate
    std::uint64_t const m_frame_rate;

    /// @brief Target frame period
    nanoseconds const m_frame_period;

    /// @brief Remainder time from frame period
    nanoseconds const m_rem_frame_period;

    /// @brief Target number of ticks per frame
    std::uint64_t const m_ticks_per_frame;

    /// @brief Remainder ticks left out from ticks_per_frame
    std::uint64_t const m_rem_ticks_per_frame;

    /// @brief Elapsed tick in current frame
    std::uint64_t m_frame_ticks;

    /// @brief Overslept period that needs to be compesated later 
    nanoseconds m_overslept_period;

    /// @brief The last frame synchronization time point
    std::optional<time_point> m_last_frame_point;
};
}
