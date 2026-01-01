import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID, CONF_TYPE
from . import BRINK_VENTILATION_ID, BrinkOpenTherm

CONFIG_SCHEMA = number.NUMBER_SCHEMA.extend({
    cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    # Tworzymy pomocniczą klasę w C++ dla suwaka
    var = await number.new_number(config, min_value=0, max_value=100, step=1)
    cg.add(var.set_parent(parent))
    cg.add(parent.set_ventilation_number(var))
