#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace kp18058 {

enum RGBDimming : uint8_t { RGB_DIMMING_ANALOG = 0, RGB_DIMMING_CHOP = 1 };
enum ChopFrequency : uint8_t {
  CHOP_FREQ_4KHZ = 0,
  CHOP_FREQ_2KHZ = 1,
  CHOP_FREQ_1KHZ = 2,
  CHOP_FREQ_500HZ = 3
};

class KP18058Component;

/// One of the five constant-current outputs (10 bit grayscale).
class KP18058Channel : public output::FloatOutput {
 public:
  void set_parent(KP18058Component *parent) { this->parent_ = parent; }
  void set_channel(uint8_t channel) { this->channel_ = channel; }

 protected:
  void write_state(float state) override;

  KP18058Component *parent_{nullptr};
  uint8_t channel_{0};
};

/// One KP18058/KP18068 chip on its own 2-wire bus. Create one instance per chip.
class KP18058Component : public Component {
 public:
  static constexpr uint8_t NUM_CHANNELS = 5;

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  void set_pins(GPIOPin *data, GPIOPin *clock) {
    this->data_pin_ = data;
    this->clock_pin_ = clock;
  }
  void set_rgb_current(uint8_t v) { this->rgb_current_ = v; }
  void set_cw_current(uint8_t v) { this->cw_current_ = v; }
  void set_rgb_dimming(RGBDimming v) { this->rgb_dimming_ = v; }
  void set_ignore_ack(bool v) { this->ignore_ack_ = v; }
  void set_chop_frequency(ChopFrequency v) { this->chop_frequency_ = v; }

  /// Store a 10 bit value (0-1023); the frame is sent once from loop().
  void set_channel_value(uint8_t channel, uint16_t value);

 protected:
  // Bit-banged 2-wire bus (I2C-like, but with parity bits and no real addressing)
  void clock_(bool level);
  void data_out_(bool level);
  bool data_in_();
  bool bus_reset_();
  void bus_start_();
  void bus_stop_();
  bool bus_write_byte_(uint8_t value);
  bool send_frame_();

  GPIOPin *data_pin_{nullptr};
  GPIOPin *clock_pin_{nullptr};
  uint8_t rgb_current_{14};
  uint8_t cw_current_{30};
  RGBDimming rgb_dimming_{RGB_DIMMING_CHOP};
  ChopFrequency chop_frequency_{CHOP_FREQ_500HZ};

  uint16_t values_[NUM_CHANNELS]{};
  bool dirty_{true};
  bool bus_ready_{false};
  bool ignore_ack_{false};
  bool data_is_input_{false};
  bool failed_{false};
  uint32_t last_attempt_{0};
};

}  // namespace kp18058
}  // namespace esphome
