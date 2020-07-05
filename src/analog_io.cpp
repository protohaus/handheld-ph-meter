#include "analog_io.h"

namespace sdg {

AnalogIo::AnalogIo(int pin, std::function<void()> updated)
    : pin_(pin), updated_(updated) {}

void AnalogIo::enable() { enabled_ = true; }

void AnalogIo::disable() { enabled_ = false; }

void AnalogIo::exec() {
  if (enabled_) {
    uint16_t value = analogRead(pin_);
    if (value != value_) {
      value_ = value;
      updated_();
    }
  }
}

float AnalogIo::getPercent() {
  if (enabled_) {
    return value_ / 4096.0 * 100.0;
  } else {
    return NAN;
  }
}

float AnalogIo::getVoltage_mV() {
  if (enabled_) {
    return value_ / 4096.0 * 3.3;
  } else {
    return NAN;
  }
}

}  // namespace sdg
