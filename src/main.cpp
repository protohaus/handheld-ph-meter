#include <Arduino.h>
#include <DallasTemperature.h>
#include <Ezo_i2c.h>
#include <Wire.h>

#include <items.h>
#include <menuIo.h>
#include <nav.h>
#include <menuIO/chainStream.h>
#include <menuIO/keyIn.h>
#include <menuIO/serialIn.h>
#include <menuIO/serialOut.h>
#include <menuIO/softKeyIn.h>
#include <menuIO/u8g2Out.h>

#define SCL_PIN 18
#define SDA_PIN 19

#define ONE_WIRE_PIN 21

#define LED_PIN LED_BUILTIN

// The pin numbers don't seem to correspond to their actions...
#define SELECT_PIN 27
#define UP_PIN 14
#define DOWN_PIN 13
#define ESC_PIN 12

#define fontName u8g2_font_7x13_mf
#define fontX 7
#define fontY 16
#define offsetX 0
#define offsetY 3
#define U8_Width 128
#define U8_Height 64

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

OneWire one_wire(ONE_WIRE_PIN);
DallasTemperature dallas_sensor(&one_wire);
DeviceAddress dallas_address;
const uint8_t dallas_resolution = 12;
bool found_dallas_sensor = false;
float dallas_temperature_c = NAN;
unsigned long dallas_request_ready_time_ms;
bool dallas_request_phase = true;

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

void performTemperatureReading() {
  if (dallas_request_phase) {
    // Serial.println("Requesting temperature");

    dallas_sensor.requestTemperaturesByAddress(dallas_address);
    dallas_request_ready_time_ms =
        millis() + dallas_sensor.millisToWaitForConversion(dallas_resolution);
    dallas_request_phase = false;
  } else {
    if (dallas_request_ready_time_ms < millis()) {
      float temp_c = dallas_sensor.getTempC(dallas_address);
      if (temp_c == DEVICE_DISCONNECTED_C) {
        temp_c = NAN;
        // Serial.println("Temperature reading failed");

      } else {
        // Serial.println("Temp *C: " + String(temp_c));
        dallas_temperature_c = temp_c;
      }
      dallas_request_phase = true;
    }
  }
}

Ezo_board PH = Ezo_board(0x63, "PH");

float ph_value = NAN;
Ezo_board::errors ph_error = Ezo_board::errors::FAIL;
// Master on/off for enable pH meter readings
bool enable_ph_meter = false;

// selects our phase
bool reading_request_phase = true;
// holds the next time we receive a response, in milliseconds
uint32_t next_poll_time_ms = 0;
// how long we wait to receive a response, in milliseconds
const unsigned int response_delay_ms = 1000;

void checkPhMeter() {
  if (enable_ph_meter) {
    performTemperatureReading();

    if (reading_request_phase) {
      if (dallas_temperature_c != NAN) {
        // Serial.println("pH measurement: " + String(dallas_temperature_c));
        PH.send_read_with_temp_comp(dallas_temperature_c);
      } else {
        // Serial.println("pH measurement, no temperature");
        PH.send_read_cmd();
      }

      // set when the response will arrive
      next_poll_time_ms = millis() + response_delay_ms;
      // switch to the receiving phase
      reading_request_phase = false;
    } else {
      if (millis() >= next_poll_time_ms) {
        ph_error = PH.receive_read_cmd();

        if (ph_error == Ezo_board::errors::SUCCESS) {
          ph_value = PH.get_last_received_reading();
          // Serial.println(ph_value);
        }

        // switch back to asking for readings
        reading_request_phase = true;
      }
    }
  }
}

void printPhStatus(Menu::menuOut& o) {
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
  const char degree = 176;

  switch (e) {
    case Menu::idleStart:
      // Serial.println("Starting ph measurement");
      enable_ph_meter = true;
      break;
    case Menu::idling:
      o.setCursor(0, 0);
      printPhStatus(o);
      o.setCursor(0, 1);
      o.print(ph_value);
      o.print(" pH  ");
      o.print(dallas_temperature_c, 1);
      o.print(" ");
      o.print(degree);
      o.print("C");
      o.setCursor(0, 3);
      o.print("Escape zu beenden");
      break;
    case Menu::idleEnd:
      // Serial.println("Ending ph measurement");
      enable_ph_meter = false;
      break;
    default:
      break;
  }

  return proceed;
}

Menu::result doAlert(eventMask e, prompt& item);

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

result doAlert(eventMask e, prompt& item) {
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

  pinMode(LED_PIN, OUTPUT);

  pinMode(SELECT_PIN, INPUT_PULLUP);
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(ESC_PIN, INPUT_PULLUP);

  Wire.begin(19, 18);
  u8g2.begin();
  u8g2.setFont(fontName);

  dallas_sensor.begin();
  dallas_sensor.setWaitForConversion(false);
  dallas_sensor.setResolution(12);

  bool found_dallas_sensor = dallas_sensor.getAddress(dallas_address, 0);
  if (!found_dallas_sensor) {
    Serial.println("Nicht temperature kompensiert");
  } else {
    Serial.println("Dallas sensor gefunden");
  }

  nav.idleTask = idle;
  Serial.println("setup done.");
  Serial.flush();
}

void loop() {
  updateTimer();

  // Run every 400 ms (update_duration) when measuring
  if (request_update && enable_ph_meter) {
    request_update = false;

    checkPhMeter();

    nav.idleChanged = true;
    nav.refresh();
  }

  nav.doInput();

  digitalWrite(LED_PIN, ledCtrl);
  if (nav.changed(0)) {  // only draw if menu changed for gfx device
    updateDisplay();
  }

  nav.doOutput();
}
