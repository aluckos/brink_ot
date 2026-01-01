import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID, UNIT_PERCENT, ICON_FAN
from . import brink_ns, BrinkOpenTherm, CONF_BRINK_VENTILATION_ID

BrinkVentilationNumber = brink_ns.class_("BrinkVentilationNumber", number.Number)

# Używamy funkcji number_schema zamiast stałej NUMBER_SCHEMA
CONFIG_SCHEMA = number.number_schema(
    BrinkVentilationNumber,
).extend({
    cv.GenerateID(CONF_BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
}).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    paren = yield cg.get_variable(config[CONF_BRINK_VENTILATION_ID])
    var = yield number.new_number(config, min_value=0, max_value=100, step=1)
    cg.add(var.set_parent(paren))
