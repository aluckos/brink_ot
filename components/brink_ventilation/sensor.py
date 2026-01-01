import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_TYPE,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)
from . import BRINK_VENTILATION_ID, BrinkVentilation

# Definicja typów sensorów
TYPES = {
    "T_SUPPLY_IN": sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    "T_SUPPLY_OUT": sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    "T_EXHAUST_IN": sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    "T_EXHAUST_OUT": sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    "RELATIVE_VENTILATION": sensor.sensor_schema(
        unit_of_measurement="m³/h", # Zmieniamy na m3/h bo kod składa bajty
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkVentilation),
        cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
    }
).extend(sensor.SENSOR_SCHEMA).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    var = await sensor.new_sensor(config)
    
    # Mapowanie typu z YAML na funkcję w C++
    cg.add(getattr(parent, f"set_{config[CONF_TYPE].lower()}_sensor")(var))
