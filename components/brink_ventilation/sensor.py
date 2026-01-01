import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import UNIT_CELSIUS, ICON_THERMOMETER, UNIT_PERCENT, ICON_FAN
from . import brink_ns, BrinkOpenTherm

CONF_SUPPLY_TEMP = "supply_temp"
CONF_EXHAUST_TEMP = "exhaust_temp"
CONF_CURRENT_VENT = "current_vent"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_BRINK_ID): cv.use_id(BrinkOpenTherm),
    cv.Optional(CONF_SUPPLY_TEMP): sensor.sensor_schema(unit_of_measurement=UNIT_CELSIUS, icon=ICON_THERMOMETER, accuracy_decimals=1),
    cv.Optional(CONF_EXHAUST_TEMP): sensor.sensor_schema(unit_of_measurement=UNIT_CELSIUS, icon=ICON_THERMOMETER, accuracy_decimals=1),
    cv.Optional(CONF_CURRENT_VENT): sensor.sensor_schema(unit_of_measurement=UNIT_PERCENT, icon=ICON_FAN, accuracy_decimals=0),
})

def to_code(config):
    paren = yield cg.get_variable(config[CONF_BRINK_ID])
    if CONF_SUPPLY_TEMP in config:
        sens = yield sensor.new_sensor(config[CONF_SUPPLY_TEMP])
        cg.add(paren.set_supply_temp_sensor(sens))
    # Powtórz analogicznie dla exhaust i current_vent
