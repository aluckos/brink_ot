import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID, UNIT_PERCENT, ICON_FAN
from . import brink_ns, BrinkOpenTherm, CONF_BRINK_VENTILATION_ID

BrinkVentilationNumber = brink_ns.class_("BrinkVentilationNumber", number.Number)

CONFIG_SCHEMA = number.NUMBER_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(BrinkVentilationNumber),
    cv.GenerateID(CONF_BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
}).extend(cv.COMPONENT_SCHEMA)

def to_code(config):
    paren = yield cg.get_variable(config[CONF_BRINK_VENTILATION_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    yield number.register_number(var, config, min_value=0, max_value=100, step=1)
    cg.add(var.set_parent(paren))
