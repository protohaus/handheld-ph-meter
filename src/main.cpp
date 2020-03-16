#include <Arduino.h>
#include <Ezo_i2c.h>
#include <Wire.h>
#include <menu.h>
#include <menuIO/chainStream.h>
#include <menuIO/keyIn.h>
#include <menuIO/serialIn.h>
#include <menuIO/serialOut.h>
#include <menuIO/softKeyIn.h>
#include <menuIO/u8g2Out.h>

#define SCL_PIN 18
#define SDA_PIN 19

#define LEDPIN LED_BUILTIN
#define SELECT_PIN 13
#define UP_PIN 12
#define DOWN_PIN 14
#define ESC_PIN 27

#define fontName u8g2_font_7x13_mf
#define fontX 7
#define fontY 16
#define offsetX 0
#define offsetY 3
#define U8_Width 128
#define U8_Height 64

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// define menu colors --------------------------------------------------------
// each color is in the format:
//  {{disabled normal,disabled selected},{enabled normal,enabled selected,
//  enabled editing}}
// this is a monochromatic color table
const Menu::colorDef<uint8_t> colors[6] MEMMODE = {
    {{0, 0}, {0, 1, 1}},  // bgColor
    {{1, 1}, {1, 0, 0}},  // fgColor
    {{1, 1}, {1, 0, 0}},  // valColor
    {{1, 1}, {1, 0, 0}},  // unitColor
    {{0, 1}, {0, 0, 1}},  // cursorColor
    {{1, 1}, {1, 0, 0}},  // titleColor
};

Ezo_board PH = Ezo_board(0x63, "PH");

float ph_value = 0;
Ezo_board::errors ph_error = Ezo_board::errors::FAIL;
// Master on/off for enable pH meter readings
bool enable_ph_meter = true;

// selects our phase
bool reading_request_phase = true;
// holds the next time we receive a response, in milliseconds
uint32_t next_poll_time_ms = 0;
// how long we wait to receive a response, in milliseconds
const unsigned int response_delay_ms = 1000;

void checkPhMeter() {
  if (enable_ph_meter) {
    if (reading_request_phase) {
      PH.send_read_cmd();

      // set when the response will arrive
      next_poll_time_ms = millis() + response_delay_ms;
      // switch to the receiving phase
      reading_request_phase = false;
    } else {
      if (millis() >= next_poll_time_ms) {
        ph_error = PH.receive_read_cmd();

        if (ph_error == Ezo_board::errors::SUCCESS) {
          ph_value = PH.get_last_received_reading();
          Serial.println(ph_value);
        }

        // switch back to asking for readings
        reading_request_phase = true;
      }
    }
  }
}

void printPhStatus(menuOut& o) {
  switch (ph_error) {
    case Ezo_board::SUCCESS:
      o.print("Aktiv");
      break;
    case Ezo_board::FAIL:
      o.print("Fehler");
      break;
    case Ezo_board::NOT_READY:
      o.print("Warte...");
      break;
    case Ezo_board::NO_DATA:
      o.print("Keine Daten");
      break;
    default:
      o.print("Unbekannter Zustand");
      break;
  }
}

result alert(menuOut& o, idleEvent e) {
  switch (e) {
    case Menu::idleStart:
      enable_ph_meter = true;
      break;
    case Menu::idling:
      o.setCursor(0, 0);
      printPhStatus(o);
      o.setCursor(0, 1);
      o.print(ph_value);
      o.print(" pH");
      o.setCursor(0, 3);
      o.print("Escape zu beenden");
      break;
    case Menu::idleEnd:
      enable_ph_meter = false;
      break;
    default:
      break;
  }

  return proceed;
}

Menu::result doAlert(eventMask e, prompt& item);

Menu::result enablePhMeter() {
  enable_ph_meter = true;
  return Menu::result::proceed;
}

Menu::result disablePhMeter() {
  enable_ph_meter = true;
  return Menu::result::proceed;
}

int ledCtrl = HIGH;

Menu::result myLedOn() {
  ledCtrl = HIGH;
  return proceed;
}
Menu::result myLedOff() {
  ledCtrl = LOW;
  return proceed;
}

MENU(mainMenu, "pH Meter", doNothing, noEvent, wrapStyle,
     OP("Messung starten", doAlert, enterEvent),
     OP("LED On", myLedOn, enterEvent), OP("LED Off", myLedOff, enterEvent),
     EXIT("<Exit"));

#define MAX_DEPTH 2

Menu::keyMap joystickBtn_map[] = {
    {-SELECT_PIN, defaultNavCodes[enterCmd].ch},
    {-UP_PIN, defaultNavCodes[upCmd].ch},
    {-DOWN_PIN, defaultNavCodes[downCmd].ch},
    {-ESC_PIN, defaultNavCodes[escCmd].ch},
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

result doAlert(eventMask e, prompt& item) {
  enablePhMeter();
  nav.idleOn(alert);
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
  while (!Serial)
    ;

  pinMode(LEDPIN, OUTPUT);

  pinMode(SELECT_PIN, INPUT_PULLUP);
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(ESC_PIN, INPUT_PULLUP);

  Wire.begin(19, 18);
  u8g2.begin();
  u8g2.setFont(fontName);

  nav.idleTask = idle;
  Serial.println("setup done.");
  Serial.flush();
}

void loop() {
  updateTimer();

  if (request_update) {
    request_update = false;

    checkPhMeter();

    nav.idleChanged = true;
    nav.refresh();
  }

  nav.doInput();

  digitalWrite(LEDPIN, ledCtrl);
  if (nav.changed(0)) {  // only draw if menu changed for gfx device
    updateDisplay();
  }

  nav.doOutput();
}
