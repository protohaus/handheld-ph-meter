#include "ec_io.h"

namespace sdg {

const std::array<uint8_t, 10> EcIo::ref_temperature_c_{5,  10, 15, 20, 25,
                                                       30, 35, 40, 45, 50};

const std::array<uint32_t, 10> EcIo::us_84_ref_{65, 67,  68,  76,  84,
                                                92, 100, 109, 116, 124};

const std::array<uint32_t, 10> EcIo::us_1413_ref_{896,  1020, 1147, 1278, 1413,
                                                  1548, 1711, 1860, 2009, 2158};

EcIo::EcIo(OneWire& one_wire, EcIo::ProbeType probe_type,
           std::function<void()> updated, uint8_t ec_sensor_i2c_address)
    : ec_sensor_(Ezo_board(ec_sensor_i2c_address, "EC")),
      probe_type_(probe_type),
      dallas_sensor_(&one_wire),
      updated_(updated) {}

void EcIo::init() {
  dallas_sensor_.begin();
  dallas_sensor_.setWaitForConversion(false);
  dallas_sensor_.setResolution(12);

  bool found_dallas_sensor = dallas_sensor_.getAddress(dallas_address_, 0);
  if (!found_dallas_sensor) {
    dallas_status_ = DallasError::DISCONNETED;
    Serial.println("Nicht temperature kompensiert");
  } else {
    dallas_status_ = DallasError::NO_DATA;
    Serial.println("Dallas sensor gefunden");
  }

  sendProbeType();
}

void EcIo::enable() { enabled_ = true; }

bool EcIo::isEnabled() { return enabled_; }

void EcIo::disable() {
  // Mark the Dallas sensor as invalid
  dallas_status_ = DallasError::DISCONNETED;
  dallas_temperature_c_ = NAN;
  dallas_request_phase_ = true;

  // Mark the pH sensor as invalid
  ec_error_ = Ezo_board::errors::FAIL;
  ec_value_ = NAN;
  reading_request_phase_ = true;

  // Mark the calibration buffer as invalid
  clearCalibration();

  enabled_ = false;
}

std::tuple<Ezo_board::errors, EcIo::DallasError, EcIo::CalibrationState>
EcIo::getStatus() {
  return std::make_tuple(ec_error_, dallas_status_, calibration_state_);
}

float EcIo::getEc() {
  if (ec_error_ == Ezo_board::SUCCESS || ec_error_ == Ezo_board::NO_DATA) {
    return ec_value_;
  } else {
    return NAN;
  }
}

float EcIo::getTemperatureC() {
  if (dallas_status_ == DallasError::SUCCESS ||
      dallas_status_ == DallasError::NO_DATA) {
    return dallas_temperature_c_;
  } else {
    return NAN;
  }
}

void EcIo::exec() {
  if (enabled_) {
    performTemperatureReading();
    performEcReading();
  }
}

void EcIo::calibrate(CalibrationState calibration_state,
                     CalibrationReference calibration_reference) {
  enable();
  calibration_state_ = calibration_state;
  calibration_reference_ = calibration_reference;
}

void EcIo::calibrate(CalibrationPoint calibration_point) {
  calibrate(std::get<0>(calibration_point), std::get<1>(calibration_point));
}

bool EcIo::isCalibrationPointDone() {
  if (stable_reading_count_ >= stable_reading_total_) {
    return true;
  } else {
    return false;
  }
}

EcIo::CalibrationPoint EcIo::getNextCalibration() {
  if (calibration_state_ == CalibrationState::TWO_POINT_LOW) {
    // Enumerate all low points that have a following high point
    if (calibration_reference_ == CalibrationReference::US_84) {
      return std::make_tuple(CalibrationState::TWO_POINT_HIGH,
                             CalibrationReference::US_1413);
    } else {
      return std::make_tuple(CalibrationState::TWO_POINT_HIGH,
                             CalibrationReference::INVALID);
    }
  } else {
    return std::make_tuple(CalibrationState::NOT_CALIBRATING,
                           CalibrationReference::INVALID);
  }
}

float EcIo::getCalibrationStdDev() { return calibration_std_dev_; }

float EcIo::getCalibrationTolerance() { return calibration_tolerance_; }

float EcIo::setCalibrationTolerance(float tolerance_std_dev) {
  if (tolerance_std_dev > 0) {
    calibration_tolerance_ = tolerance_std_dev;
  }
  return calibration_tolerance_;
}

uint8_t EcIo::getStableReadingCount() { return stable_reading_count_; }

uint8_t EcIo::getStableReadingTotal() { return stable_reading_total_; }

uint8_t EcIo::setStableReadingTotal(uint8_t stable_reading_total) {
  if (stable_reading_total > calibration_measurements_.size()) {
    stable_reading_total_ = calibration_measurements_.size();
  } else {
    stable_reading_total_ = stable_reading_total;
  }
  return stable_reading_total_;
}

float EcIo::getCalibrationTarget(bool temperature_compensate) {
  if (not temperature_compensate) {
    switch (calibration_reference_) {
      case CalibrationReference::US_84:
        return 84;
      case CalibrationReference::US_1413:
        return 1413;
      default:
        return NAN;
    }
  } else {
    // target = (high_µS - low_µS)
    //          * (current_°C - low_°C) / (high_°C - low_°C)
    //          + low_µS
    // target = (high_µS - low_µS)
    //          * scalar
    //          + low_µS
    // target = (1020 - 896)
    //          * (7 - 5) / (10 - 5)
    //          + 896
    const float& temperature_c = dallas_temperature_c_;
    uint8_t low_i = 0;

    // Check if the current temperature is in a valid range
    if (temperature_c < ref_temperature_c_.front() ||
        temperature_c > ref_temperature_c_.back()) {
      Serial.printf("Invalid temperature: %f\n", temperature_c);
      return NAN;
    }

    // Find the largest temperature index less than the current temperature
    for (int i = 0; i < ref_temperature_c_.size(); i++) {
      if (ref_temperature_c_[i] < temperature_c) {
        low_i = i;
      }
    }
    const uint8_t high_i = low_i + 1;

    // Calculate the scalar which is the same for all reference points
    float scalar = (temperature_c - ref_temperature_c_[low_i]) /
                   (ref_temperature_c_[high_i] - ref_temperature_c_[low_i]);

    // Calculate the temperature dependent reference point
    switch (calibration_reference_) {
      case CalibrationReference::US_84:
        return float(us_84_ref_[high_i] - us_84_ref_[low_i]) * scalar +
               float(us_84_ref_[low_i]);
      case CalibrationReference::US_1413:
        return float(us_1413_ref_[high_i] - us_1413_ref_[low_i]) * scalar +
               float(us_1413_ref_[low_i]);
      default:
        return NAN;
    }
  }
}

void EcIo::performTemperatureReading() {
  if (dallas_request_phase_) {
    // Request a new temperature reading
    dallas_sensor_.requestTemperaturesByAddress(dallas_address_);
    // set when the reading will be ready and switch to the receiving phase
    dallas_request_ready_time_ms_ =
        millis() + dallas_sensor_.millisToWaitForConversion(dallas_resolution_);
    dallas_request_phase_ = false;
  } else {
    // Retrieve the temperature measurement if the timeout has elapsed
    if (dallas_request_ready_time_ms_ < millis()) {
      float temp_c = dallas_sensor_.getTempC(dallas_address_);
      if (temp_c == DEVICE_DISCONNECTED_C) {
        temp_c = NAN;
        dallas_status_ = DallasError::DISCONNETED;
      } else {
        dallas_temperature_c_ = temp_c;
        dallas_status_ = DallasError::SUCCESS;
      }
      dallas_request_phase_ = true;
      updated_();
    }
  }
}

bool EcIo::completeCalibration(bool override) {
  // Create one of the following commands, where X is the temperature
  // compensated reference conductivity (µS/cm)
  // "cal,X" "cal,low,X" "cal,high,X"

  if (isCalibrationPointDone() || override) {
    std::array<char, 5> point_name;
    switch (calibration_state_) {
      case CalibrationState::SINGLE_POINT:
        strcpy(point_name.data(), "");
        break;
      case CalibrationState::TWO_POINT_LOW:
        strcpy(point_name.data(), "low,");
        break;
      case CalibrationState::TWO_POINT_HIGH:
        strcpy(point_name.data(), "high,");
        break;
      default:
        return false;
        break;
    }

    std::array<char, 16> calibration_command;
    sprintf(calibration_command.data(), "cal,%s%.0f", point_name.data(),
            getCalibrationTarget(dallas_temperature_c_));

    Serial.print(calibration_command.data());
    // ec_sensor_.send_cmd(calibration_command.data());
    disable();
    return true;
  } else {
    return false;
  }
}

void EcIo::clearCalibration() {
  calibration_state_ = CalibrationState::NOT_CALIBRATING;
  calibration_position_ = 0;
  calibration_is_full_ = false;
  calibration_std_dev_ = NAN;
  calibration_average_ = NAN;
  stable_reading_count_ = 0;
}

void EcIo::performEcReading() {
  if (reading_request_phase_) {
    // Request a new pH reading. Use temperature compensation if available
    if (dallas_status_ == DallasError::SUCCESS) {
      ec_sensor_.send_read_with_temp_comp(dallas_temperature_c_);
    } else {
      ec_sensor_.send_read_cmd();
    }
    // set when the reading will be ready and switch to the receiving phase
    next_poll_time_ms_ = millis() + response_delay_ms_;
    reading_request_phase_ = false;
  } else {
    // Retrieve the pH measurement if the timeout has elapsed
    if (millis() >= next_poll_time_ms_) {
      ec_error_ = ec_sensor_.receive_read_cmd();
      if (ec_error_ == Ezo_board::errors::SUCCESS) {
        ec_value_ = ec_sensor_.get_last_received_reading();
        if (calibration_state_ != CalibrationState::NOT_CALIBRATING) {
          addCalibrationPoint(ec_value_);
          performCalibration();
          updateStableReadingCount();
        }
      }
      // switch back to ask for a new reading
      reading_request_phase_ = true;
      updated_();
    }
  }
}

void EcIo::performCalibration() {
  // If calibration is active, only perform calculation once 2 or more values
  // have been collected
  if (calibration_state_ != CalibrationState::NOT_CALIBRATING &&
      (calibration_position_ > 1 || calibration_is_full_)) {
    // Calculate the range of measurements. If the calibration is full, the
    // oldest values will be overwritten
    CalibrationBuffer::iterator start = std::begin(calibration_measurements_);
    CalibrationBuffer::iterator end;
    uint8_t calibration_size;
    if (calibration_is_full_) {
      end = std::end(calibration_measurements_);
      calibration_size = calibration_measurements_.size();
    } else {
      end = std::begin(calibration_measurements_) + calibration_position_;
      calibration_size = calibration_position_;
    }

    double sum = std::accumulate(start, end, 0.0);
    double m = sum / calibration_size;

    double accum = 0.0;
    std::for_each(start, end,
                  [&](const double d) { accum += (d - m) * (d - m); });

    calibration_average_ = m;
    calibration_std_dev_ = sqrt(accum / (calibration_size - 1));
  }
}

void EcIo::updateStableReadingCount() {
  const float target_c = getCalibrationTarget(getTemperatureC());
  if (calibration_std_dev_ <= calibration_tolerance_ &&
      abs(target_c - calibration_average_) / target_c <
          calibration_max_offset_percent_) {
    if (stable_reading_count_ < stable_reading_total_) {
      stable_reading_count_++;
    }
  } else {
    stable_reading_count_ = 0;
  }
}

void EcIo::addCalibrationPoint(float ec_value) {
  calibration_measurements_[calibration_position_] = ec_value;
  calibration_position_++;
  if (calibration_position_ >= calibration_measurements_.size()) {
    calibration_position_ = 0;
    calibration_is_full_ = true;
  }
}

void EcIo::sendProbeType() {
  switch (probe_type_) {
    case ProbeType::K0p1:
      ec_sensor_.send_cmd("K,0.1");
      break;
    case ProbeType::K1:
      ec_sensor_.send_cmd("K,1");
      break;
    case ProbeType::K10:
      ec_sensor_.send_cmd("K,10");
      break;
    default:
      Serial.println("Unknown probe type");
      break;
  }
}

}  // namespace sdg