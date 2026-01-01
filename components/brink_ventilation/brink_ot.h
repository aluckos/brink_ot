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
  int current_step = 0;

  sensor::Sensor *current_vent_sensor{nullptr};
  sensor::Sensor *supply_temp_sensor{nullptr};
  sensor::Sensor *exhaust_temp_sensor{nullptr};
  
  float target_ventilation = 0.0f;

  // Zmniejszamy interwał do 1s, bo i tak robimy tylko 1 krok na raz
  BrinkOpenTherm(int in, int out) : PollingComponent(1000), pin_in(in), pin_out(out) {}

  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void setup() override {
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
    ESP_LOGI("brink", "Inicjalizacja nieblokująca (ID 71/77/80/82/89)");
  }

  void update() override {
    unsigned long response;

    switch(current_step) {
      case 0: // KROK 0: Podtrzymanie statusu (ID 0)
        ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));
        current_step++;
        break;

      case 1: // KROK 1: Temperatura Nawiewu (ID 80)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
        if (ot->isValidResponse(response) && supply_temp_sensor != nullptr) {
            supply_temp_sensor->publish_state(ot->getFloat(response));
        }
        current_step++;
        break;

      case 2: // KROK 2: Temperatura Wywiewu (ID 82)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
        if (ot->isValidResponse(response) && exhaust_temp_sensor != nullptr) {
            exhaust_temp_sensor->publish_state(ot->getFloat(response));
        }
        current_step++;
        break;

      case 3: // KROK 3: Aktualny przepływ (ID 89, TSP 52)
        // VentTSPEntry = 89, CurrentVol = 52
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8));
        if (ot->isValidResponse(response) && current_vent_sensor != nullptr) {
            current_vent_sensor->publish_state(ot->getUInt(response) & 0xFF);
        }
        current_step++;
        break;

      case 4: // KROK 4: Zapis nastawy (ID 71)
        if (target_ventilation > 0) {
            ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
        }
        current_step = 0; // Powrót do początku
        break;
    }
  }

  void set_ventilation_level(float level) {
    target_ventilation = level;
    // Nie wysyłamy tutaj (blokująco), krok 4 w update() zajmie się wysyłką w swojej kolejce
    ESP_LOGD("brink", "Planowana zmiana mocy na: %.0f%%", level);
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
