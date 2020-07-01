#include <IoAbstractionWire.h>
#include <WiFi.h>
#include <Wire.h>

#include "ec_io.h"
#include "ota.h"
#include "ph_io.h"
#include "simpleU8g2_menu.h"

const char* ssid = "PROTOHAUS";
const char* password = "PH-Wlan-2016#";

// the width and height of the attached OLED display.
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// Here we declare the variable using exactly the name that we used in the
// designers code generator panel for the graphics variable. The name and
// type must match exactly
// U8G2_SSD1306_128X64_NONAME_F_SW_I2C gfx(U8G2_R0, 5, 4);
U8G2_SSD1306_128X64_NONAME_F_2ND_HW_I2C gfx(U8G2_R0, U8X8_PIN_NONE);

const uint one_wire_pin = 18;
OneWire one_wire(one_wire_pin);
PhIo phIo(one_wire);
EcIo ecIo(one_wire, EcIo::ProbeType::K0p1);

//
// In a tcMenu application, before calling setupMenu it's your responsibility to
// ensure that the display you're going to use is ready for drawing. You also
// need to start wire if you have any I2C devices. Here I start serial for some
// printing in the callback.
//
void setup() {
  Serial.begin(115200);

  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Wire.begin(16, 17);
  Wire1.begin(27, 26);

  phIo.init();
  ecIo.init();

  // start up the display. Important, the rendering expects this has been done.
  gfx.begin();

  // This is added by tcMenu Designer automatically during the first setup.
  setupMenu();

  taskManager.scheduleFixedRate(100, &phIo);
  taskManager.scheduleFixedRate(100, &ecIo);
  // menuCalibrationTolerance.setFromFloatingPointValue(
  //     phIo.getCalibrationTolerance());
  // menuStableReadingTotal.setCurrentValue(phIo.getStableReadingTotal());
  menuEcCalibrationTolerance.setFromFloatingPointValue(
      ecIo.getCalibrationTolerance());
  menuEcStableReadingTotal.setCurrentValue(ecIo.getStableReadingTotal());

  sdg::setup_ota("bernd_box_a", "sdg");

  phIo.enable();
  ecIo.enable();
}

//
// In any IoAbstraction based application you'll normally use tasks via
// taskManager instead of writing code in loop. You are free to write code here
// as long as it does not delay or block execution. Otherwise task manager will
// be blocked.
//
unsigned long last_check = 0;

void loop() {
  sdg::handle_ota();
  taskManager.runLoop();
}

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
    if (std::get<2>(ph_io_status) != PhIo::CalibrationState::NOT_CALIBRATING)
    {
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
    AnalogMenuItem* analog_menu_item =
    static_cast<AnalogMenuItem*>(menu_item); float value =
    phIo.setCalibrationTolerance(analog_menu_item->getAsFloatingPointValue());
    analog_menu_item->setFromFloatingPointValue(value);
  }
}

void CALLBACK_FUNCTION onUpdateStableReadingTotal(int id) {
  MenuItem* menu_item = menuMgr.getCurrentEditor();
  if (menu_item) {
    AnalogMenuItem* analog_menu_item =
    static_cast<AnalogMenuItem*>(menu_item); uint8_t value =
        phIo.setStableReadingTotal(analog_menu_item->getCurrentValue());
    analog_menu_item->setCurrentValue(value);
  }
}

void displayEcMeasuring(unsigned int encoderValue, RenderPressMode clicked);
void CALLBACK_FUNCTION onStartEcMeasuring(int id) {
  ecIo.enable();
  renderer.takeOverDisplay(displayEcMeasuring);
}

void drawEcIoStatus(EcIo::Status ec_io_status);
void drawEcCalibrateStatus(EcIo::Status ec_io_status);
void displayEcMeasuring(unsigned int encoderValue, RenderPressMode clicked) {
  EcIo::Status ec_io_status = ecIo.getStatus();
  if (clicked) {
    ecIo.disable();
    renderer.giveBackDisplay();
  } else {
    gfx.clearBuffer();
    drawEcIoStatus(ec_io_status);
    if (std::get<2>(ec_io_status) != EcIo::CalibrationState::NOT_CALIBRATING) {
      drawEcCalibrateStatus(ec_io_status);
    }
    gfx.sendBuffer();
  }
}

void drawEcIoStatus(EcIo::Status ec_io_status) {
  gfx.setCursor(0, 20);
  gfx.setFont(gfxConfig.titleFont);
  gfx.print("EC:");
  gfx.setFont(u8g2_font_open_iconic_check_1x_t);
  switch (std::get<0>(ec_io_status)) {
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
  gfx.print(" Temp:");
  gfx.setFont(u8g2_font_open_iconic_check_1x_t);
  switch (std::get<1>(ec_io_status)) {
    case EcIo::DallasError::SUCCESS:
      gfx.print("\x40");
      break;
    case EcIo::DallasError::NO_DATA:
      gfx.print("\x43");
      break;
    case EcIo::DallasError::DISCONNETED:
    default:
      gfx.print("\x44");
      break;
  }

  gfx.setFont(gfxConfig.titleFont);
  gfx.printf(
      "  %4.1f\xb0"
      "C",
      ecIo.getTemperatureC());

  gfx.setCursor(0, 62);
  gfx.setFont(u8g2_font_courB18_tr);
  gfx.printf("%.0f", ecIo.getEc());
  gfx.setCursor(gfx.tx + 5, gfx.ty - 4);
  gfx.setFont(gfxConfig.titleFont);
  gfx.print("\xb5S/cm");
}

void drawEcCalibrateStatus(EcIo::Status ec_io_status) {
  gfx.setCursor(0, 34);

  gfx.setFont(u8g2_font_open_iconic_check_1x_t);
  if (ecIo.isCalibrationPointDone()) {
    // Upload calibration to sensor board
    ecIo.completeCalibration();
    // Check what the next calibration step is
    auto calibrateion_point = ecIo.getNextCalibration();
    if (std::get<0>(calibrateion_point) ==
        EcIo::CalibrationState::NOT_CALIBRATING) {
      // The calibration procedure is complete
      ecIo.disable();
      renderer.giveBackDisplay();
      gfx.print("\x40");
    } else {
      // Go to the next calibration step
      ecIo.calibrate(calibrateion_point);
    }
  } else {
    gfx.print("\x44");
  }
  gfx.setFont(gfxConfig.titleFont);
  gfx.printf(" %.0f ", ecIo.getCalibrationTarget(false));

  gfx.printf("SD:%4.2f %d/%d", ecIo.getCalibrationStdDev(),
             ecIo.getStableReadingCount(), ecIo.getStableReadingTotal());
}

void CALLBACK_FUNCTION onStartEcCalibrate84and1413(int id) {
  ecIo.calibrate(EcIo::CalibrationState::TWO_POINT_LOW,
                 EcIo::CalibrationReference::US_84);
  renderer.takeOverDisplay(displayEcMeasuring);
}

void CALLBACK_FUNCTION onStartEcCalibrate84(int id) {
  ecIo.calibrate(EcIo::CalibrationState::SINGLE_POINT,
                 EcIo::CalibrationReference::US_84);
  renderer.takeOverDisplay(displayEcMeasuring);
}

void CALLBACK_FUNCTION onStartEcCalibrate1413(int id) {
  ecIo.calibrate(EcIo::CalibrationState::SINGLE_POINT,
                 EcIo::CalibrationReference::US_1413);
  renderer.takeOverDisplay(displayEcMeasuring);
}

void CALLBACK_FUNCTION onUpdateEcCalibrationTolerance(int id) {
  MenuItem* menu_item = menuMgr.getCurrentEditor();
  if (menu_item) {
    AnalogMenuItem* analog_menu_item = static_cast<AnalogMenuItem*>(menu_item);
    float value = ecIo.setCalibrationTolerance(
        analog_menu_item->getAsFloatingPointValue());
    analog_menu_item->setFromFloatingPointValue(value);
  }
}

void CALLBACK_FUNCTION onUpdateEcStableReadingTotal(int id) {
  MenuItem* menu_item = menuMgr.getCurrentEditor();
  if (menu_item) {
    AnalogMenuItem* analog_menu_item = static_cast<AnalogMenuItem*>(menu_item);
    uint8_t value =
        ecIo.setStableReadingTotal(analog_menu_item->getCurrentValue());
    analog_menu_item->setCurrentValue(value);
  }
}
