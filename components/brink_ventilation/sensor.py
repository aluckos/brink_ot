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

# Definicje typów sensorów i ich domyślnych właściwości
TYPES = {
    "T_SUPPLY_IN": {
        "unit": UNIT_CELSIUS,
        "icon": ICON_THERMOMETER,
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "state_class": STATE_CLASS_MEASUREMENT,
        "decimals": 1,
    },
    "T_SUPPLY_OUT": {
        "unit": UNIT_CELSIUS,
        "icon": ICON_THERMOMETER,
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "state_class": STATE_CLASS_MEASUREMENT,
        "decimals": 1,
    },
    "T_EXHAUST_IN": {
        "unit": UNIT_CELSIUS,
        "icon": ICON_THERMOMETER,
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "state_class": STATE_CLASS_MEASUREMENT,
        "decimals": 1,
    },
    "T_EXHAUST_OUT": {
        "unit": UNIT_CELSIUS,
        "icon": ICON_THERMOMETER,
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "state_class": STATE_CLASS_MEASUREMENT,
        "decimals": 1,
    },
    "CURRENT_FLOW": {
        "unit": "m³/h",
        "icon": "mdi:air-filter",
        "device_class": cv.maybe_simple_value, # brak dedykowanej klasy dla przepływu m3
        "state_class": STATE_CLASS_MEASUREMENT,
        "decimals": 0,
    },
}

CONFIG_SCHEMA = sensor.sensor_schema().extend({
    cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
    cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    
    # Pobieramy ustawienia dla danego typu
    type_info = TYPES[config[CONF_TYPE]]
    
    # Tworzymy sensor z uwzględnieniem domyślnych parametrów
    var = await sensor.new_sensor(config)
    
    # Ustawiamy parametry, jeśli nie zostały nadpisane w YAML
    if not config.get("unit_of_measurement"):
        cg.add(var.set_unit_of_measurement(type_info["unit"]))
    if not config.get("icon"):
        cg.add(var.set_icon(type_info["icon"]))
    if not config.get("accuracy_decimals"):
        cg.add(var.set_accuracy_decimals(type_info["decimals"]))

    # Buduje nazwę funkcji w C++, np. set_t_supply_in_sensor
    func_name = f"set_{config[CONF_TYPE].lower()}_sensor"
    cg.add(getattr(parent, func_name)(var))
