#pragma once

#include <Arduino.h>

#include <map>

#include "analog_io.h"
#include "ec_io.h"
#include "ph_io.h"
#include "pump_io.h"

namespace sdg {
struct Ios {
  std::map<String, EcIo> ecs;
  std::map<String, PhIo> phs;
  std::map<String, PumpIo> pumps;
  std::map<String, AnalogIo> analogs;

  bool is_updated;
  void updated() { is_updated = true; }

  void init_all() {
    for (auto& ec : ecs) {
      ec.second.init();
    }
    for (auto& ph : phs) {
      ph.second.init();
    }
    for (auto& pump : pumps) {
      pump.second.init();
    }
  }

  void handle_all() {
    for (auto& ec : ecs) {
      ec.second.exec();
    }
    for (auto& ph : phs) {
      ph.second.exec();
    }
    for(auto& analog : analogs) {
      analog.second.exec();
    }
  }
};

}  // namespace sdg
