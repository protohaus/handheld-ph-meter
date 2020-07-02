#pragma once

#include <DallasTemperature.h>
#include <Ezo_i2c.h>
#include <Wire.h>

#include <tuple>

class EcIo {
 public:
  /// Probe types with K0.1, K1 and K10
  enum class ProbeType { K0p1, K1, K10 };
  enum class DallasError { SUCCESS, NO_DATA, DISCONNETED };
  enum class CalibrationState {
    NOT_CALIBRATING,
    SINGLE_POINT,
    TWO_POINT_LOW,
    TWO_POINT_HIGH,
  };
  /// The reference solutions with 84 µS and 1413 µS @ 25°C
  enum class CalibrationReference { INVALID, US_84, US_1413 };
  typedef std::tuple<Ezo_board::errors, DallasError, CalibrationState> Status;
  typedef std::tuple<CalibrationState, CalibrationReference> CalibrationPoint;
  typedef std::array<float, 30> CalibrationBuffer;

  EcIo(OneWire& one_wire, ProbeType probe_type,
       uint8_t ec_sensor_i2c_address = 0x64);
  void init();
  void enable();
  bool isEnabled();
  void disable();

  Status getStatus();
  float getEc();
  float getTemperatureC();

  void exec();

  void calibrate(CalibrationState calibration_state,
                 CalibrationReference reference);
  void calibrate(CalibrationPoint);
  bool isCalibrationPointDone();
  CalibrationPoint getNextCalibration();
  float getCalibrationStdDev();
  float getCalibrationTolerance();
  float setCalibrationTolerance(float tolerance_std_dev);
  uint8_t getStableReadingCount();
  uint8_t getStableReadingTotal();
  uint8_t setStableReadingTotal(uint8_t stable_reading_total);

  /**
   * Calculate the temperature dependent calibration target in µS/cm
   *
   * @param temperature_c The current temperature in °C
   */
  float getCalibrationTarget(bool temperature_compensate = true);
  bool completeCalibration(bool override = false);
  void clearCalibration();

 private:
  void performTemperatureReading();
  void performEcReading();
  void performCalibration();
  void updateStableReadingCount();
  void addCalibrationPoint(float ph_value);
  void sendProbeType();

  Ezo_board ec_sensor_;
  ProbeType probe_type_;
  DallasTemperature dallas_sensor_;
  DeviceAddress dallas_address_;
  const uint8_t dallas_resolution_ = 12;
  bool found_dallas_sensor_ = false;
  float dallas_temperature_c_ = NAN;
  DallasError dallas_status_ = DallasError::DISCONNETED;
  unsigned long dallas_request_ready_time_ms_;
  bool dallas_request_phase_ = true;

  float ec_value_ = NAN;
  Ezo_board::errors ec_error_ = Ezo_board::errors::FAIL;
  // Master on/off for enable pH meter readings
  bool enabled_ = false;

  // selects our phase
  bool reading_request_phase_ = true;
  // holds the next time we receive a response, in milliseconds
  uint32_t next_poll_time_ms_ = 0;
  // how long we wait to receive a response, in milliseconds
  const unsigned int response_delay_ms_ = 1000;

  CalibrationState calibration_state_ = CalibrationState::NOT_CALIBRATING;
  CalibrationReference calibration_reference_;
  CalibrationBuffer calibration_measurements_;
  uint8_t calibration_position_ = 0;
  float calibration_tolerance_ = 4;
  float calibration_max_offset_percent_ = 0.4;
  float calibration_average_ = NAN;
  float calibration_std_dev_ = NAN;
  bool calibration_is_full_ = false;
  uint8_t stable_reading_count_ = 0;
  uint8_t stable_reading_total_ = 20;

  static const std::array<uint8_t, 10> ref_temperature_c_;
  static const std::array<uint32_t, 10> us_84_ref_;
  static const std::array<uint32_t, 10> us_1413_ref_;
};