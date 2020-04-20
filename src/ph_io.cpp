#include "ph_io.h"

PhIo::PhIo(OneWire &one_wire) : dallas_sensor_(&one_wire) {}

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
  // Mark the Dallas temperature as invalid
  if (dallas_status_ == DallasError::SUCCESS) {
    dallas_status_ = DallasError::NO_DATA;
  }
  if (ph_error_ == Ezo_board::SUCCESS) {
    ph_error_ = Ezo_board::NO_DATA;
  }
  enabled_ = false;
}

std::tuple<Ezo_board::errors, PhIo::DallasError, PhIo::CalibrationState>
PhIo::getStatus() {
  return std::make_tuple(ph_error_, dallas_status_, calibration_state_);
}

float PhIo::getCurrentPh() {
  if (ph_error_ == Ezo_board::SUCCESS) {
    return ph_value_;
  } else {
    return NAN;
  }
}

float PhIo::getCurrentTemperatureC() {
  if (dallas_status_ == DallasError::SUCCESS) {
    return dallas_temperature_c_;
  } else {
    return NAN;
  }
}

void PhIo::update() {
  if (enabled_) {
    performTemperatureReading();
    performPhReading();
    performCalibration();
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
        }
      }
      // switch back to ask for a new reading
      reading_request_phase_ = true;
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
    }
  }
}

void PhIo::performCalibration() {
  if (calibration_state_ != CalibrationState::NOT_CALIBRATING) {
  }
}

void PhIo::addCalibrationPoint(float ph_value) {
  
}