import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID, CONF_TYPE, CONF_NAME, DEVICE_CLASS_TEMPERATURE, 
    STATE_CLASS_MEASUREMENT, UNIT_CELSIUS
)
from . import BRINK_VENTILATION_ID, BrinkOpenTherm

# Definicja dostępnych typów sensorów
# Klucz: Nazwa typu w YAML
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
    
    # Tworzymy obiekt sensora z parametrami z YAML
    var = await sensor.new_sensor(config)
    
    # Ustawiamy dokładność dla temperatur
    sensor_type = config[CONF_TYPE]
    if sensor_type in ["T_SUPPLY_IN", "T_SUPPLY_OUT", "T_EXHAUST_IN", "T_EXHAUST_OUT"]:
        cg.add(var.set_accuracy_decimals(1))
    else:
        cg.add(var.set_accuracy_decimals(0))
    
    # Łączymy sensor z komponentem głównym (brink_ot.h)
    # config[CONF_TYPE] zwraca np. "T_SUPPLY_OUT"
    # .lower() zmienia to na "t_supply_out"
    # f-string tworzy nazwę funkcji: "set_t_supply_out_sensor"
    func = getattr(parent, f"set_{config[CONF_TYPE].lower()}_sensor")
    cg.add(func(var))
