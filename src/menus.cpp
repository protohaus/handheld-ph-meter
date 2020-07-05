#include "menus.h"

namespace sdg {

Menus::Menus(Ios& ios) : ios_(ios) {}

result Menus::measurement(menuOut& o, idleEvent e) {
  int v_offset = 0;

  result ph_r = result::proceed;
  if (ios_.phs.size() > 0) {
    ph_r = printPh(o, e, v_offset);
  }

  result ec_r = result::proceed;
  if (ios_.ecs.size() > 0) {
    ec_r = printEc(o, e, v_offset);
  }

  if (!ios_.ecs.size() && !ios_.ecs.size()) {
    o.setCursor(0, 1);
    o.print("Kein EC oder pH");
    o.setCursor(0, 2);
    o.print("Sensor");
  } else {
    printSpinner(o);
  }

  if (ph_r == result::quit || ec_r == result::quit) {
    return result::quit;
  } else {
    return result::proceed;
  }
}

result Menus::waterLevel(menuOut& o, idleEvent e) {
  auto water_level = ios_.analogs.find("Water Level");
  if (water_level != ios_.analogs.end()) {
    int v_offset = 1;

    switch (e) {
      case Menu::idleStart:
        water_level->second.enable();
        break;
      case Menu::idling:
        o.setCursor(0, 0);
        o.printf("Wasserstand: %.0f%%", water_level->second.getPercent());
        for (auto& pump : ios_.pumps) {
          o.setCursor(0, v_offset);
          o.printf("%s: %d", pump.first.c_str(), pump.second.getState());
          v_offset++;
        }
        break;
      case Menu::idleEnd:
        water_level->second.disable();
        break;
      default:
        break;
    }
  }

  return result::proceed;
}

result Menus::printPh(menuOut& o, idleEvent e, int& v_offset) {
  auto phIo = ios_.phs.begin();
  if (phIo != ios_.phs.end()) {
    switch (e) {
      case Menu::idleStart:
        phIo->second.enable();
        break;
      case Menu::idling:
        o.setCursor(0, 0 + v_offset);
        printPhStatus(o);
        o.setCursor(0, 1 + v_offset);
        printPhValues(o);
        break;
      case Menu::idleEnd:
        phIo->second.disable();
        break;
      default:
        break;
    }

    v_offset += 2;
  }

  return result::proceed;
}

void Menus::printPhStatus(Menu::menuOut& o) {
  auto phIo = ios_.phs.begin();
  if (phIo != ios_.phs.end()) {
    auto ph_status = phIo->second.getStatus();
    switch (std::get<0>(ph_status)) {
      case Ezo_board::SUCCESS:
        o.print("pH: Aktiv");
        break;
      case Ezo_board::FAIL:
        o.print("pH: Fehler");
        break;
      case Ezo_board::NOT_READY:
        o.print("pH: Warte...");
        break;
      case Ezo_board::NO_DATA:
        o.print("pH: Keine Daten");
        break;
      default:
        o.print("pH: Error");
        break;
    }
  } else {
    o.print("pH deaktiviert");
  }
}

void Menus::printPhValues(Menu::menuOut& o) {
  auto phIo = ios_.phs.begin();
  if (phIo != ios_.phs.end()) {
    o.print(phIo->second.getPh());
    o.print(" pH  ");
    o.print(phIo->second.getTemperatureC(), 0);
    o.print(" ");
    o.print(degree_char_);
    o.print("C");
  } else {
    o.print("pH deaktiviert");
  }
}

result Menus::printEc(menuOut& o, idleEvent e, int& v_offset) {
  auto ecIo = ios_.ecs.begin();
  if (ecIo != ios_.ecs.end()) {
    switch (e) {
      case Menu::idleStart:
        ecIo->second.enable();
        break;
      case Menu::idling:
        o.setCursor(0, 0 + v_offset);
        printEcStatus(o);
        o.setCursor(0, 1 + v_offset);
        printEcValues(o);
        break;
      case Menu::idleEnd:
        ecIo->second.disable();
        break;
      default:
        break;
    }

    v_offset += 2;
  }

  return result::proceed;
}

void Menus::printEcStatus(Menu::menuOut& o) {
  auto ecIo = ios_.ecs.begin();
  if (ecIo != ios_.ecs.end()) {
    auto ec_status = ecIo->second.getStatus();
    switch (std::get<0>(ec_status)) {
      case Ezo_board::SUCCESS:
        o.print("EC: Aktiv");
        break;
      case Ezo_board::FAIL:
        o.print("EC: Fehler");
        break;
      case Ezo_board::NOT_READY:
        o.print("EC: Warte...");
        break;
      case Ezo_board::NO_DATA:
        o.print("EC: Keine Daten");
        break;
      default:
        o.print("EC: Error");
        break;
    }
  } else {
    o.print("EC deaktiviert");
  }
}

void Menus::printEcValues(Menu::menuOut& o) {
  auto ecIo = ios_.ecs.begin();
  if (ecIo != ios_.ecs.end()) {
    o.print(ecIo->second.getEc(), 0);
    o.print(" uS/cm  ");
    o.print(ecIo->second.getTemperatureC(), 0);
    o.print(" ");
    o.print(degree_char_);
    o.print("C");
  }
}

void Menus::printSpinner(Menu::menuOut& o) {
  o.setCursor(16, 0);
  if (spinner_state == 0) {
    o.print(".");
  }
  if (spinner_state == 1) {
    o.print(":");
  }
  if (spinner_last_update_ms + spinner_blink_duration_ms < millis()) {
    spinner_last_update_ms = millis();
    if (spinner_state == 0) {
      spinner_state = 1;
    } else {
      spinner_state = 0;
    }
  }
}

}  // namespace sdg
