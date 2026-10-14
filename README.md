<a id="readme-top"></a>

<!-- PROJECT SHIELDS -->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![License][license-shield]][license-url]
<br />
<h1 align="center">KP18058 <sup>WIP</sup></h1>
<p align="center">
  ESPHome component for the KP18058 / KP18068 5-channel LED constant-current driver family.
</p>

`kp18058` is an ESPHome external component for the **KP18058** (and, reportedly,
protocol-compatible **KP18068**) LED driver, found in many Tuya RGBCW smart bulbs
next to a CB2L / BK7231N module. The chip has five constant-current outputs with
10-bit grayscale each and is controlled over a proprietary, I²C-like 2-wire bus.

Every chip gets its own hub entry, so a device can drive **several chips**, each
on its own pair of pins. The five outputs are exposed as ordinary ESPHome
`FloatOutput`s and plug straight into `rgbww`, `rgbw`, `cwww`, `monochromatic`, …

> **Status:** early release. The protocol was cross-checked against the
> OpenBeken driver, the [ESPHome PR #7685](https://github.com/esphome/esphome/pull/7685)
> and the [Espressif lightbulb_driver](https://github.com/espressif/esp-iot-solution/blob/master/components/led/lightbulb_driver/drivers/kp18058/kp18058.c).
> The component compiles and runs with ESPHome 2026.9.1 on an ESP32-C3, but it
> has **not yet been verified against a powered KP18058**. Reports welcome.

> **Safety:** the KP18058 sits on the mains side of a bulb, which is not
> isolated. Never connect a bulb to the mains while it is wired to a programmer
> or an ESP. Flash and test only unpowered from the mains (3.3 V from the
> adapter), or use an isolation transformer if you know what you are doing.


Installation
------------

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/oxynatOr/esphome-kp18058
      ref: v0.1.0            # pin a release tag
    components: [kp18058]
```

For a local checkout use `type: local` with `path: components`.


Hardware Setup
--------------

The bus has two lines, **DAT** and **CLK**. It looks like I²C (start/stop
condition, 9th-bit ACK) but is *not* standard I²C: every byte carries a parity
bit and there is no device address, so the hardware I²C peripheral cannot be
used. The component bit-bangs the bus on any two GPIOs.

| Signal | Typical CB2L / BK7231N bulb pin |
| ---    | ---                             |
| CLK    | P7                              |
| DAT    | P8                              |

The channel order (which `OUTx` drives red, green, blue, cold, warm) differs from
bulb to bulb. Map it with the `channel:` option of the outputs.


Tested devices
--------------

| Manufacturer | Device | Chip | Tested on hardware |
| ---          | ---    | ---  | --- |
| –            | –      | KP18058 | not yet |
| –            | –      | KP18068 | not yet (assumed protocol-compatible) |


Component / Hub
---------------

```yaml
kp18058:
  - id: driver1
    data_pin: P8
    clock_pin: P7
    rgb_current: 22.5
    cw_current: 75
    rgb_dimming: chop
    chop_frequency: 500hz
```

### Configuration variables

- **id** (*Optional*): The id of this chip. Required when referenced from
  outputs of more than one chip.
- **data_pin** (***Required***): The DAT pin ([Pin Schema](https://esphome.io/guides/configuration-types/#pin-schema)).
- **clock_pin** (***Required***): The CLK pin.
- **rgb_current** (*Optional*, default `22.5`): Maximum current of OUT1–3 in
  mA. `1.5` to `48`, in steps of `1.5`.
- **cw_current** (*Optional*, default `75`): Maximum current of OUT4–5 in mA.
  `0` to `77.5`, in steps of `2.5`.
- **rgb_dimming** (*Optional*, default `chop`): Dimming method of OUT1–3.
  `analog` changes the current amplitude, `chop` changes the duty cycle.
- **chop_frequency** (*Optional*, default `500hz`): Chop frequency for
  `rgb_dimming: chop`. One of `4khz`, `2khz`, `1khz`, `500hz`.
- **ignore_ack** (*Optional*, default `false`): Do not treat a missing ACK as an
  error. Only meant for debugging, e.g. to check whether the chip reacts even
  though it does not acknowledge.

The component is `MULTI_CONF`: add one list entry per chip.


Output
------

```yaml
output:
  - platform: kp18058
    kp18058_id: driver1
    id: out_r
    channel: 3
```

### Configuration variables

- **id** (***Required***): The id of this output.
- **kp18058_id** (*Optional*): The chip this output belongs to. Required when
  more than one `kp18058` is configured.
- **channel** (***Required***): Output pin of the chip, `1..5` (OUT1–OUT5).
  OUT1–3 share `rgb_current`, OUT4–5 share `cw_current`.

All accept the standard ESPHome
[float output options](https://esphome.io/components/output/#config-output).

Values are sent as 10-bit grayscale (0–1023). Changes of several outputs in the
same loop cycle are combined into a **single** bus frame. When all outputs are
zero the chip is put into standby.


Examples
--------

Ready-to-use YAML in [`examples/`](examples/):

- [`kp18058-rgbcw.yaml`](examples/kp18058-rgbcw.yaml) — RGBCW bulb with one chip.

Two chips on one device:

```yaml
kp18058:
  - id: driver_a
    data_pin: GPIO4
    clock_pin: GPIO5
  - id: driver_b
    data_pin: GPIO6
    clock_pin: GPIO7

output:
  - platform: kp18058
    kp18058_id: driver_a
    id: a_out1
    channel: 1
  - platform: kp18058
    kp18058_id: driver_b
    id: b_out1
    channel: 1
```


Protocol
--------

One transfer is a start condition, 14 bytes (each followed by an ACK from the
chip) and a stop condition. Every byte holds 7 data bits in bits 7..1 and an
even-parity bit in bit 0.

| Byte | Content |
| ---  | --- |
| 0    | `1` · working mode (2 bit: standby / RGB / CW / RGBCW) · start byte address (4 bit) |
| 1    | Line compensation (enable, threshold, slope) — disabled by this component |
| 2    | Chop frequency (2 bit) · OUT1–3 max current (5 bit) |
| 3    | Chop dimming OUT1–3 · RC filter · OUT4–5 max current (5 bit) |
| 4–13 | OUT1…OUT5, two bytes each: upper 5 bits, lower 5 bits of the 10-bit value |

Current scaling: OUT1–3 = `1.5 mA × (n + 1)`, OUT4–5 = `2.5 mA × n`, with
`n = 0..31`.


Notes & troubleshooting
-----------------------

- **Log shows `Bus reset failed`:** DAT is stuck low. Check wiring and that no
  other component drives the pins.
- **Log shows `No ACK for byte …`:** the chip does not answer. Check the
  pins, swap DAT/CLK, and make sure the bus has pull-ups (the internal ones are
  enabled, but bulbs usually have external ones).
- **Always "No ACK" on a bench setup:** the chip sits on the mains side of the
  bulb. Without a powered chip (and a common ground) it can never answer. Do
  not power a bulb board from the mains for testing unless it is isolated.
- **Lamp flickers at low brightness:** try `rgb_dimming: chop` with a higher
  `chop_frequency`, or `analog`.
- **Colors are swapped:** adjust `channel:` — the OUT order is not the same on
  every bulb.
- **KP18068:** treated as protocol-compatible by the community, but not
  verified here.


Credits
-------

Protocol knowledge from the OpenBeken KP18058 driver (NewoPL), the unmerged
[ESPHome PR #7685](https://github.com/esphome/esphome/pull/7685) and the
Espressif `lightbulb_driver`. This is an independent implementation.


<!-- LICENSE -->
## License

Licensed under the [Apache License 2.0](LICENSE).

<p align="right">(<a href="#readme-top">back to top</a>)</p>


<!-- MARKDOWN LINKS & IMAGES -->
[contributors-shield]: https://img.shields.io/github/contributors/oxynatOr/esphome-kp18058.svg?style=for-the-badge
[contributors-url]: https://github.com/oxynatOr/esphome-kp18058/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/oxynatOr/esphome-kp18058.svg?style=for-the-badge
[forks-url]: https://github.com/oxynatOr/esphome-kp18058/network/members
[stars-shield]: https://img.shields.io/github/stars/oxynatOr/esphome-kp18058.svg?style=for-the-badge
[stars-url]: https://github.com/oxynatOr/esphome-kp18058/stargazers
[issues-shield]: https://img.shields.io/github/issues/oxynatOr/esphome-kp18058.svg?style=for-the-badge
[issues-url]: https://github.com/oxynatOr/esphome-kp18058/issues
[license-shield]: https://img.shields.io/github/license/oxynatOr/esphome-kp18058.svg?style=for-the-badge
[license-url]: https://github.com/oxynatOr/esphome-kp18058/blob/main/LICENSE
