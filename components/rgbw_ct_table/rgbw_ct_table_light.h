// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 oxynatOr

#pragma once

#include <algorithm>
#include <vector>

#include "esphome/components/light/color_mode.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/output/float_output.h"
#include "esphome/core/component.h"

namespace esphome {
namespace rgbw_ct_table {

/// One row of the colour temperature table: the level (0..1) of each channel that gives this colour temperature
/// at full brightness.
struct CtPoint {
  float kelvin;
  float red;
  float green;
  float blue;
  float white;
};

/// RGB + colour temperature light for lamps with red, green, blue and ONE white LED.
///
/// - RGB mode: the colour is written to the three colour outputs, scaled by a gain per colour (white balance); the
///   white output is 0.
/// - Colour temperature mode: red, green, blue and white come from a calibration table (rows sorted by kelvin,
///   linear interpolation in kelvin, clamped at the ends) and are scaled by the brightness. The gains are NOT applied
///   here, the table values are absolute.
///
/// The two modes are exclusive, like the Color and White tabs of the Shelly web UI.
class RgbwCtTableLight : public light::LightOutput {
 public:
  void set_red(output::FloatOutput *red) { this->red_ = red; }
  void set_green(output::FloatOutput *green) { this->green_ = green; }
  void set_blue(output::FloatOutput *blue) { this->blue_ = blue; }
  void set_white(output::FloatOutput *white) { this->white_ = white; }

  void set_red_gain(float gain) { this->red_gain_ = gain; }
  void set_green_gain(float gain) { this->green_gain_ = gain; }
  void set_blue_gain(float gain) { this->blue_gain_ = gain; }

  /// Rows must be added in ascending kelvin order (checked by the Python validation).
  void add_table_point(float kelvin, float red, float green, float blue, float white) {
    this->table_.push_back(CtPoint{kelvin, red, green, blue, white});
  }

  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
    if (!this->table_.empty()) {
      // Mireds are inverted: the coldest table row gives the minimum, the warmest the maximum.
      traits.set_min_mireds(1000000.0f / this->table_.back().kelvin);
      traits.set_max_mireds(1000000.0f / this->table_.front().kelvin);
    }
    return traits;
  }

  void write_state(light::LightState *state) override {
    float red = 0.0f, green = 0.0f, blue = 0.0f, white = 0.0f;
    const auto mode = state->current_values.get_color_mode();
    if (mode & light::ColorCapability::RGB) {
      state->current_values_as_rgb(&red, &green, &blue);
      red *= this->red_gain_;
      green *= this->green_gain_;
      blue *= this->blue_gain_;
    } else if (mode & light::ColorCapability::COLOR_TEMPERATURE) {
      float brightness = 0.0f;
      state->current_values_as_brightness(&brightness);
      const CtPoint p = this->lookup_(state->current_values.get_color_temperature());
      red = p.red * brightness;
      green = p.green * brightness;
      blue = p.blue * brightness;
      white = p.white * brightness;
    }
    this->red_->set_level(red);
    this->green_->set_level(green);
    this->blue_->set_level(blue);
    this->white_->set_level(white);
  }

 protected:
  /// Row for a colour temperature given in mireds (clamped to the table, linear in kelvin between rows).
  CtPoint lookup_(float mireds) const {
    if (this->table_.empty())
      return CtPoint{0, 0, 0, 0, 0};
    // A light that was never set to a colour temperature has 0 mireds: use the warmest row.
    const float kelvin = mireds > 0.0f ? 1000000.0f / mireds : this->table_.front().kelvin;
    if (kelvin <= this->table_.front().kelvin)
      return this->table_.front();
    if (kelvin >= this->table_.back().kelvin)
      return this->table_.back();
    size_t i = 1;
    while (i + 1 < this->table_.size() && kelvin > this->table_[i].kelvin)
      i++;
    const CtPoint &a = this->table_[i - 1];
    const CtPoint &b = this->table_[i];
    const float t = (kelvin - a.kelvin) / (b.kelvin - a.kelvin);
    return CtPoint{kelvin, a.red + (b.red - a.red) * t, a.green + (b.green - a.green) * t,
                   a.blue + (b.blue - a.blue) * t, a.white + (b.white - a.white) * t};
  }

  output::FloatOutput *red_{nullptr};
  output::FloatOutput *green_{nullptr};
  output::FloatOutput *blue_{nullptr};
  output::FloatOutput *white_{nullptr};
  float red_gain_{1.0f};
  float green_gain_{1.0f};
  float blue_gain_{1.0f};
  std::vector<CtPoint> table_;
};

}  // namespace rgbw_ct_table
}  // namespace esphome
