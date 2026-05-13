import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID, CONF_TYPE, STATE_CLASS_MEASUREMENT, UNIT_CELSIUS, CONF_UNIT_OF_MEASUREMENT
)
from . import BRINK_VENTILATION_ID, BrinkOpenTherm

TYPES = {
    "T_SUPPLY_IN": "Brink Temp Czerpnia (T1)",
    "T_SUPPLY_OUT": "Brink Temp Nawiew (T2)",
    "T_EXHAUST_IN": "Brink Temp Wywiew (T3)",
    "T_EXHAUST_OUT": "Brink Temp Wyrzutnia (T4)",
    "CURRENT_FLOW": "Brink Przepływ",
}

# Używamy podstawowego schematu sensora, ale wymuszamy brak jednostki w definicji
CONFIG_SCHEMA = sensor.sensor_schema().extend({
    cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
    cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    
    # Tworzymy sensor, ale ręcznie czyścimy jednostkę w obiekcie config
    # zanim ESPHome zacznie generować kod C++
    config_copy = config.copy()
    unit = ""
    if config_copy[CONF_TYPE] in ["T_SUPPLY_IN", "T_SUPPLY_OUT", "T_EXHAUST_IN", "T_EXHAUST_OUT"]:
        unit = UNIT_CELSIUS
    elif config_copy[CONF_TYPE] == "CURRENT_FLOW":
        unit = "m³/h"

    # To jest najważniejsza linia: oszukujemy system, że nie ma jednostki
    config_copy[CONF_UNIT_OF_MEASUREMENT] = ""
    
    var = await sensor.new_sensor(config_copy)
    
    # Ręcznie wstrzykujemy jednostkę do metadanych (nie jako wywołanie funkcji C++)
    cg.add(var.set_unit_of_measurement(unit))
    
    if config_copy[CONF_TYPE] == "CURRENT_FLOW":
        cg.add(var.set_accuracy_decimals(0))
    else:
        cg.add(var.set_accuracy_decimals(1))

    func = getattr(parent, f"set_{config[CONF_TYPE].lower()}_sensor")
    cg.add(func(var))
