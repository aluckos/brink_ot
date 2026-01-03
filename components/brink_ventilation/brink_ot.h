#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

// Najpierw deklarujemy klasę główną, żeby BrinkNumber mógł o niej wiedzieć
class BrinkOpenTherm;

// Klasa do sterowania suwakiem (Moc wentylacji)
class BrinkNumber : public number::Number {
 public:
  BrinkOpenTherm *parent_{nullptr};
  void set_parent(BrinkOpenTherm *parent) { parent_ = parent; }
  
  // Metoda sterująca wywoływana z Home Assistant
  void control(float value) override;
};

// Klasa główna komunikacji
class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot{nullptr};
  int pin_in, pin_out;
  int current_step = 0;
  float target_ventilation = 25.0f;
  uint8_t temp_lb = 0;

  // Definicje sensorów
  sensor::Sensor *t_supply_in_sensor{nullptr};   // T1
  sensor::Sensor *t_supply_out_sensor{nullptr};  // T2
  sensor::Sensor *t_exhaust_in_sensor{nullptr};  // T3
  sensor::Sensor *t_exhaust_out_sensor{nullptr}; // T4
  
  sensor::Sensor *current_flow_sensor{nullptr};
  binary_sensor::BinarySensor *filter_status_binary{nullptr};
  text_sensor::TextSensor *status_text_sensor{nullptr};

  void set_pins(int in, int out) { pin_in = in; pin_out = out; }
  
  void set_t_supply_in_sensor(sensor::Sensor *s) { t_supply_in_sensor = s; }
  void set_t_supply_out_sensor(sensor::Sensor *s) { t_supply_out_sensor = s; }
  void set_t_exhaust_in_sensor(sensor::Sensor *s) { t_exhaust_in_sensor = s; }
  void set_t_exhaust_out_sensor(sensor::Sensor *s) { t_exhaust_out_sensor = s; }
  
  void set_current_flow_sensor(sensor::Sensor *s) { current_flow_sensor = s; }
  void set_filter_status_binary(binary_sensor::BinarySensor *s) { filter_status_binary = s; }
  void set_status_text_sensor(text_sensor::TextSensor *s) { status_text_sensor = s; }
  void set_ventilation_number(BrinkNumber *n) { n->set_parent(this); }

  void setup() override;
  void update() override;
};

// --- IMPLEMENTACJA FUNKCJI ---

static BrinkOpenTherm *global_brink_ot = nullptr;

static void IRAM_ATTR handleInterrupt() {
  if (global_brink_ot != nullptr && global_brink_ot->ot != nullptr) {
    global_brink_ot->ot->handleInterrupt();
  }
}

inline void BrinkOpenTherm::setup() {
  global_brink_ot = this;
  ot = new OpenTherm(pin_in, pin_out);
  ot->begin(handleInterrupt);
}

inline void BrinkNumber::control(float value) {
  this->publish_state(value);
  if (this->parent_ != nullptr) {
    this->parent_->target_ventilation = value;
  }
}

inline void BrinkOpenTherm::update() {
  if (ot == nullptr) return;

  // Sprawdzamy, czy biblioteka jest gotowa na kolejny krok (uwzględnia 100ms idle time)
  if (!ot->isReady()) return;

  float response_val = 0;

  // Master Status - podtrzymanie komunikacji
  ot->setBoilerStatus(false, false, false, false, false);

  if (this->status_text_sensor != nullptr) {
    this->status_text_sensor->publish_state("Połączono");
  }

  switch(current_step) {
    case 0: // Nastawa mocy (Używamy dedykowanej funkcji setVentilation)
      ot->setVentilation((unsigned int)target_ventilation);
      current_step++; break;

    case 1: // T1 Czerpnia
      response_val = ot->getVentSupplyInTemperature();
      if (response_val != -1.0f && t_supply_in_sensor) t_supply_in_sensor->publish_state(response_val);
      current_step++; break;

    case 2: // T2 Nawiew do domu
      response_val = ot->getVentSupplyOutTemperature();
      if (response_val != -1.0f && t_supply_out_sensor) t_supply_out_sensor->publish_state(response_val);
      current_step++; break;

    case 3: // T3 Wywiew z domu
      response_val = ot->getVentExhaustInTemperature();
      if (response_val != -1.0f && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state(response_val);
      current_step++; break;

    case 4: // T4 Wyrzutnia na zewnątrz
      response_val = ot->getVentExhaustOutTemperature();
      if (response_val != -1.0f && t_exhaust_out_sensor) t_exhaust_out_sensor->publish_state(response_val);
      current_step++; break;

    case 5: // Przepływ m3/h (TSP 52 - Low Byte)
      // Używamy getBrinkTSP, która wewnętrznie obsługuje ID 89 i READ
      response_val = ot->getBrinkTSP((BrinkTSPindex)52);
      if (response_val != -1.0f) temp_lb = (uint8_t)response_val;
      current_step++; break;

    case 6: // Przepływ m3/h (TSP 53 - High Byte)
      response_val = ot->getBrinkTSP((BrinkTSPindex)53);
      if (response_val != -1.0f && current_flow_sensor) {
        current_flow_sensor->publish_state(((uint16_t)((uint8_t)response_val) << 8) | temp_lb);
      }
      current_step++; break;

    case 7: // Status Filtra (TSP 13)
      response_val = ot->getBrinkTSP((BrinkTSPindex)13);
      if (response_val != -1.0f && filter_status_binary) {
        filter_status_binary->publish_state(((uint8_t)response_val) == 1);
      }
      current_step = 0; break;
      
    default:
      current_step = 0; break;
  }
}

} // namespace brink_ventilation
} // namespace esphome
