import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID, 
    CONF_TYPE, 
    CONF_NAME, 
    STATE_CLASS_MEASUREMENT, 
    UNIT_CELSIUS
)
from . import BRINK_VENTILATION_ID, BrinkOpenTherm

# Definicja dostępnych typów sensorów
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
    # Pobieramy referencję do głównego komponentu (brink_ot.h)
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    
    # Tworzymy obiekt sensora
    var = await sensor.new_sensor(config)
    
    sensor_type = config[CONF_TYPE]
    
    # --- LOGIKA POPRAWIAJĄCA BŁĄD ---
    
    if sensor_type in ["T_SUPPLY_IN", "T_SUPPLY_OUT", "T_EXHAUST_IN", "T_EXHAUST_OUT"]:
        # Ustawienia dla temperatur
        cg.add(var.set_unit_of_measurement(UNIT_CELSIUS))
        cg.add(var.set_accuracy_decimals(1))
    elif sensor_type == "CURRENT_FLOW":
        # Ustawienia dla przepływu powietrza
        cg.add(var.set_unit_of_measurement("m³/h"))
        cg.add(var.set_accuracy_decimals(0))
    
    # --- KONIEC POPRAWKI ---

    # Łączymy sensor z odpowiednią funkcją "set_..._sensor" w brink_ot.h
    func_name = f"set_{sensor_type.lower()}_sensor"
    func = getattr(parent, func_name)
    cg.add(func(var))
