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
# Wartość: [Domyślna nazwa, Jednostka, Dokładność, Klasa urządzenia]
TYPES = {
    # T1 - Czerpnia (ID 80)
    "T_SUPPLY_IN": ["Brink Temp Czerpnia (T1)", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],
    
    # T2 - Nawiew do domu (ID 81) - NOWOŚĆ
    "T_SUPPLY_OUT": ["Brink Temp Nawiew (T2)", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],
    
    # T3 - Wywiew z domu (ID 82)
    "T_EXHAUST_IN": ["Brink Temp Wywiew (T3)", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],
    
    # T4 - Wyrzutnia na zewnątrz (ID 83) - NOWOŚĆ
    "T_EXHAUST_OUT": ["Brink Temp Wyrzutnia (T4)", UNIT_CELSIUS, 1, DEVICE_CLASS_TEMPERATURE],
    
    # Przepływ powietrza (TSP 52/53)
    "CURRENT_FLOW": ["Brink Przepływ", "m³/h", 0, None],
}

CONFIG_SCHEMA = sensor.sensor_schema(
    state_class=STATE_CLASS_MEASUREMENT,
).extend({
    cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
    cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    conf_data = TYPES[config[CONF_TYPE]]
    
    # Tworzymy obiekt sensora
    var = await sensor.new_sensor(config)
    
    # Ustawiamy parametry poprzez konfigurację
    # (te metody są dostępne w sensor_schema)
    cg.add(var.set_accuracy_decimals(conf_data[2]))
    
    # Jednostka i klasa urządzenia są ustawiane poprzez konfigurację YAML
    # a nie poprzez metody C++ (które zostały usunięte)
    
    # Magia automatycznego łączenia nazw:
    # config[CONF_TYPE] zwraca np. "T_SUPPLY_OUT"
    # .lower() zmienia to na "t_supply_out"
    # f-string tworzy nazwę funkcji: "set_t_supply_out_sensor"
    # Ta funkcja musi istnieć w brink_ot.h (i teraz już istnieje!)
    func = getattr(parent, f"set_{config[CONF_TYPE].lower()}_sensor")
    cg.add(func(var))
