#include "climate_ir_onnline.h"
#include "esphome/core/log.h"

namespace esphome {
namespace climate_ir_onnline {

static const char *TAG = "climate.climate_ir_onnline";

const uint32_t COMMAND_ON =     0x000400; // INVERT FOR OFF
const uint32_t COMMAND_OFF =    0xFFFBFF; // USE AND
const uint32_t COMMAND_FOLLOW = 0x000900;
const uint32_t COMMAND_TURBO =  0x000500;

const uint32_t COMMAND_DIR =    0xFF0FF0; // MASK: These must be 0, use AND

const uint32_t COMMAND_COOL =   0x00B000;
const uint32_t COMMAND_AUTO =   0x001008;
const uint32_t COMMAND_HEAT =   0x00B00C;
const uint32_t COMMAND_DRY =    0x001004;
const uint32_t COMMAND_FAN =    0x000004; // TODO: This is same as DRY?

const uint32_t FAN_AUTO =       0x00B000;
const uint32_t FAN_LOW =        0x009000;
const uint32_t FAN_MED =        0x005000;
const uint32_t FAN_HIGH =       0x003000;

// Temperature (Gray code 17-30)
const uint32_t TEMP_MASK = 0x0000F0;

// Constants
static const uint32_t HEADER_HIGH_US = 4000;
static const uint32_t HEADER_LOW_US = 4000;
static const uint32_t SPACER_US = 5500;
static const uint32_t BIT_HIGH_US = 600;
static const uint32_t BIT_ONE_LOW_US = 1600;
static const uint32_t BIT_ZERO_LOW_US = 550;

const uint16_t BITS = 24;

void OnnlineIrClimate::transmit_state() {
  uint32_t remote_state = 0xB000000;

  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      remote_state |= COMMAND_COOL;
      break;
    case climate::CLIMATE_MODE_AUTO:
      remote_state |= COMMAND_AUTO;
      break;
    case climate::CLIMATE_MODE_DRY:
      remote_state |= COMMAND_DRY;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      remote_state |= COMMAND_FAN;
      break;
    case climate::CLIMATE_MODE_HEAT:
      remote_state |= COMMAND_HEAT;
      break;
    case climate::CLIMATE_MODE_OFF:
      remote_state &= COMMAND_OFF;
      break;
  }
  if (toggle_swing_cmd_) {
    remote_state &= COMMAND_DIR;
  }

  if (this->mode == climate::CLIMATE_MODE_FAN_ONLY) {
    switch (this->fan_mode) {
      case climate::CLIMATE_FAN_HIGH:
        remote_state |= FAN_HIGH;
        break;
      case climate::CLIMATE_FAN_MEDIUM:
        remote_state |= FAN_MED;
        break;
      case climate::CLIMATE_FAN_LOW:
        remote_state |= FAN_LOW;
        break;
      case climate::CLIMATE_FAN_AUTO:
      default:
        remote_state |= FAN_AUTO;
        break;
    }
  } else {
    uint32_t temp_value[16] = {  // Gray codes, 4-bit
      0x000000,
      0x000010,
      0x000030,
      0x000020,
      0x000060,
      0x000070,
      0x000050,
      0x000040,
      0x0000C0,
      0x0000D0,
      0x0000F0,
      0x0000E0,
      0x0000A0,
      0x0000B0,
      0x000090,
      0x000080
    };
    remote_state |= temp_value[static_cast<int>(this->target_temperature - TEMP_MIN)];
  }

  transmit_(remote_state);
  this->publish_state();
}

void OnnlineIrClimate::transmit_(uint32_t value) {
  uint32_t remote_state = 0xB000000;
  ESP_LOGD(TAG, "Sending climate_ir_onnline code: 0x%02X", value);

  auto transmit = this->transmitter_->transmit();
  auto data = transmit.get_data();

  data->set_carrier_frequency(38000);
  data->reserve(2 + BITS * 4u);

  data->item(HEADER_HIGH_US, HEADER_LOW_US);

  uint16_t counter = 0;
  bool invert = false;
  for (uint32_t mask = 1UL << (BITS - 1); mask != 0; mask >>= 1) {
    counter++;
    if (counter % 8 == 0) {
      if (!invert) {
        // Repeat last 8 bits inverted
        invert = true;
        mask<<=8;
      } else {
        invert = false;
      }
    }
    if (!invert && value & mask)
      data->item(BIT_HIGH_US, BIT_ONE_LOW_US);
    else
      data->item(BIT_HIGH_US, BIT_ZERO_LOW_US);
  }

  data->item(BIT_HIGH_US, BIT_ZERO_LOW_US);
  transmit.perform();
  transmit.perform();
}

}  // namespace climate_ir_onnline
}  // namespace esphome