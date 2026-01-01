#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

static void IRAM_ATTR handleInterrupt() { }

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot = nullptr;
  int pin_in;
  int pin_out;

  sensor::Sensor *current_vent_sensor{nullptr};
  sensor::Sensor *supply_temp_sensor{nullptr};
  sensor::Sensor *exhaust_temp_sensor{nullptr};
  
  float target_ventilation = 0.0f;

  BrinkOpenTherm(int in, int out) : PollingComponent(2000), pin_in(in), pin_out(out) {}

  // Przywracamy brakujące metody set_, których szuka kompilator w main.cpp
  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void setup() override {
    pinMode(pin_in, INPUT);
    pinMode(pin_out, OUTPUT);
    ot = new OpenTherm(pin_in, pin_out);
    
    // Zgodnie z błędem - tylko jeden argument
    ot->begin(handleInterrupt); 
    
    ESP_LOGI("brink", "Inicjalizacja OpenTherm na pinach IN:%d, OUT:%d", pin_in, pin_out);
  }

  void update() override {
    static int step = 0;
    unsigned long response;

    switch(step) {
      case 0: // Status (ID 0)
        ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));
        step++;
        break;

      case 1: // Odczyt mocy (ID 77)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)77, 0));
        if (ot->isValidResponse(response)) {
          float val = ot->getUInt(response);
          if (current_vent_sensor != nullptr) current_vent_sensor->publish_state(val);
        }
        step++;
        break;

      case 2: // Temperatura nawiewu (ID 80)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
        if (ot->isValidResponse(response) && supply_temp_sensor != nullptr) {
          supply_temp_sensor->publish_state(ot->getFloat(response));
        }
        step++;
        break;

      case 3: // Zapis mocy (ID 71)
        if (target_ventilation > 0) {
          unsigned int data = ot->temperatureToData(target_ventilation);
          ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, data));
        }
        step = 0; 
        break;
    }
  }

  void set_ventilation_level(float level) {
    target_ventilation = level;
    ESP_LOGI("brink", "Zmieniono suwak na: %.1f%%", level);
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
