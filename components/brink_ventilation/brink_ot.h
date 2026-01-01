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

  // Skracamy interwał do 2s, ale będziemy wysyłać tylko jedno zapytanie na raz
  BrinkOpenTherm(int in, int out) : PollingComponent(2000), pin_in(in), pin_out(out) {}

  void setup() override {
    pinMode(pin_in, INPUT);
    pinMode(pin_out, OUTPUT);
    ot = new OpenTherm(pin_in, pin_out);
    
    // TEST: Włączamy tryb INVERTED (drugi parametr true)
    // Jeśli po tym logi nadal będą puste, zmień na false
    ot->begin(handleInterrupt, true); 
    
    ESP_LOGI("brink", "Inicjalizacja OpenTherm (Tryb Inverted)");
  }

  void update() override {
    static int step = 0;
    unsigned long response;

    // Maszyna stanów, aby nie wysyłać wszystkiego na raz (nie blokować procesora)
    switch(step) {
      case 0: // Krok 0: Status i wybudzenie (ID 0)
        // 0x0100 = Master Status: CH enabled (wymagane przez Brinka)
        ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));
        step++;
        break;

      case 1: // Krok 1: Odczyt mocy (ID 77)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)77, 0));
        if (ot->isValidResponse(response)) {
          float val = ot->getUInt(response);
          if (current_vent_sensor != nullptr) current_vent_sensor->publish_state(val);
          ESP_LOGD("brink", "Moc Brinka: %.1f%%", val);
        }
        step++;
        break;

      case 2: // Krok 2: Temperatura nawiewu (ID 80)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
        if (ot->isValidResponse(response) && supply_temp_sensor != nullptr) {
          supply_temp_sensor->publish_state(ot->getFloat(response));
        }
        step++;
        break;

      case 3: // Krok 3: Zapis mocy (ID 71)
        if (target_ventilation > 0) {
          unsigned int data = ot->temperatureToData(target_ventilation);
          ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, data));
          ESP_LOGD("brink", "Wysyłam nastawę mocy: %.1f%%", target_ventilation);
        }
        step = 0; // Powrót do początku
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
