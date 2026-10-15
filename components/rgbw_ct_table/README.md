# rgbw_ct_table - RGB + colour temperature light from a calibration table

A light platform for lamps with **red, green, blue and one white LED** that still offer a colour temperature control: the
colour temperature is made by mixing red, green and blue into the white LED, using a table that comes from the lamp's own firmware or
from your own calibration. It works with any `FloatOutput`s (for example the five outputs of the `kp18058` driver in this repository), it
does not depend on a specific chip.

Two **exclusive** color modes are offered (the Color and White tabs of many vendor apps): `RGB` and `COLOR_TEMPERATURE`. There is no
mode that mixes an RGB colour with the white LED.

- **RGB mode:** the colour (times brightness, after gamma) is written to the red, green and blue outputs, each multiplied by its gain
  (`red_gain`, `green_gain`, `blue_gain`: a white balance). White is 0.
- **Colour temperature mode:** the table row for the requested temperature (linear interpolation in kelvin between rows, clamped to the first and
  last row) gives the levels of red, green, blue and white at full brightness; they are multiplied by the brightness (after gamma). The gains are
  **not** applied in this mode: the table values are absolute.

The warmest row defines the maximum mireds (warmest light) and the coldest row the minimum mireds of the light entity, so Home Assistant's colour
temperature slider covers exactly the table.

## Configuration

```yaml
external_components:
  - source: github://oxynatOr/esphome-kp18058
    components: [kp18058, rgbw_ct_table]

light:
  - platform: rgbw_ct_table
    name: "Light"
    red: out_r              # FloatOutput ids
    green: out_g
    blue: out_b
    white: out_white
    gamma_correct: 1.0      # all options of the standard light schema work (effects, transition, restore_mode, ...)
    red_gain: 0.856         # optional, 0..1 (default 1.0), RGB mode only
    green_gain: 0.701
    blue_gain: 0.749
    table_max: 1023         # optional (default 1.0): the value that means "full" in the table below
    color_temperature_table:   # at least 2 rows, kelvin ascending
      - {kelvin: 2700, red: 0,   green: 0,   blue: 0,   white: 1023}
      - {kelvin: 6500, red: 0,   green: 659, blue: 366, white: 425}
```

`examples/rgbw-ct-table.yaml` has a full example (the table of the Shelly Multicolor Bulb E27 Gen3, taken from its stock firmware).

## Notes

- With an 8-bit or 10-bit driver and a strong gamma, the lowest brightness steps round to zero. The stock firmware this was written for uses
  a linear brightness, so `gamma_correct: 1.0` is the matching setting.
- Not implemented on purpose: an RGB colour on top of a colour temperature (ESPHome mode `RGB_COLOR_TEMPERATURE`).
- Verified: the configuration validates and compiles with ESPHome 2026.10.0-dev for an ESP32-C3. The behaviour on a real lamp is not verified yet.
