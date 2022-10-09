#include "mos6502/syncer.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <optional>
#include <thread>
#include <utility>

using nanoseconds = std::chrono::nanoseconds;
using wall_clock = std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<wall_clock>;

namespace mos6502
{
class Syncer::Impl {
public:
    Impl(std::uint64_t const clock_rate, std::uint64_t const frame_rate, nanoseconds const spin_threshold)
        : m_clock_rate(clock_rate)
        , m_frame_rate(frame_rate)
        , m_frame_period(1'000'000'000U / frame_rate)
        , m_rem_frame_period(1'000'000'000U % frame_rate)
        , m_ticks_per_frame{clock_rate / frame_rate}
        , m_rem_ticks_per_frame{clock_rate % frame_rate}
        , m_spin_threshold{spin_threshold}
        , m_frame_ticks{}
        , m_sub_frame_ticks{}
        , m_overslept_period{}
        , m_last_frame_point{}
        , m_frame_count{}
        , m_busy_total_time{}
        , m_idle_total_time{}
        , m_jitter_total_time{}
        {
            assert(spin_threshold.count() >= 0);
            static_cast<void>(m_clock_rate);
            static_cast<void>(m_frame_rate);
            static_cast<void>(m_rem_ticks_per_frame);
        }

    void elapse(std::uint64_t const ticks) {
        m_frame_ticks += ticks;

        // First step initialization
        if (!m_last_frame_point.has_value()) {
            m_last_frame_point.emplace(wall_clock::now());
        }

        // Frame sync
        if (m_frame_ticks >= m_ticks_per_frame) {
            m_frame_ticks -= m_ticks_per_frame;
            m_frame_count += 1;

            std::uint64_t const sub_frames = std::min(m_sub_frame_ticks, m_frame_ticks);
            m_sub_frame_ticks -= sub_frames;

            if (m_sub_frame_ticks == 0) {
                m_sub_frame_ticks = m_rem_ticks_per_frame;
            }

            time_point const pre_sync_point = wall_clock::now();

            nanoseconds const busy_period = pre_sync_point - m_last_frame_point.value();
            nanoseconds const idle_period = m_frame_period - busy_period - m_overslept_period;

            time_point const sync_point = pre_sync_point + idle_period;

            time_point const wake_up_point = sync_point - m_spin_threshold;

            m_busy_total_time += busy_period;
            m_idle_total_time += idle_period;

            if (idle_period.count() > 0) {
                std::this_thread::sleep_until(wake_up_point);
                time_point post_sync_point = wall_clock::now();

                // Spin
                while (post_sync_point < sync_point) {
                    std::this_thread::yield();
                    post_sync_point = wall_clock::now();
                }

                nanoseconds const sync_period = post_sync_point - pre_sync_point;

                m_overslept_period = sync_period - idle_period;

                m_jitter_total_time += m_overslept_period;
                m_last_frame_point.emplace(post_sync_point);
            } else {
                m_overslept_period = nanoseconds{};
                m_last_frame_point.emplace(pre_sync_point);
            }
        }
    }

    std::uint64_t frame_count() const {
        return m_frame_count;
    }

    nanoseconds busy_total_time() const {
        return m_busy_total_time;
    }

    nanoseconds idle_total_time() const {
        return m_idle_total_time;
    }

    nanoseconds jitter_total_time() const {
        return m_jitter_total_time;;
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

    /// @brief The threshold where spin loop start
    nanoseconds const m_spin_threshold;

    /// @brief Elapsed tick in current frame
    std::uint64_t m_frame_ticks;

    /// @brief Elapsed sub tick in current frame
    std::uint64_t m_sub_frame_ticks;

    /// @brief Overslept period that needs to be compesated later 
    nanoseconds m_overslept_period;

    /// @brief The last frame synchronization time point
    std::optional<time_point> m_last_frame_point;

    std::uint64_t m_frame_count;
    nanoseconds m_busy_total_time;
    nanoseconds m_idle_total_time;
    nanoseconds m_jitter_total_time;
};

Syncer::Syncer(std::uint64_t const clock_rate, std::uint64_t const frame_rate, Syncer::Strategy strat)
    : m_pimpl{std::make_unique<Syncer::Impl>(clock_rate, frame_rate, [strat]() -> nanoseconds {
        switch (strat) {
        case Syncer::Strategy::Sleep:
            return nanoseconds{};
        case Syncer::Strategy::Spin:
            return nanoseconds{std::numeric_limits<std::int64_t>::max()};
        case Syncer::Strategy::Hybrid:
            return nanoseconds{2'000'000};
        }
        return nanoseconds{};
    }())} {
}

Syncer::~Syncer() = default;

void Syncer::elapse(std::uint64_t const ticks) {
    m_pimpl->elapse(ticks);
}

std::uint64_t Syncer::frame_count() const {
    return m_pimpl->frame_count();
}

std::chrono::nanoseconds Syncer::busy_total_time() const {
    return m_pimpl->busy_total_time();
}

std::chrono::nanoseconds Syncer::idle_total_time() const {
    return m_pimpl->idle_total_time();
}

std::chrono::nanoseconds Syncer::jitter_total_time() const {
    return m_pimpl->jitter_total_time();
}

}
