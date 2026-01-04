import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID, 
    CONF_TYPE, 
    UNIT_CELSIUS, 
    ICON_THERMOMETER, 
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
)
from . import BRINK_VENTILATION_ID, BrinkOpenTherm

# Definicje typów sensorów i ich właściwości
TYPES = {
    "T_SUPPLY_IN": sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        icon=ICON_THERMOMETER,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
    ),
    "T_SUPPLY_OUT": sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        icon=ICON_THERMOMETER,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
    ),
    "T_EXHAUST_IN": sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        icon=ICON_THERMOMETER,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
    ),
    "T_EXHAUST_OUT": sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        icon=ICON_THERMOMETER,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
    ),
    "CURRENT_FLOW": sensor.sensor_schema(
        unit_of_measurement="m³/h",
        icon="mdi:air-filter",
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=0,
    ),
}

CONFIG_SCHEMA = cv.TypedConfig(
    CONF_TYPE,
    {k: v.extend({cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm)}) for k, v in TYPES.items()},
)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    var = await sensor.new_sensor(config)
    
    # Buduje nazwę funkcji w C++, np. set_t_supply_in_sensor
    func_name = f"set_{config[CONF_TYPE].lower()}_sensor"
    cg.add(getattr(parent, func_name)(var))
