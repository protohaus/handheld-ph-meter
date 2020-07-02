#include "menus.h"

namespace sdg {

Menus::Menus(PhIo* phIo, EcIo* ecIo) : phIo_(phIo), ecIo_(ecIo) {}

result Menus::measurement(menuOut& o, idleEvent e) {
  int v_offset = 0;

  result ph_r = result::proceed;
  if (phIo_) {
    ph_r = printPh(o, e, v_offset);
  }

  result ec_r = result::proceed;
  if (ecIo_) {
    ec_r = printEc(o, e, v_offset);
  }

  if (!phIo_ && !ecIo_) {
    o.setCursor(0, 0);
    o.print("No EC or pH sensor");
  }

  printSpinner(o);
  
  if (ph_r == result::quit || ec_r == result::quit) {
    return result::quit;
  } else {
    return result::proceed;
  }
}

result Menus::printPh(menuOut& o, idleEvent e, int& v_offset) {
  if (phIo_) {
    switch (e) {
      case Menu::idleStart:
        phIo_->enable();
        break;
      case Menu::idling:
        o.setCursor(0, 0 + v_offset);
        printPhStatus(o);
        o.setCursor(0, 1 + v_offset);
        printPhValues(o);
        break;
      case Menu::idleEnd:
        phIo_->disable();
        break;
      default:
        break;
    }

    v_offset += 2;
  }

  return result::proceed;
}

void Menus::printPhStatus(Menu::menuOut& o) {
  if (phIo_) {
    auto ph_status = phIo_->getStatus();
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
  if (phIo_) {
    o.print(phIo_->getPh());
    o.print(" pH  ");
    o.print(phIo_->getTemperatureC(), 0);
    o.print(" ");
    o.print(degree_char_);
    o.print("C");
  } else {
    o.print("pH deaktiviert");
  }
}

result Menus::printEc(menuOut& o, idleEvent e, int& v_offset) {
  if (ecIo_) {
    switch (e) {
      case Menu::idleStart:
        ecIo_->enable();
        break;
      case Menu::idling:
        o.setCursor(0, 0 + v_offset);
        printEcStatus(o);
        o.setCursor(0, 1 + v_offset);
        printEcValues(o);
        break;
      case Menu::idleEnd:
        ecIo_->disable();
        break;
      default:
        break;
    }

    v_offset += 2;
  }

  return result::proceed;
}

void Menus::printEcStatus(Menu::menuOut& o) {
  if (ecIo_) {
    auto ec_status = ecIo_->getStatus();
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
  o.print(ecIo_->getEc(), 0);
  o.print(" uS/cm  ");
  o.print(ecIo_->getTemperatureC(), 0);
  o.print(" ");
  o.print(degree_char_);
  o.print("C");
}

void Menus::printSpinner(Menu::menuOut& o) {
  o.setCursor(16, 0);
  if(spinner_state == 0) {
    o.print(".");
  }
  if(spinner_state == 1) {
    o.print(":");
  }
  if (spinner_last_update_ms + spinner_blink_duration_ms < millis()) {
    spinner_last_update_ms = millis();
    if(spinner_state == 0) {
      spinner_state = 1;
    } else {
      spinner_state = 0;
    }
  }
}

}  // namespace sdg
