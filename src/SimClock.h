#pragma once
// One simulated clock for millis()/micros(), delay(), vTaskDelay(), FreeRTOS
// timeouts and the input layer's held-key timestamps.
//
// CROSSPOINT_SIM_SPEED=N (default 1) runs simulated time N times faster than
// the wall clock, so scripted runs that mostly wait (transitions, toasts,
// script offsets) finish sooner. Real CPU work (EPUB indexing, image decode)
// still takes wall time, so it looks N times slower to the firmware; prefer
// WAIT over fixed offsets after heavy work when fast-forwarding.
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <thread>

namespace sim_clock {

inline double speed() {
  static const double value = [] {
    const char *v = std::getenv("CROSSPOINT_SIM_SPEED");
    const double parsed = v ? std::atof(v) : 1.0;
    return parsed > 0.0 ? parsed : 1.0;
  }();
  return value;
}

inline std::chrono::steady_clock::time_point origin() {
  static const auto start = std::chrono::steady_clock::now();
  return start;
}

// Simulated microseconds since the first clock use.
inline uint64_t micros() {
  using namespace std::chrono;
  const auto real =
      duration_cast<microseconds>(steady_clock::now() - origin()).count();
  return static_cast<uint64_t>(static_cast<double>(real) * speed());
}

inline uint64_t millis() { return micros() / 1000; }

// Wall-clock duration of a simulated span.
inline std::chrono::microseconds realDuration(int64_t simMs) {
  return std::chrono::microseconds(
      static_cast<int64_t>(static_cast<double>(simMs) * 1000.0 / speed()));
}

inline void sleepMs(int64_t simMs) {
  std::this_thread::sleep_for(realDuration(simMs));
}

} // namespace sim_clock
