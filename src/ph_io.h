#pragma once

#include <DallasTemperature.h>
#include <Ezo_i2c.h>
#include <Wire.h>

#include <tuple>

class PhIo {
 public:
  enum class DallasError { SUCCESS, NO_DATA, DISCONNETED };
  enum class CalibrationState {
    NOT_CALIBRATING,
    LOW_POINT,
    MID_POINT,
    HIGH_POINT
  };

  PhIo(OneWire& one_wire);
  void init();
  void enable();
  bool isEnabled();
  void disable();

  std::tuple<Ezo_board::errors, PhIo::DallasError, PhIo::CalibrationState>
  getStatus();
  float getCurrentPh();
  float getCurrentTemperatureC();

  void update();

  void calibrate(CalibrationState calibration_state);
  bool isCalibrationPointDone();

 private:
  void performTemperatureReading();
  void performPhReading();
  void performCalibration();
  void addCalibrationPoint(float ph_value_);

  Ezo_board PH = Ezo_board(0x63, "PH");
  DallasTemperature dallas_sensor_;
  DeviceAddress dallas_address_;
  const uint8_t dallas_resolution_ = 12;
  bool found_dallas_sensor_ = false;
  float dallas_temperature_c_ = NAN;
  DallasError dallas_status_ = DallasError::DISCONNETED;
  unsigned long dallas_request_ready_time_ms_;
  bool dallas_request_phase_ = true;

  float ph_value_ = NAN;
  Ezo_board::errors ph_error_ = Ezo_board::errors::FAIL;
  // Master on/off for enable pH meter readings
  bool enabled_ = false;

  // selects our phase
  bool reading_request_phase_ = true;
  // holds the next time we receive a response, in milliseconds
  uint32_t next_poll_time_ms_ = 0;
  // how long we wait to receive a response, in milliseconds
  const unsigned int response_delay_ms_ = 1000;

  CalibrationState calibration_state_ = CalibrationState::NOT_CALIBRATING;
  std::array<float, 20> calibration_measurements_;
  uint8_t calibration_position_ = 0;
  float calibration_tolerance = 0.02;
};