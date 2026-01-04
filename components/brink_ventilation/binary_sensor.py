import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    CONF_ID, CONF_TYPE, DEVICE_CLASS_PROBLEM, DEVICE_CLASS_CONNECTIVITY
)
from . import BRINK_VENTILATION_ID, BrinkOpenTherm

# Definicje typów sensorów binarnych i ich domyślnych klas urządzeń
TYPES = {
    "FILTER_STATUS": DEVICE_CLASS_PROBLEM,
    "CONNECTION_STATUS": DEVICE_CLASS_CONNECTIVITY,
}

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema().extend({
    cv.GenerateID(BRINK_VENTILATION_ID): cv.use_id(BrinkOpenTherm),
    cv.Required(CONF_TYPE): cv.one_of(*TYPES, upper=True),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    parent = await cg.get_variable(config[BRINK_VENTILATION_ID])
    
    # Tworzymy obiekt binary_sensor
    var = await binary_sensor.new_binary_sensor(config)
    
    # Ustawiamy klasę urządzenia na podstawie typu, jeśli użytkownik nie podał własnej w YAML
    if CONF_TYPE in config:
        cg.add(var.set_device_class(TYPES[config[CONF_TYPE]]))

    # Łączymy z metodą w pliku brink_ot.h
    # Typ "CONNECTION_STATUS" wywoła parent->set_connection_status_binary(var)
    func_name = f"set_{config[CONF_TYPE].lower()}_binary"
    func = getattr(parent, func_name)
    cg.add(func(var))
