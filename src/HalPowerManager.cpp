#include <cstdlib>
#include "HalPowerManager.h"

#include "HalGPIO.h"
#include "SimulatorLifecycle.h"

HalPowerManager powerManager;

void HalPowerManager::begin() {}
void HalPowerManager::startDeepSleep(HalGPIO &gpio,
                                     bool wakeOnHeldButton) const {
  if (wakeOnHeldButton)
    SimulatorLifecycle::rebootAsPowerWake();
  gpio.startDeepSleep();
}
#if FREEINK_DEVICE_PICCO
bool HalPowerManager::holdWhileUsbPowered(
    HalGPIO & /*gpio*/, void (* /*onStatus*/)(bool, uint16_t)) const {
  // No charger model: keep the existing host sleep/wake path available.
  return false;
}
#endif
void HalPowerManager::setPowerSaving(bool enable) {}
// CROSSPOINT_SIM_BATTERY=N fakes the charge level (default 100).
uint16_t HalPowerManager::getBatteryPercentage() const {
  const char *v = std::getenv("CROSSPOINT_SIM_BATTERY");
  return v ? static_cast<uint16_t>(std::atoi(v)) : 100;
}

HalPowerManager::Lock::Lock() {}
HalPowerManager::Lock::~Lock() {}
