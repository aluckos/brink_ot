import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, CONF_TYPE
from . import BRINK_VENTILATION_ID, BrinkOpenTherm

# Dodajemy CURRENT_GEAR do dostępnych typów
TYPES = {
    "STATUS": "Brink Status Pracy",
    "CURRENT_GEAR": "Brink Aktualny Bieg"
}

CONFIG_SCHEMA = text_sensor.text_sensor_schema().extend({
    cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
    cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    var = await text_sensor.new_text_sensor(config)
    
    # Dynamiczne budowanie nazwy funkcji, np. set_status_sensor lub set_current_gear_sensor
    # Uwaga: w brink_ot.h zmieniłem set_status_text_sensor na set_status_sensor dla spójności
    func_name = f"set_{config[CONF_TYPE].lower()}_sensor"
    func = getattr(parent, func_name)
    cg.add(func(var))
