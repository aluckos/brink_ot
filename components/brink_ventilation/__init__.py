import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

brink_ns = cg.esphome_ns.namespace('brink_ventilation')
BrinkOpenTherm = brink_ns.class_('BrinkOpenTherm', cg.PollingComponent)

CONF_BRINK_VENTILATION_ID = "brink_ventilation_id"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BrinkOpenTherm),
    cv.Required("in_pin"): cv.int_,
    cv.Required("out_pin"): cv.int_,
}).extend(cv.polling_component_schema('10s'))

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], config['in_pin'], config['out_pin'])
    yield cg.register_component(var, config)
    cg.add_library("ihormaze/OpenTherm Library", "1.1.5")
