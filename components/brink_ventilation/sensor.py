import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_TYPE

from . import BRINK_VENTILATION_ID, BrinkOpenTherm

TYPES = ["T_SUPPLY_IN", "T_SUPPLY_OUT", "T_EXHAUST_IN", "T_EXHAUST_OUT", "CURRENT_FLOW"]

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
    cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    
    # Tworzymy tylko pustą zmienną reprezentującą sensor w C++
    # Nie używamy sensor.new_sensor(config), co blokuje automatyczne generowanie kodu w main.cpp
    var = cg.Pvariable(config[CONF_ID], cg.none, sensor.Sensor)

    func = getattr(parent, f"set_{config[CONF_TYPE].lower()}_sensor")
    cg.add(func(var))
