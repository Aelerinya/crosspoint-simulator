#pragma once
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <thread>

#include "SimClock.h"
#include "freertos/FreeRTOS.h"

#define PROGMEM
#define ICACHE_RODATA_ATTR
#define IRAM_ATTR
#define DRAM_ATTR
#define RTC_NOINIT_ATTR
#define SET_LOOP_TASK_STACK_SIZE(bytes)
#define PGM_P const char *
#define PSTR(s) (s)

// Simulated time; CROSSPOINT_SIM_SPEED scales it (see SimClock.h).
inline unsigned long millis() {
  return static_cast<unsigned long>(sim_clock::millis());
}

inline unsigned long micros() {
  return static_cast<unsigned long>(sim_clock::micros());
}

inline void delay(unsigned long ms) { sim_clock::sleepMs(ms); }
inline void yield() { std::this_thread::yield(); }
inline void configTzTime(const char *, const char *) {}

// Native builds have no GPIO pins. Treat every input as released, matching the
// idle pull-up state used by the button diagnostics in firmware startup.
inline int digitalRead(int /*pin*/) { return 1; }

#include "HardwareSerial.h"
#include "Print.h"
#include "WString.h"

struct ESPMock {
  static constexpr uint32_t HEAP_SIZE = 1024 * 1024;

  static uint32_t heapValue(const char *name) {
    const char *value = std::getenv(name);
    if (!value || *value == '\0')
      return HEAP_SIZE;

    char *end = nullptr;
    errno = 0;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (errno == ERANGE || *end != '\0' || parsed > HEAP_SIZE)
      return HEAP_SIZE;
    return static_cast<uint32_t>(parsed);
  }

  uint32_t getFreeHeap() { return heapValue("CROSSPOINT_SIM_FREE_HEAP"); }
  void restart() {}
  uint32_t getHeapSize() { return HEAP_SIZE; }
  uint32_t getMinFreeHeap() { return getFreeHeap(); }
  uint32_t getMaxAllocHeap() {
    return std::min(heapValue("CROSSPOINT_SIM_MAX_ALLOC_HEAP"), getFreeHeap());
  }
  const char *getChipModel() const { return "simulator"; }
  uint32_t getChipRevision() const { return 0; }
  uint32_t getFlashChipSize() const { return 16U * 1024U * 1024U; }
};
extern ESPMock ESP;

inline long random(long max) { return std::rand() % max; }

template <typename A, typename B>
constexpr auto max(A a, B b) -> decltype(a > b ? a : b) {
  return a > b ? a : b;
}
template <typename A, typename B>
constexpr auto min(A a, B b) -> decltype(a < b ? a : b) {
  return a < b ? a : b;
}
