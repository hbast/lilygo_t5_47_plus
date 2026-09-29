import esphome.codegen as cg
from esphome.components import display, esp32
from esphome.components.esp32.const import VARIANT_ESP32S3
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_LAMBDA, CONF_PAGES

lilygo_t5_47_plus_ns = cg.esphome_ns.namespace("lilygo_t5_47_plus")
LilygoT5_47PlusDisplay = lilygo_t5_47_plus_ns.class_(
    "LilygoT5_47PlusDisplay", cg.PollingComponent, display.DisplayBuffer
)

DEPENDENCIES = ["esp32", "psram"]

CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(LilygoT5_47PlusDisplay),
            cv.Optional("full_update_every", default=1): cv.uint32_t,
        }
    ).extend(cv.polling_component_schema("5s")),
    cv.has_at_most_one_key(CONF_PAGES, CONF_LAMBDA),
    cv.only_with_arduino,
    esp32.only_on_variant(supported=[VARIANT_ESP32S3]),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    await display.register_display(var, config)
    cg.add(var.set_full_update_every(config["full_update_every"]))

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))
