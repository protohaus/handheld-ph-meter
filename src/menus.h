#pragma once

#include <Arduino.h>
#include <items.h>
#include <menuIO/chainStream.h>
#include <menuIO/keyIn.h>
#include <menuIO/serialIn.h>
#include <menuIO/serialOut.h>
#include <menuIO/softKeyIn.h>
#include <menuIO/u8g2Out.h>
#include <menuIo.h>
#include <nav.h>

#include "ios.h"

namespace sdg {

class Menus {
 public:
  Menus(Ios& ios);

  result measurement(menuOut& o, idleEvent e);
  result waterLevel(menuOut& o, idleEvent e);

 private:
  result printPh(menuOut& o, idleEvent e, int& v_offset);
  void printPhStatus(Menu::menuOut& o);
  void printPhValues(Menu::menuOut& o);

  result printEc(menuOut& o, idleEvent e, int& v_offset);
  void printEcStatus(Menu::menuOut& o);
  void printEcValues(Menu::menuOut& o);

  void printSpinner(Menu::menuOut& o);

  Ios& ios_;

  int spinner_state = 0;
  long spinner_last_update_ms = 0;
  long spinner_blink_duration_ms = 500;

  const char degree_char_ = 176;
};

}  // namespace sdg
