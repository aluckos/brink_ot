import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

# To wymusza na ESPHome załadowanie plików sensor.py i binary_sensor.py 
# wewnątrz Twojego komponentu.
from esphome.components import sensor, binary_sensor 

brink_ventilation_ns = cg.esphome_ns.namespace("brink_ventilation")
BrinkOpenTherm = brink_ventilation_ns.class_("BrinkOpenTherm", cg.PollingComponent)

BRINK_VENTILATION_ID = "brink_ventilation_id"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BrinkOpenTherm),
    cv.Required("in_pin"): cv.int_,
    cv.Required("out_pin"): cv.int_,
}).extend(cv.polling_component_schema("1500ms"))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_pins(config["in_pin"], config["out_pin"]))
    cg.add_library("ihormelnyk/OpenTherm Library", "1.1.5")
