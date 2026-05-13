import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID, CONF_TYPE, STATE_CLASS_MEASUREMENT, UNIT_CELSIUS
)
from . import BRINK_VENTILATION_ID, BrinkOpenTherm

TYPES = {
    "T_SUPPLY_IN": "Brink Temp Czerpnia (T1)",
    "T_SUPPLY_OUT": "Brink Temp Nawiew (T2)",
    "T_EXHAUST_IN": "Brink Temp Wywiew (T3)",
    "T_EXHAUST_OUT": "Brink Temp Wyrzutnia (T4)",
    "CURRENT_FLOW": "Brink Przepływ",
}

CONFIG_SCHEMA = sensor.sensor_schema(
    state_class=STATE_CLASS_MEASUREMENT,
).extend({
    cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
    cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    
    # Kopiujemy konfigurację, aby nie zmieniać oryginału
    conf = config.copy()
    
    # KLUCZOWY FIX: Usuwamy jednostkę z konfiguracji przed wywołaniem new_sensor.
    # To powstrzyma ESPHome przed generowaniem linii ->set_unit_of_measurement() w C++
    unit = ""
    if conf[CONF_TYPE] in ["T_SUPPLY_IN", "T_SUPPLY_OUT", "T_EXHAUST_IN", "T_EXHAUST_OUT"]:
        unit = UNIT_CELSIUS
    elif conf[CONF_TYPE] == "CURRENT_FLOW":
        unit = "m³/h"
    
    # Czyścimy jednostkę w obiekcie config, żeby generator jej nie użył w C++
    conf[sensor.CONF_UNIT_OF_MEASUREMENT] = "" 
    
    var = await sensor.new_sensor(conf)
    
    # Teraz ustawiamy jednostkę "po cichu" tylko dla Home Assistant (w metadanych)
    cg.add(var.set_unit_of_measurement(unit))
    
    # Ustawiamy dokładność
    if conf[CONF_TYPE] == "CURRENT_FLOW":
        cg.add(var.set_accuracy_decimals(0))
    else:
        cg.add(var.set_accuracy_decimals(1))

    func = getattr(parent, f"set_{config[CONF_TYPE].lower()}_sensor")
    cg.add(func(var))
