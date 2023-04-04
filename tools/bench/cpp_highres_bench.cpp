#include <chrono>
#include <ctime>
#include <iostream>
#include <x86intrin.h>
#include <sys/time.h>

static void chrono_high_res_bench() {
    auto ticks_beg = _rdtsc();
    auto const beg = std::chrono::high_resolution_clock::now();
    auto ticks_end = _rdtsc();
    auto end = beg;

    constexpr int count = 10'000'000;
    for (int i = 0; i < count; ++i) {
        end = std::chrono::high_resolution_clock::now();
    }
    double average = (end - beg).count() / static_cast<double>(count);
    auto ticks = ticks_end - ticks_beg;

    std::cout << "high_resolution_clock::now() takes about " << average << "ns and " << ticks << " ticks" << std::endl;
}

static void clock_high_res_bench() {
    constexpr clockid_t kClockRealTime{};
    timespec ts{};

    auto ticks_beg = _rdtsc();
    clock_gettime(kClockRealTime, &ts);
    std::int64_t const beg{(ts.tv_sec * 1'000'000'000) + ts.tv_nsec};
    std::int64_t end = beg;
    auto ticks_end = _rdtsc();

    constexpr int count = 10'000'000;
    for (int i = 0; i < count; ++i) {
        clock_gettime(kClockRealTime, &ts);
        end = std::int64_t{ts.tv_sec * 1'000'000'000} + std::int64_t{ts.tv_nsec};
    }
    double average = (end - beg) / static_cast<double>(count);
    auto ticks = ticks_end - ticks_beg;

    std::cout << "clock_gettime takes about " << average << "ns and " << ticks << " ticks" << std::endl;
}

static void clock_high_res_np_bench() {
    constexpr clockid_t kClockRealTime{};

    auto ticks_beg = _rdtsc();
    std::uint64_t const beg = clock_gettime_nsec_np(kClockRealTime);
    std::uint64_t end = beg;
    auto ticks_end = _rdtsc();

    constexpr int count = 10'000'000;
    for (int i = 0; i < count; ++i) {
        end = clock_gettime_nsec_np(kClockRealTime);
    }
    double average = (end - beg) / static_cast<double>(count);
    auto ticks = ticks_end - ticks_beg;

    std::cout << "clock_gettime_nsec_np takes about " << average << "ns and " << ticks << " ticks" << std::endl;
}

int main() {
    std::cout << "rdtsc takes about " << -(_rdtsc() - _rdtsc()) << " ticks" << std::endl;

    chrono_high_res_bench();

    clock_high_res_bench();

    clock_high_res_np_bench();

    return 0;
}
