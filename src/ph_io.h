#pragma once

#include <DallasTemperature.h>
#include <Ezo_i2c.h>
#include <Wire.h>
#include <TaskManager.h>

#include <tuple>

class PhIo : public Executable {
 public:
  enum class DallasError { SUCCESS, NO_DATA, DISCONNETED };
  enum class CalibrationState {
    NOT_CALIBRATING,
    LOW_POINT,
    MID_POINT,
    HIGH_POINT
  };
  typedef std::tuple<Ezo_board::errors, PhIo::DallasError, PhIo::CalibrationState> Status;
  typedef std::array<float, 30> CalibrationBuffer;

  PhIo(OneWire& one_wire);
  void init();
  void enable();
  bool isEnabled();
  void disable();

  
  Status getStatus();
  float getPh();
  float getTemperatureC();

  void exec();

  void calibrate(CalibrationState calibration_state);
  bool isCalibrationPointDone();
  float getCalibrationStdDev();
  float getCalibrationTolerance();
  float setCalibrationTolerance(float tolerance_std_dev);
  uint8_t getCalibrationTotal();
  uint8_t getCalibrationCount();
  uint8_t getStableReadingCount();
  uint8_t setStableReadingTotal(uint8_t stable_reading_total);
  uint8_t getStableReadingTotal();
  float getCalibrationTarget();
  bool completeCalibration(bool override=false);
  void clearCalibration();

 private:
  void performTemperatureReading();
  void performPhReading();
  void performCalibration();
  void updateStableReadingCount();
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
  CalibrationBuffer calibration_measurements_;
  uint8_t calibration_position_ = 0;
  float calibration_tolerance_ = 0.02;
  float calibration_max_offset_ = 1.5;
  float calibration_average_ = NAN;
  float calibration_std_dev_ = NAN;
  bool calibration_is_full_ = false;
  uint8_t stable_reading_count_ = 0;
  uint8_t stable_reading_total_ = 20;
};