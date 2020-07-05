#pragma once

#include <Arduino.h>

#include <functional>

namespace sdg {

class AnalogIo {
 public:
  AnalogIo(int pin, std::function<void()> updated);

  void enable();
  void disable();
  void exec();
  
  float getPercent();
  float getVoltage_mV();

 private:
  int pin_ = -1;
  uint16_t value_ = 0;
  bool enabled_ = false;
  std::function<void()> updated_;
};

}  // namespace sdg
