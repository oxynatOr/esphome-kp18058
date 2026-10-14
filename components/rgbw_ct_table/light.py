# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 oxynatOr

import esphome.codegen as cg
from esphome.components import light, output
import esphome.config_validation as cv
from esphome.const import CONF_BLUE, CONF_GREEN, CONF_OUTPUT_ID, CONF_RED, CONF_WHITE
from esphome.types import ConfigType

rgbw_ct_table_ns = cg.esphome_ns.namespace("rgbw_ct_table")
RgbwCtTableLight = rgbw_ct_table_ns.class_("RgbwCtTableLight", light.LightOutput)

CONF_RED_GAIN = "red_gain"
CONF_GREEN_GAIN = "green_gain"
CONF_BLUE_GAIN = "blue_gain"
CONF_COLOR_TEMPERATURE_TABLE = "color_temperature_table"
CONF_TABLE_MAX = "table_max"
CONF_KELVIN = "kelvin"

TABLE_ROW_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_KELVIN): cv.int_range(min=1000, max=20000),
        cv.Required(CONF_RED): cv.float_range(min=0),
        cv.Required(CONF_GREEN): cv.float_range(min=0),
        cv.Required(CONF_BLUE): cv.float_range(min=0),
        cv.Required(CONF_WHITE): cv.float_range(min=0),
    }
)


def _validate_table(config: ConfigType) -> ConfigType:
    table = config[CONF_COLOR_TEMPERATURE_TABLE]
    table_max = config[CONF_TABLE_MAX]
    last = 0
    for row in table:
        if row[CONF_KELVIN] <= last:
            raise cv.Invalid(
                f"{CONF_COLOR_TEMPERATURE_TABLE} must be sorted by {CONF_KELVIN} (ascending, no duplicates), "
                f"{row[CONF_KELVIN]} follows {last}"
            )
        last = row[CONF_KELVIN]
        for channel in (CONF_RED, CONF_GREEN, CONF_BLUE, CONF_WHITE):
            if row[channel] > table_max:
                raise cv.Invalid(
                    f"{channel} {row[channel]} at {row[CONF_KELVIN]} K is above {CONF_TABLE_MAX} ({table_max})"
                )
    return config


CONFIG_SCHEMA = cv.All(
    light.RGB_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(RgbwCtTableLight),
            cv.Required(CONF_RED): cv.use_id(output.FloatOutput),
            cv.Required(CONF_GREEN): cv.use_id(output.FloatOutput),
            cv.Required(CONF_BLUE): cv.use_id(output.FloatOutput),
            cv.Required(CONF_WHITE): cv.use_id(output.FloatOutput),
            cv.Optional(CONF_RED_GAIN, default=1.0): cv.float_range(min=0, max=1),
            cv.Optional(CONF_GREEN_GAIN, default=1.0): cv.float_range(min=0, max=1),
            cv.Optional(CONF_BLUE_GAIN, default=1.0): cv.float_range(min=0, max=1),
            cv.Optional(CONF_TABLE_MAX, default=1.0): cv.float_range(
                min=0, min_included=False
            ),
            cv.Required(CONF_COLOR_TEMPERATURE_TABLE): cv.All(
                cv.ensure_list(TABLE_ROW_SCHEMA), cv.Length(min=2)
            ),
        }
    ),
    _validate_table,
)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)

    red = await cg.get_variable(config[CONF_RED])
    cg.add(var.set_red(red))
    green = await cg.get_variable(config[CONF_GREEN])
    cg.add(var.set_green(green))
    blue = await cg.get_variable(config[CONF_BLUE])
    cg.add(var.set_blue(blue))
    white = await cg.get_variable(config[CONF_WHITE])
    cg.add(var.set_white(white))

    cg.add(var.set_red_gain(config[CONF_RED_GAIN]))
    cg.add(var.set_green_gain(config[CONF_GREEN_GAIN]))
    cg.add(var.set_blue_gain(config[CONF_BLUE_GAIN]))

    table_max = config[CONF_TABLE_MAX]
    for row in config[CONF_COLOR_TEMPERATURE_TABLE]:
        cg.add(
            var.add_table_point(
                row[CONF_KELVIN],
                row[CONF_RED] / table_max,
                row[CONF_GREEN] / table_max,
                row[CONF_BLUE] / table_max,
                row[CONF_WHITE] / table_max,
            )
        )
