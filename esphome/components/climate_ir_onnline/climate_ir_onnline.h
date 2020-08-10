#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace climate_ir_onnline {

// Temperature
const uint8_t TEMP_MIN = 17;  // Celsius
const uint8_t TEMP_MAX = 30;  // Celsius

class OnnlineIrClimate : public climate_ir::ClimateIR {
 public:
  OnnlineIrClimate()
      : climate_ir::ClimateIR(TEMP_MIN, TEMP_MAX, 1.0f, true, true,
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                               climate::CLIMATE_FAN_HIGH},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL}) {}

  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override {
    // Onnline has no swing/direction setting, only toggle/loop through positions
    toggle_swing_cmd_ = call.get_swing_mode().has_value();
  }

 protected:
  /// Transmit via IR the state of this climate controller.
  void transmit_state() override;

  bool toggle_swing_cmd_{false};

  void calc_checksum_(uint32_t &value);
  void transmit_(uint32_t value);

  climate::ClimateMode mode_before_{climate::CLIMATE_MODE_OFF};
};

}  // namespace climate_ir_onnline
}  // namespace esphome