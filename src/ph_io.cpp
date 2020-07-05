#include "ph_io.h"

namespace sdg {

PhIo::PhIo(OneWire &one_wire, std::function<void()> updated)
    : dallas_sensor_(&one_wire), updated_(updated) {}

void PhIo::init() {
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
}

void PhIo::enable() { enabled_ = true; }

bool PhIo::isEnabled() { return enabled_; }

void PhIo::disable() {
  // Mark the Dallas sensor as invalid
  dallas_status_ = DallasError::DISCONNETED;
  dallas_temperature_c_ = NAN;
  dallas_request_phase_ = true;

  // Mark the pH sensor as invalid
  ph_error_ = Ezo_board::errors::FAIL;
  ph_value_ = NAN;
  reading_request_phase_ = true;

  // Mark the calibration buffer as invalid
  clearCalibration();

  enabled_ = false;
}

std::tuple<Ezo_board::errors, PhIo::DallasError, PhIo::CalibrationState>
PhIo::getStatus() {
  return std::make_tuple(ph_error_, dallas_status_, calibration_state_);
}

float PhIo::getPh() {
  if (ph_error_ == Ezo_board::SUCCESS || ph_error_ == Ezo_board::NO_DATA) {
    return ph_value_;
  } else {
    return NAN;
  }
}

float PhIo::getTemperatureC() {
  if (dallas_status_ == DallasError::SUCCESS ||
      dallas_status_ == DallasError::NO_DATA) {
    return dallas_temperature_c_;
  } else {
    return NAN;
  }
}

void PhIo::exec() {
  if (enabled_) {
    performTemperatureReading();
    performPhReading();
  }
}

void PhIo::calibrate(CalibrationState calibration_state) {
  enable();
  calibration_state_ = calibration_state;
}

void PhIo::performPhReading() {
  if (reading_request_phase_) {
    // Request a new pH reading. Use temperature compensation if available
    if (dallas_status_ == DallasError::SUCCESS) {
      PH.send_read_with_temp_comp(dallas_temperature_c_);
    } else {
      PH.send_read_cmd();
    }
    // set when the reading will be ready and switch to the receiving phase
    next_poll_time_ms_ = millis() + response_delay_ms_;
    reading_request_phase_ = false;
  } else {
    // Retrieve the pH measurement if the timeout has elapsed
    if (millis() >= next_poll_time_ms_) {
      ph_error_ = PH.receive_read_cmd();
      if (ph_error_ == Ezo_board::errors::SUCCESS) {
        ph_value_ = PH.get_last_received_reading();
        if (calibration_state_ != CalibrationState::NOT_CALIBRATING) {
          addCalibrationPoint(ph_value_);
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

void PhIo::performTemperatureReading() {
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

void PhIo::performCalibration() {
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

float PhIo::getCalibrationStdDev() { return calibration_std_dev_; }

uint8_t PhIo::getCalibrationCount() {
  if (!calibration_is_full_) {
    return calibration_position_;
  } else {
    return calibration_measurements_.size();
  }
}

uint8_t PhIo::getCalibrationTotal() { return calibration_measurements_.size(); }

float PhIo::getCalibrationTarget() {
  switch (calibration_state_) {
    case CalibrationState::HIGH_POINT:
      return 10;
    case CalibrationState::MID_POINT:
      return 7;
    case CalibrationState::LOW_POINT:
      return 4;
    case CalibrationState::NOT_CALIBRATING:
      return NAN;
      break;
    default:
      return NAN;
      break;
  }
}

uint8_t PhIo::getStableReadingCount() { return stable_reading_count_; }

uint8_t PhIo::getStableReadingTotal() { return stable_reading_total_; }

bool PhIo::isCalibrationPointDone() {
  if (stable_reading_count_ >= stable_reading_total_) {
    return true;
  } else {
    return false;
  }
}

void PhIo::addCalibrationPoint(float ph_value) {
  calibration_measurements_[calibration_position_] = ph_value;
  calibration_position_++;
  if (calibration_position_ >= calibration_measurements_.size()) {
    calibration_position_ = 0;
    calibration_is_full_ = true;
  }
}

void PhIo::updateStableReadingCount() {
  if (calibration_std_dev_ <= calibration_tolerance_ &&
      abs(getCalibrationTarget() - calibration_average_) <
          calibration_max_offset_) {
    if (stable_reading_count_ < stable_reading_total_) {
      stable_reading_count_++;
    }
  } else {
    stable_reading_count_ = 0;
  }
}

bool PhIo::completeCalibration(bool override) {
  if (isCalibrationPointDone() || override) {
    std::array<char, 5> point_name;
    switch (calibration_state_) {
      case CalibrationState::LOW_POINT:
        strcpy(point_name.data(), "low");
        break;
      case CalibrationState::MID_POINT:
        strcpy(point_name.data(), "mid");
        break;
      case CalibrationState::HIGH_POINT:
        strcpy(point_name.data(), "high");
        break;
      default:
        return false;
        break;
    }

    std::array<char, 16> calibration_command;
    sprintf(calibration_command.data(), "cal,%s,%.0f", point_name.data(),
            getCalibrationTarget());

    Serial.print(calibration_command.data());
    PH.send_cmd(calibration_command.data());
    disable();
    return true;
  } else {
    return false;
  }
}

void PhIo::clearCalibration() {
  calibration_state_ = CalibrationState::NOT_CALIBRATING;
  calibration_position_ = 0;
  calibration_is_full_ = false;
  calibration_std_dev_ = NAN;
  calibration_average_ = NAN;
  stable_reading_count_ = 0;
}

float PhIo::getCalibrationTolerance() { return calibration_tolerance_; }

float PhIo::setCalibrationTolerance(float tolerance_std_dev) {
  if (tolerance_std_dev > 0) {
    calibration_tolerance_ = tolerance_std_dev;
  }
  return calibration_tolerance_;
}

uint8_t PhIo::setStableReadingTotal(uint8_t stable_reading_total) {
  if (stable_reading_total > calibration_measurements_.size()) {
    stable_reading_total_ = calibration_measurements_.size();
  } else {
    stable_reading_total_ = stable_reading_total;
  }
  return stable_reading_total_;
}

}  // namespace sdg