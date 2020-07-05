#pragma once

#include "Arduino.h"

namespace sdg {

class PumpIo {
 public:
  PumpIo(unsigned int pin);
  void init();
  
  void toggle();
  void setState(bool state);
  bool getState();

 private:
  const unsigned int pin_;
  bool state_;
};
}  // namespace sdg
