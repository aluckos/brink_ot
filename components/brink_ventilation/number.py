import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID
from . import BRINK_VENTILATION_ID, BrinkOpenTherm

# Definiujemy klasę BrinkNumber w Pythonie, aby pasowała do C++
brink_ventilation_ns = cg.esphome_ns.namespace("brink_ventilation")
BrinkNumber = brink_ventilation_ns.class_("BrinkNumber", number.Number)

CONFIG_SCHEMA = number.NUMBER_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(BrinkNumber),
    cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    var = cg.new_variable(config[CONF_ID])
    await number.register_number(var, config, min_value=0, max_value=100, step=1)
    cg.add(var.set_parent(parent))
    cg.add(parent.set_ventilation_number(var))
