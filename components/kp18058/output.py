# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 oxynatOr

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_CHANNEL, CONF_ID, CONF_OUTPUT, CONF_PLATFORM
import esphome.final_validate as fv

from . import KP18058Component, kp18058_ns

DEPENDENCIES = ["kp18058"]

CONF_KP18058_ID = "kp18058_id"

KP18058Channel = kp18058_ns.class_("KP18058Channel", output.FloatOutput)

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.Required(CONF_ID): cv.declare_id(KP18058Channel),
        cv.GenerateID(CONF_KP18058_ID): cv.use_id(KP18058Component),
        cv.Required(CONF_CHANNEL): cv.int_range(min=1, max=5),
    }
)


def _final_validate(config):
    # Two outputs on the same OUT pin of one chip would overwrite each other silently.
    parent = str(config[CONF_KP18058_ID])
    users = [
        cfg
        for cfg in fv.full_config.get().get(CONF_OUTPUT, [])
        if cfg.get(CONF_PLATFORM) == "kp18058"
        and str(cfg[CONF_KP18058_ID]) == parent
        and cfg[CONF_CHANNEL] == config[CONF_CHANNEL]
    ]
    if len(users) > 1:
        raise cv.Invalid(
            f"Channel {config[CONF_CHANNEL]} of '{parent}' is used by more than one output"
        )


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    parent = await cg.get_variable(config[CONF_KP18058_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_CHANNEL] - 1))
    await output.register_output(var, config)
