import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID, CONF_TYPE, CONF_NAME, DEVICE_CLASS_TEMPERATURE, 
    STATE_CLASS_MEASUREMENT, UNIT_CELSIUS
)
from . import BRINK_VENTILATION_ID, BrinkOpenTherm

TYPES = {
    "T_SUPPLY_IN": ["Brink Temp Czerpnia", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],
    "T_SUPPLY_OUT": ["Brink Temp Nawiew", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],
    "T_EXHAUST_IN": ["Brink Temp Wywiew", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],
    "T_EXHAUST_OUT": ["Brink Temp Wyrzutnia", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],
    "CURRENT_FLOW": ["Brink Przepływ", "m³/h", 0, None],
    "PRESSURE_IN": ["Brink Ciśnienie", "Pa", 0, None],
}

CONFIG_SCHEMA = sensor.sensor_schema().extend({
    cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
    cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    conf_data = TYPES[config[CONF_TYPE]]
    
    # Tworzymy obiekt sensora
    var = await sensor.new_sensor(config)
    
    # Ustawiamy parametry techniczne w sposób zrozumiały dla C++
    cg.add(var.set_unit_of_measurement(conf_data[1]))
    cg.add(var.set_accuracy_decimals(conf_data[2]))
    if conf_data[3] is not None:
        cg.add(var.set_device_class(conf_data[3]))
    
    # To naprawia Twój błąd kompilacji:
    cg.add(var.set_state_class(STATE_CLASS_MEASUREMENT))
    
    # Łączymy z odpowiednią funkcją w brink_ot.h
    cg.add(getattr(parent, f"set_{config[CONF_TYPE].lower()}_sensor")(var))
