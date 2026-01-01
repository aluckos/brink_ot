import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    UNIT_CELSIUS, 
    ICON_THERMOMETER, 
    UNIT_PERCENT, 
    ICON_FAN,
    DEVICE_CLASS_TEMPERATURE
)
from . import brink_ns, BrinkOpenTherm, CONF_BRINK_VENTILATION_ID

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
    cv.Optional("supply_temp"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        icon=ICON_THERMOMETER,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional("exhaust_temp"): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        icon=ICON_THERMOMETER,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional("current_vent"): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        icon=ICON_FAN,
        accuracy_decimals=0,
    ),
})

def to_code(config):
    paren = yield cg.get_variable(config[CONF_BRINK_VENTILATION_ID])

    if "supply_temp" in config:
        sens = yield sensor.new_sensor(config["supply_temp"])
        cg.add(paren.set_supply_temp_sensor(sens))

    if "exhaust_temp" in config:
        sens = yield sensor.new_sensor(config["exhaust_temp"])
        cg.add(paren.set_exhaust_temp_sensor(sens))

    if "current_vent" in config:
        sens = yield sensor.new_sensor(config["current_vent"])
        cg.add(paren.set_current_vent_sensor(sens))
