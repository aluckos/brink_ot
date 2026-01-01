#pragma once

#include "esphome.h"
#include <OpenTherm.h>

using namespace esphome;

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm ot;
  int in_pin, out_pin;

  // Sensory i Sterowanie
  sensor::Sensor *current_vent_sensor{nullptr};
  sensor::Sensor *supply_temp_sensor{nullptr};
  sensor::Sensor *exhaust_temp_sensor{nullptr};
  
  BrinkOpenTherm(int in, int out) : PollingComponent(10000), in_pin(in), out_pin(out) {}

  void setup() override {
    ot.begin(in_pin, out_pin);
    ESP_LOGCONFIG("brink", "OpenTherm setup na pinach IN:%d, OUT:%d", in_pin, out_pin);
  }

  // Settery dla Pythona
  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void update() override {
    // ID 77: Aktualna moc wentylacji (Relative ventilation)
    unsigned long response = ot.sendRequest(ot.buildRequest(OpenThermMessageType::Read_Data, 77, 0));
    if (ot.isValidResponse(response) && current_vent_sensor != nullptr) {
        current_vent_sensor->publish_state(ot.getFloat(response));
    }

    // ID 80: Temperatura nawiewu
    response = ot.sendRequest(ot.buildRequest(OpenThermMessageType::Read_Data, 80, 0));
    if (ot.isValidResponse(response) && supply_temp_sensor != nullptr) {
        supply_temp_sensor->publish_state(ot.getFloat(response));
    }

    // ID 82: Temperatura wywiewu
    response = ot.sendRequest(ot.buildRequest(OpenThermMessageType::Read_Data, 82, 0));
    if (ot.isValidResponse(response) && exhaust_temp_sensor != nullptr) {
        exhaust_temp_sensor->publish_state(ot.getFloat(response));
    }
  }

  void set_ventilation_level(float level) {
    // ID 71: Sterowanie (Set Ventilation Level)
    // Konwersja na format f8.8 (wartość * 256)
    uint16_t data = ot.temperatureToData(level); 
    unsigned long response = ot.sendRequest(ot.buildRequest(OpenThermMessageType::Write_Data, 71, data));
    
    if (ot.isValidResponse(response)) {
        ESP_LOGD("brink", "Ustawiono wentylację na %.1f%%", level);
    } else {
        ESP_LOGE("brink", "Błąd zapisu wentylacji!");
    }
  }
};
