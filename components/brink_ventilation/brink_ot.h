#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

static void IRAM_ATTR handleInterrupt() { }

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm ot;
  sensor::Sensor *current_vent_sensor{nullptr};
  sensor::Sensor *supply_temp_sensor{nullptr};
  sensor::Sensor *exhaust_temp_sensor{nullptr};
  
  BrinkOpenTherm(int in_pin, int out_pin) 
      : PollingComponent(10000), ot(in_pin, out_pin) {}

  void setup() override {
    ot.begin(handleInterrupt);
    ESP_LOGI("brink", "Inicjalizacja OpenTherm zakończona.");
  }

  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void update() override {
    ESP_LOGD("brink", "Wysyłam zapytanie o status (ID 77)...");
    
    unsigned long request77 = ot.buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)77, 0);
    unsigned long response77 = ot.sendRequest(request77);
    
    if (ot.isValidResponse(response77)) {
        float val = ot.getUInt(response77);
        ESP_LOGI("brink", "Otrzymano odpowiedź ID 77: %.1f", val);
        if (current_vent_sensor != nullptr) current_vent_sensor->publish_state(val);
    } else {
        ESP_LOGE("brink", "Błąd odpowiedzi OpenTherm (ID 77). Sprawdź połączenie!");
    }

    // Odczyt temperatury nawiewu (ID 80)
    unsigned long request80 = ot.buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0);
    unsigned long response80 = ot.sendRequest(request80);
    if (ot.isValidResponse(response80)) {
        float temp = ot.getFloat(response80);
        ESP_LOGI("brink", "Temperatura nawiewu: %.1f C", temp);
        if (supply_temp_sensor != nullptr) supply_temp_sensor->publish_state(temp);
    }
  }

  void set_ventilation_level(float level) {
    ESP_LOGI("brink", "Próba ustawienia mocy na: %.1f%%", level);
    unsigned int data = ot.temperatureToData(level);
    unsigned long request71 = ot.buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, data);
    unsigned long response = ot.sendRequest(request71);
    
    if (ot.isValidResponse(response)) {
        ESP_LOGI("brink", "Sukces! Brink potwierdził zmianę mocy.");
    } else {
        ESP_LOGE("brink", "Błąd! Brink nie odpowiedział na komendę zmiany mocy.");
    }
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
