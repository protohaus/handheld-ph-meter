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

#include "ec_io.h"
#include "menus.h"
#include "ph_io.h"

OneWire one_wire(23);

PhIo phIo(one_wire);
EcIo ecIo(one_wire, EcIo::ProbeType::K0p1);
sdg::Menus menus(&phIo, &ecIo);

// The pin numbers don't seem to correspond to their actions...
#define SELECT_PIN 32
#define UP_PIN 33
#define DOWN_PIN 25
#define ESC_PIN 22

#define fontName u8g2_font_7x13_mf
#define fontX 7
#define fontY 16
#define offsetX 0
#define offsetY 3
#define U8_Width 128
#define U8_Height 64

U8G2_SSD1306_128X64_NONAME_1_2ND_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// define menu colors --------------------------------------------------------
// each color is in the format:
//  {{disabled normal,disabled selected},{enabled normal,enabled selected,
//  enabled editing}}
// this is a monochromatic color table
const colorDef<uint8_t> colors[6] MEMMODE = {
    {{0, 0}, {0, 1, 1}},  // bgColor
    {{1, 1}, {1, 0, 0}},  // fgColor
    {{1, 1}, {1, 0, 0}},  // valColor
    {{1, 1}, {1, 0, 0}},  // unitColor
    {{0, 1}, {0, 0, 1}},  // cursorColor
    {{1, 1}, {1, 0, 0}},  // titleColor
};

result calibrate(menuOut& o, idleEvent e) { return proceed; }

result measurement(menuOut& o, idleEvent e) { return menus.measurement(o, e); }

result doMeasurement(eventMask e, prompt& item);
result doCalibration(eventMask e, prompt& item);

MENU(mainMenu, "pH Meter", doNothing, noEvent, wrapStyle,
     OP("Messung starten", doMeasurement, enterEvent),
     OP("Kalibrieren", doCalibration, enterEvent), EXIT("<Exit"));

#define MAX_DEPTH 2

Menu::keyMap joystickBtn_map[] = {
    {-SELECT_PIN, defaultNavCodes[Menu::enterCmd].ch},
    {-ESC_PIN, defaultNavCodes[Menu::escCmd].ch},
    {-UP_PIN, defaultNavCodes[Menu::upCmd].ch},
    {-DOWN_PIN, defaultNavCodes[Menu::downCmd].ch},
};
Menu::softKeyIn<4> joystickBtns(joystickBtn_map);

Menu::serialIn serial(Serial);
Menu::menuIn* inputsList[] = {&joystickBtns, &serial};
Menu::chainStream<2> in(inputsList);  // 3 is the number of inputs

MENU_OUTPUTS(out, MAX_DEPTH,
             U8G2_OUT(u8g2, colors, fontX, fontY, offsetX, offsetY,
                      {0, 0, U8_Width / fontX, U8_Height / fontY}),
             SERIAL_OUT(Serial));

NAVROOT(nav, mainMenu, MAX_DEPTH, in, out);

result doMeasurement(eventMask e, prompt& item) {
  nav.idleOn(measurement);
  return proceed;
}

result doCalibration(eventMask e, prompt& item) {
  nav.idleOn(calibrate);
  return proceed;
}

// when menu is suspended
Menu::result idle(Menu::menuOut& o, Menu::idleEvent e) {
  o.clear();
  switch (e) {
    case Menu::idleStart:
      o.println("suspending menu!");
      break;
    case Menu::idling:
      o.println("suspended...");
      break;
    case Menu::idleEnd:
      o.println("resuming menu.");
      break;
  }
  return Menu::proceed;
}

void updateDisplay() {
  // change checking leaves more time for other tasks
  u8g2.firstPage();
  do {
    nav.doOutput();
  } while (u8g2.nextPage());
}

bool request_update = false;
const unsigned long update_duration = 400;
unsigned long last_update_ms = 0;

void updateTimer() {
  const unsigned long now_ms = millis();
  if (last_update_ms + update_duration < now_ms) {
    last_update_ms = now_ms;
    request_update = true;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  pinMode(SELECT_PIN, INPUT_PULLUP);
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(ESC_PIN, INPUT_PULLUP);

  Wire.begin(16, 17);
  Wire1.begin(27, 26);

  phIo.init();
  ecIo.init();

  u8g2.begin();
  u8g2.setFont(fontName);

  nav.idleTask = idle;
  Serial.println("setup done.");
  Serial.flush();
}

void loop() {
  updateTimer();

  // Run every 400 ms (update_duration) when measuring
  if (request_update && phIo.isEnabled()) {
    request_update = false;

    phIo.exec();
    ecIo.exec();

    nav.idleChanged = true;
    nav.refresh();
  }

  nav.doInput();

  if (nav.changed(0)) {  // only draw if menu changed for gfx device
    updateDisplay();
  }

  nav.doOutput();
}
