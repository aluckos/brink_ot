#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

static void IRAM_ATTR handleInterrupt() { }

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot; // Używamy wskaźnika, aby zainicjalizować go w setup
  int pin_in;
  int pin_out;

  sensor::Sensor *current_vent_sensor{nullptr};
  sensor::Sensor *supply_temp_sensor{nullptr};
  sensor::Sensor *exhaust_temp_sensor{nullptr};
  
  BrinkOpenTherm(int in, int out) : PollingComponent(10000), pin_in(in), pin_out(out) {}

  void setup() override {
    // Inicjalizacja obiektu OpenTherm z pinami
    ot = new OpenTherm(pin_in, pin_out);
    
    // Ustawienie trybu pinów
    pinMode(pin_in, INPUT);
    pinMode(pin_out, OUTPUT);
    
    // Start komunikacji
    ot->begin(handleInterrupt);

    ESP_LOGI("brink", "Zainicjalizowano OpenTherm na pinach IN:%d, OUT:%d", pin_in, pin_out);
  }

  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void update() override {
    // ID 77: Relative ventilation
    unsigned long request77 = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)77, 0);
    unsigned long response77 = ot->sendRequest(request77);
    
    if (ot->isValidResponse(response77)) {
        float val = ot->getUInt(response77);
        if (current_vent_sensor != nullptr) current_vent_sensor->publish_state(val);
        ESP_LOGD("brink", "Odczytano moc: %.1f%%", val);
    } else {
        // Logowanie stanu pinu wejściowego dla diagnostyki
        ESP_LOGW("brink", "Brak odpowiedzi ID 77. Stan pinu IN (%d): %d", pin_in, digitalRead(pin_in));
        
        // Próba "wybudzenia" - zapytanie o status (ID 0)
        unsigned long request0 = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0);
        ot->sendRequest(request0);
    }

    // ID 80: Temperatura nawiewu
    unsigned long request80 = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0);
    unsigned long response80 = ot->sendRequest(request80);
    if (ot->isValidResponse(response80) && supply_temp_sensor != nullptr) {
        supply_temp_sensor->publish_state(ot->getFloat(response80));
    }
  }

  void set_ventilation_level(float level) {
    unsigned int data = ot->temperatureToData(level);
    unsigned long request71 = ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, data);
    ot->sendRequest(request71);
    ESP_LOGI("brink", "Wysłano żądanie mocy: %.1f%%", level);
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
