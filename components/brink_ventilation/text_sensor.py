import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, CONF_TYPE, CONF_NAME
from . import BRINK_VENTILATION_ID, BrinkOpenTherm

TYPES = {
    "STATUS": ["Brink Status Pracy"],
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
    cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
    cv.Optional(CONF_NAME): cv.string,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    
    conf_data = TYPES[config[CONF_TYPE]]
    
    var = await text_sensor.new_text_sensor({
        "name": config.get(CONF_NAME, conf_data[0]),
    })
    
    # Wywołuje metodę set_status_text_sensor w pliku brink_ot.h
    cg.add(parent.set_status_text_sensor(var))
