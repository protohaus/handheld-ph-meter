#include "pump_io.h"

namespace sdg {

PumpIo::PumpIo(unsigned int pin) : pin_(pin), state_(false) {}

void PumpIo::init() {
  pinMode(pin_, OUTPUT);

  return;
}

void PumpIo::toggle() {
  state_ = !state_;
  digitalWrite(pin_, state_);
}

void PumpIo::setState(bool state) {
  state_ = state;
  digitalWrite(pin_, state_);
}

bool PumpIo::getState() { return state_; }

}  // namespace sdg
