// #include <Arduino.h>
// #include <items.h>
// #include <menuIO/chainStream.h>
// #include <menuIO/keyIn.h>
// #include <menuIO/serialIn.h>
// #include <menuIO/serialOut.h>
// #include <menuIO/softKeyIn.h>
// #include <menuIO/u8g2Out.h>
// #include <menuIo.h>
// #include <nav.h>

// #include "ph_io.h"
// // #include "ph_ui.h"

// #define SCL_PIN 18
// #define SDA_PIN 19

// #define ONE_WIRE_PIN 21
// OneWire one_wire(ONE_WIRE_PIN);
// PhIo phIo(one_wire);

// // The pin numbers don't seem to correspond to their actions...
// #define SELECT_PIN 27
// #define UP_PIN 14
// #define DOWN_PIN 13
// #define ESC_PIN 12

// #define fontName u8g2_font_7x13_mf
// #define fontX 7
// #define fontY 16
// #define offsetX 0
// #define offsetY 3
// #define U8_Width 128
// #define U8_Height 64

// U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// // define menu colors
// --------------------------------------------------------
// // each color is in the format:
// //  {{disabled normal,disabled selected},{enabled normal,enabled selected,
// //  enabled editing}}
// // this is a monochromatic color table
// const colorDef<uint8_t> colors[6] MEMMODE = {
//     {{0, 0}, {0, 1, 1}},  // bgColor
//     {{1, 1}, {1, 0, 0}},  // fgColor
//     {{1, 1}, {1, 0, 0}},  // valColor
//     {{1, 1}, {1, 0, 0}},  // unitColor
//     {{0, 1}, {0, 0, 1}},  // cursorColor
//     {{1, 1}, {1, 0, 0}},  // titleColor
// };

// void printPhStatus(Menu::menuOut& o) {
//   auto ph_status = phIo.getStatus();
//   switch (std::get<0>(ph_status)) {
//     case Ezo_board::SUCCESS:
//       o.print("Aktiv");
//       break;
//     case Ezo_board::FAIL:
//       o.print("Fehler");
//       break;
//     case Ezo_board::NOT_READY:
//       o.print("Warte...");
//       break;
//     case Ezo_board::NO_DATA:
//       o.print("Keine Daten");
//       break;
//     default:
//       o.print("Unbekannter Zustand");
//       break;
//   }
// }
// result calibrate(menuOut& o, idleEvent e) {
//   return proceed;
// }

// result measurement(menuOut& o, idleEvent e) {
//   const char degree = 176;

//   switch (e) {
//     case Menu::idleStart:
//       // Serial.println("Starting ph measurement");
//       phIo.enable();
//       break;
//     case Menu::idling:
//       o.setCursor(0, 0);
//       printPhStatus(o);
//       o.setCursor(0, 1);
//       o.print(phIo.getCurrentPh());
//       o.print(" pH  ");
//       o.print(phIo.getCurrentTemperatureC(), 1);
//       o.print(" ");
//       o.print(degree);
//       o.print("C");
//       o.setCursor(0, 3);
//       o.print("Escape zu beenden");
//       break;
//     case Menu::idleEnd:
//       // Serial.println("Ending ph measurement");
//       phIo.disable();
//       break;
//     default:
//       break;
//   }

//   return proceed;
// }

// result doMeasurement(eventMask e, prompt& item);
// result doCalibration(eventMask e, prompt& item);

// MENU(mainMenu, "pH Meter", doNothing, noEvent, wrapStyle,
//      OP("Messung starten", doMeasurement, enterEvent),
//      OP("Kalibrieren", doCalibration, enterEvent),
//      EXIT("<Exit"));

// #define MAX_DEPTH 2

// Menu::keyMap joystickBtn_map[] = {
//     {-SELECT_PIN, defaultNavCodes[Menu::enterCmd].ch},
//     {-ESC_PIN, defaultNavCodes[Menu::escCmd].ch},
//     {-UP_PIN, defaultNavCodes[Menu::upCmd].ch},
//     {-DOWN_PIN, defaultNavCodes[Menu::downCmd].ch},
// };
// Menu::softKeyIn<4> joystickBtns(joystickBtn_map);

// Menu::serialIn serial(Serial);
// Menu::menuIn* inputsList[] = {&joystickBtns, &serial};
// Menu::chainStream<2> in(inputsList);  // 3 is the number of inputs

// MENU_OUTPUTS(out, MAX_DEPTH,
//              U8G2_OUT(u8g2, colors, fontX, fontY, offsetX, offsetY,
//                       {0, 0, U8_Width / fontX, U8_Height / fontY}),
//              SERIAL_OUT(Serial));

// NAVROOT(nav, mainMenu, MAX_DEPTH, in, out);

// result doMeasurement(eventMask e, prompt& item) {
//   nav.idleOn(measurement);
//   return proceed;
// }

// result doCalibration(eventMask e, prompt& item) {
//   nav.idleOn(calibrate);
//   return proceed;
// }

// // when menu is suspended
// Menu::result idle(Menu::menuOut& o, Menu::idleEvent e) {
//   o.clear();
//   switch (e) {
//     case Menu::idleStart:
//       o.println("suspending menu!");
//       break;
//     case Menu::idling:
//       o.println("suspended...");
//       break;
//     case Menu::idleEnd:
//       o.println("resuming menu.");
//       break;
//   }
//   return Menu::proceed;
// }

// void updateDisplay() {
//   // change checking leaves more time for other tasks
//   u8g2.firstPage();
//   do {
//     nav.doOutput();
//   } while (u8g2.nextPage());
// }

// bool request_update = false;
// const unsigned long update_duration = 400;
// unsigned long last_update_ms = 0;

// void updateTimer() {
//   const unsigned long now_ms = millis();
//   if (last_update_ms + update_duration < now_ms) {
//     last_update_ms = now_ms;
//     request_update = true;
//   }
// }

// void setup() {
//   Serial.begin(115200);
//   while (!Serial) {
//   }

//   pinMode(SELECT_PIN, INPUT_PULLUP);
//   pinMode(UP_PIN, INPUT_PULLUP);
//   pinMode(DOWN_PIN, INPUT_PULLUP);
//   pinMode(ESC_PIN, INPUT_PULLUP);

//   Wire.begin(19, 18);
//   u8g2.begin();
//   u8g2.setFont(fontName);

//   nav.idleTask = idle;
//   Serial.println("setup done.");
//   Serial.flush();
// }

// void loop() {
//   updateTimer();

//   // Run every 400 ms (update_duration) when measuring
//   if (request_update && phIo.isEnabled()) {
//     request_update = false;

//     phIo.update();

//     nav.idleChanged = true;
//     nav.refresh();
//   }

//   nav.doInput();

//   if (nav.changed(0)) {  // only draw if menu changed for gfx device
//     updateDisplay();
//   }

//   nav.doOutput();
// }

/**
 * ESP8266 example of the *simplest* possible menu on u8g2.
 *
 * This example shows about the most basic example possible and should be useful
 * for anyone trying to get started with either adafruit graphics or u8g2. It is
 * missing title widgets, remote capabilities, EEPROM storage and many other
 * things but makes for the simplest possible starting point for a graphical
 * build.
 *
 * The circuit uses a PCF8574 for the input using a rotary encoder, a common
 * configuration.
 */

#include <IoAbstractionWire.h>
#include <Wire.h>

#include "ph_io.h"
#include "simpleU8g2_menu.h"

// the width and height of the attached OLED display.
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// Here we declare the variable using exactly the name that we used in the
// designers code generator panel for the graphics variable. The name and
// type must match exactly
// U8G2_SSD1306_128X64_NONAME_F_SW_I2C gfx(U8G2_R0, 5, 4);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C gfx(U8G2_R0, U8X8_PIN_NONE);

const uint one_wire_pin = 21;
OneWire one_wire(one_wire_pin);
PhIo phIo(one_wire);

//
// In a tcMenu application, before calling setupMenu it's your responsibility to
// ensure that the display you're going to use is ready for drawing. You also
// need to start wire if you have any I2C devices. Here I start serial for some
// printing in the callback.
//
void setup() {
  // If you use i2c devices, be sure to start wire.
  Wire.begin(19, 18);
  phIo.init();

  Serial.begin(115200);

  // start up the display. Important, the rendering expects this has been done.
  gfx.begin();

  // This is added by tcMenu Designer automatically during the first setup.
  setupMenu();

  taskManager.scheduleFixedRate(100, &phIo);
  menuCalibrationTolerance.setFromFloatingPointValue(
      phIo.getCalibrationTolerance());
  menuStableReadingTotal.setCurrentValue(phIo.getStableReadingTotal());
}

//
// In any IoAbstraction based application you'll normally use tasks via
// taskManager instead of writing code in loop. You are free to write code here
// as long as it does not delay or block execution. Otherwise task manager will
// be blocked.
//
void loop() { taskManager.runLoop(); }

void displayMeasuring(unsigned int encoderValue, RenderPressMode clicked);
void CALLBACK_FUNCTION onStartMeasuring(int id) {
  phIo.enable();
  renderer.takeOverDisplay(displayMeasuring);
}

void CALLBACK_FUNCTION onStartCalibrate7Ph(int id) {
  phIo.calibrate(PhIo::CalibrationState::MID_POINT);
  renderer.takeOverDisplay(displayMeasuring);
}

void CALLBACK_FUNCTION onStartCalibrate4Ph(int id) {
  phIo.calibrate(PhIo::CalibrationState::LOW_POINT);
  renderer.takeOverDisplay(displayMeasuring);
}

void drawPhIoStatus(PhIo::Status ph_io_status);
void drawCalibrateStatus(PhIo::Status ph_io_status);
void displayMeasuring(unsigned int encoderValue, RenderPressMode clicked) {
  PhIo::Status ph_io_status = phIo.getStatus();
  if (clicked) {
    phIo.disable();
    renderer.giveBackDisplay();
  } else {
    gfx.clearBuffer();
    drawPhIoStatus(ph_io_status);
    if (std::get<2>(ph_io_status) != PhIo::CalibrationState::NOT_CALIBRATING) {
      drawCalibrateStatus(ph_io_status);
    }
    gfx.sendBuffer();
  }
}

void drawPhIoStatus(PhIo::Status ph_io_status) {
  gfx.setCursor(0, 20);
  gfx.setFont(gfxConfig.titleFont);
  gfx.print("pH: ");
  gfx.setFont(u8g2_font_open_iconic_check_1x_t);
  switch (std::get<0>(ph_io_status)) {
    case Ezo_board::errors::SUCCESS:
      gfx.print("\x40");
      break;
    case Ezo_board::errors::NO_DATA:
      gfx.print("\x43");
      break;
    case Ezo_board::errors::FAIL:
    default:
      gfx.print("\x44");
      break;
  }

  gfx.setFont(gfxConfig.titleFont);
  gfx.print(" Temp: ");
  gfx.setFont(u8g2_font_open_iconic_check_1x_t);
  switch (std::get<1>(ph_io_status)) {
    case PhIo::DallasError::SUCCESS:
      gfx.print("\x40");
      break;
    case PhIo::DallasError::NO_DATA:
      gfx.print("\x43");
      break;
    case PhIo::DallasError::DISCONNETED:
    default:
      gfx.print("\x44");
      break;
  }

  gfx.setFont(gfxConfig.titleFont);
  gfx.printf(
      "  %4.1f\xb0"
      "C",
      phIo.getTemperatureC());

  gfx.setCursor(0, 62);
  gfx.setFont(u8g2_font_courB18_tr);
  gfx.printf("%4.2f pH", phIo.getPh());
}

void drawCalibrateStatus(PhIo::Status ph_io_status) {
  gfx.setCursor(0, 34);

  gfx.setFont(u8g2_font_open_iconic_check_1x_t);
  if (phIo.isCalibrationPointDone()) {
    phIo.completeCalibration();
    phIo.disable();
    renderer.giveBackDisplay();
    gfx.print("\x40");
  } else {
    gfx.print("\x44");
  }
  gfx.setFont(gfxConfig.titleFont);
  gfx.printf(" %.0f pH ", phIo.getCalibrationTarget());

  gfx.printf("SD:%4.2f %d/%d", phIo.getCalibrationStdDev(),
             phIo.getStableReadingCount(), phIo.getStableReadingTotal());
}

void confirmCalibration(unsigned int encoderValue, RenderPressMode clicked);
void CALLBACK_FUNCTION onConfirmCalibration(int id) {
  phIo.calibrate(PhIo::CalibrationState::LOW_POINT);
  renderer.takeOverDisplay(confirmCalibration);
}

void CALLBACK_FUNCTION onUpdateCalibrationTolerance(int id) {
  MenuItem* menu_item = menuMgr.getCurrentEditor();
  if (menu_item) {
    AnalogMenuItem* analog_menu_item = static_cast<AnalogMenuItem*>(menu_item);
    float value = phIo.setCalibrationTolerance(analog_menu_item->getAsFloatingPointValue());
    analog_menu_item->setFromFloatingPointValue(value);
  }
}

void CALLBACK_FUNCTION onUpdateStableReadingTotal(int id) {
  MenuItem* menu_item = menuMgr.getCurrentEditor();
  if (menu_item) {
    AnalogMenuItem* analog_menu_item = static_cast<AnalogMenuItem*>(menu_item);
    uint8_t value =
        phIo.setStableReadingTotal(analog_menu_item->getCurrentValue());
    analog_menu_item->setCurrentValue(value);
  }
}
