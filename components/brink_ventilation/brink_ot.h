#include "esphome.h"
#include "OpenTherm.h"

#pragma once

namespace esphome {
namespace brink_ventilation {

class BrinkOpenTherm;

// Obsługa przerwań dla komunikacji OpenTherm
static BrinkOpenTherm *global_brink_ot = nullptr;
static void IRAM_ATTR handleInterrupt();

// --- Klasa pomocnicza dla suwaka mocy w Home Assistant ---
class BrinkNumber : public number::Number {
 public:
  BrinkOpenTherm *parent_;
  void set_parent(BrinkOpenTherm *parent) { parent_ = parent; }
  void control(float value) override;
};

// --- Główna klasa obsługująca rekuperator Brink ---
class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot = nullptr;
  int pin_in, pin_out;
  int current_step = 0;
  float target_ventilation = 0.0f;
  uint8_t temp_lb = 0; // Bufor na młodszy bajt danych

  // Wskaźniki na sensory (ustawiane przez Python przy starcie)
  sensor::Sensor *t_supply_in_sensor{nullptr};
  sensor::Sensor *t_supply_out_sensor{nullptr};
  sensor::Sensor *t_exhaust_in_sensor{nullptr};
  sensor::Sensor *t_exhaust_out_sensor{nullptr};
  sensor::Sensor *current_flow_sensor{nullptr};
  sensor::Sensor *pressure_in_sensor{nullptr};

  // Metody konfiguracyjne wywoływane przez ESPHome
  void set_pins(int in, int out) { pin_in = in; pin_out = out; }
  
  // Te nazwy muszą być identyczne z tymi w sensor.py
  void set_t_supply_in_sensor(sensor::Sensor *s) { t_supply_in_sensor = s; }
  void set_t_supply_out_sensor(sensor::Sensor *s) { t_supply_out_sensor = s; }
  void set_t_exhaust_in_sensor(sensor::Sensor *s) { t_exhaust_in_sensor = s; }
  void set_t_exhaust_out_sensor(sensor::Sensor *s) { t_exhaust_out_sensor = s; }
  void set_current_flow_sensor(sensor::Sensor *s) { current_flow_sensor = s; }
  void set_pressure_in_sensor(sensor::Sensor *s) { pressure_in_sensor = s; }
  
  void set_ventilation_number(BrinkNumber *n) { n->set_parent(this); }

  void setup() override {
    global_brink_ot = this;
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
    ESP_LOGD("brink_ot", "Uruchomiono Brink OpenTherm (In:%d, Out:%d)", pin_in, pin_out);
  }

  void update() override {
    unsigned long response = 0;
    
    // Status (ID 0) - podstawa komunikacji
    ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));
    delay(20);

    switch(current_step) {
      case 0: // Zapis nastawy (ID 71)
        ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
        current_step++; 
        break;

      case 1: // T1 - Czerpnia (ID 80)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
        if (response && t_supply_in_sensor) t_supply_in_sensor->publish_state(ot->getFloat(response));
        current_step++; 
        break;

      case 2: // T2 - Nawiew (ID 81)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)81, 0));
        if (response && t_supply_out_sensor) t_supply_out_sensor->publish_state(ot->getFloat(response));
        current_step++; 
        break;

      case 3: // T3 - Wywiew (ID 82)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
        if (response && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state(ot->getFloat(response));
        current_step++; 
        break;

      case 4: // T4 - Wyrzutnia (ID 83)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)83, 0));
        if (response && t_exhaust_out_sensor) t_exhaust_out_sensor->publish_state(ot->getFloat(response));
        current_step++; 
        break;

      case 5: // Przepływ m3/h (TSP 52 - Bajt Młodszy)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8));
        if (response) temp_lb = (uint8_t)(response & 0xFF);
        current_step++; 
        break;

      case 6: // Przepływ m3/h (TSP 53 - Bajt Starszy + Publikacja)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 53 << 8));
        if (response && current_flow_sensor) {
          uint16_t flow = ((uint16_t)(response & 0xFF) << 8) | temp_lb;
          current_flow_sensor->publish_state(flow);
        }
        current_step++; 
        break;

      case 7: // Ciśnienie Pa (TSP 64 - Bajt Młodszy)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 64 << 8));
        if (response) temp_lb = (uint8_t)(response & 0xFF);
        current_step++; 
        break;

      case 8: // Ciśnienie Pa (TSP 65 - Bajt Starszy + Publikacja)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 65 << 8));
        if (response && pressure_in_sensor) {
          uint16_t pressure = ((uint16_t)(response & 0xFF) << 8) | temp_lb;
          pressure_in_sensor->publish_state(pressure);
        }
        current_step = 0; // Powrót na początek
        break;
    }
  }
};

// Funkcja sterująca wywoływana z Home Assistant
inline void BrinkNumber::control(float value) {
  this->publish_state(value);
  parent_->target_ventilation = value;
}

// Przekierowanie przerwania do biblioteki OpenTherm
static void IRAM_ATTR handleInterrupt() {
  if (global_brink_ot && global_brink_ot->ot) {
    global_brink_ot->ot->handleInterrupt();
  }
}

} // namespace brink_ventilation
} // namespace esphome
