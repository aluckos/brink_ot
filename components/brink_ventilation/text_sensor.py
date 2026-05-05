import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, CONF_TYPE
from . import BRINK_VENTILATION_ID, BrinkOpenTherm

TYPES = {"OT_STATUS": "Brink Status Pracy"}

CONFIG_SCHEMA = text_sensor.text_sensor_schema().extend({
    cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
    cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    var = await text_sensor.new_text_sensor(config)
    cg.add(parent.set_status_text_sensor(var))
