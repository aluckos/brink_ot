import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID

# To musi pasować do Twojego namespace w C++
brink_ventilation_ns = cg.esphome_ns.namespace("brink_ventilation")
BrinkOpenTherm = brink_ventilation_ns.class_("BrinkOpenTherm", cg.PollingComponent)

BRINK_VENTILATION_ID = "brink_ventilation_id"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BrinkOpenTherm),
    cv.Required("in_pin"): pins.gpio_input_pin_number,
    cv.Required("out_pin"): pins.gpio_output_pin_number,
}).extend(cv.polling_component_schema("1500ms"))

async def to_code(config):
    var = cg.new_variable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_pins(config["in_pin"], config["out_pin"]))
