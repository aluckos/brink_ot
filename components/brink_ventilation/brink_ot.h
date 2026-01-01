#pragma once

#include "esphome.h"
#include "OpenTherm.h" // Zmienione z opentherm.h na OpenTherm.h

namespace esphome {
namespace brink_ventilation {

// Definicja statycznych zmiennych dla obsługi przerwań
static void IRAM_ATTR handleInterrupt() {
    // Pusta funkcja
}

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm ot;
  int in_pin, out_pin;

  sensor::Sensor *current_vent_sensor{nullptr};
  sensor::Sensor *supply_temp_sensor{nullptr};
  sensor::Sensor *exhaust_temp_sensor{nullptr};
  
  BrinkOpenTherm(int in, int out) : PollingComponent(10000), in_pin(in), out_pin(out) {}

  void setup() override {
    // Inicjalizacja biblioteki
    ot.begin(in_pin, out_pin, handleInterrupt);
  }

  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void update() override {
    // ID 77: Relative ventilation
    unsigned long response = ot.sendRequest(ot.buildRequest(OpenThermMessageType::Read_Data, 77, 0));
    if (ot.isValidResponse(response) && current_vent_sensor != nullptr) {
        current_vent_sensor->publish_state(ot.getUInt(response));
    }

    // ID 80: Supply inlet temp
    response = ot.sendRequest(ot.buildRequest(OpenThermMessageType::Read_Data, 80, 0));
    if (ot.isValidResponse(response) && supply_temp_sensor != nullptr) {
        supply_temp_sensor->publish_state(ot.getFloat(response));
    }

    // ID 82: Exhaust air temp
    response = ot.sendRequest(ot.buildRequest(OpenThermMessageType::Read_Data, 82, 0));
    if (ot.isValidResponse(response) && exhaust_temp_sensor != nullptr) {
        exhaust_temp_sensor->publish_state(ot.getFloat(response));
    }
  }

  void set_ventilation_level(float level) {
    unsigned int data = ot.temperatureToData(level);
    ot.sendRequest(ot.buildRequest(OpenThermMessageType::Write_Data, 71, data));
  }
};

class BrinkVentilationNumber : public number::Number {
 public:
  BrinkOpenTherm *parent_;
  void set_parent(BrinkOpenTherm *parent) { parent_ = parent; }
  void control(float value) override {
    this->publish_state(value);
    parent_->set_ventilation_level(value);
  }
};

} // namespace brink_ventilation
} // namespace esphome
