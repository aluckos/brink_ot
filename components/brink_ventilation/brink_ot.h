#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

static const char *const TAG = "brink_ot";

class BrinkOpenTherm;

// Klasa do obsługi suwaka/nastawy
class BrinkNumber : public number::Number {
 public:
  BrinkOpenTherm *parent_{nullptr};
  void set_parent(BrinkOpenTherm *parent) { parent_ = parent; }
  void control(float value) override;
};

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot{nullptr};
  int pin_in, pin_out;
  int current_step = 0;
  float target_ventilation = 25.0f; // To steruje % lub m3/h
  uint8_t temp_lb = 0;

  // Sensory temperatury (TSP zgodne z Twoją listą)
  sensor::Sensor *t_supply_in_sensor{nullptr};   // T1 (TSP 57)
  sensor::Sensor *t_supply_out_sensor{nullptr};  // T2 (TSP 69)
  sensor::Sensor *t_exhaust_in_sensor{nullptr};  // T3 (TSP 58)
  sensor::Sensor *t_exhaust_out_sensor{nullptr}; // T4 (TSP 70)
  sensor::Sensor *current_flow_sensor{nullptr};  // Przepływ (TSP 62)
  
  binary_sensor::BinarySensor *filter_status_binary{nullptr}; // Filtr (TSP 13)
  text_sensor::TextSensor *status_sensor{nullptr};
  text_sensor::TextSensor *current_gear_sensor{nullptr};

  void set_pins(int in, int out) { pin_in = in; pin_out = out; }
  
  // Settery wywoływane przez kod generowany przez ESPHome
  void set_t_supply_in_sensor(sensor::Sensor *s) { t_supply_in_sensor = s; }
  void set_t_supply_out_sensor(sensor::Sensor *s) { t_supply_out_sensor = s; } 
  void set_t_exhaust_in_sensor(sensor::Sensor *s) { t_exhaust_in_sensor = s; }
  void set_t_exhaust_out_sensor(sensor::Sensor *s) { t_exhaust_out_sensor = s; } 
  void set_current_flow_sensor(sensor::Sensor *s) { current_flow_sensor = s; }
  void set_filter_status_binary(binary_sensor::BinarySensor *s) { filter_status_binary = s; }
  void set_status_sensor(text_sensor::TextSensor *s) { status_sensor = s; }
  void set_current_gear_sensor(text_sensor::TextSensor *s) { current_gear_sensor = s; }
  void set_ventilation_number(BrinkNumber *n) { n->set_parent(this); }

  void setup() override;
  void update() override;
};

// Logika dla suwaka/nastawy biegów
inline void BrinkNumber::control(float value) {
  this->publish_state(value);
  if (this->parent_ != nullptr) {
    this->parent_->target_ventilation = value;
  }
}

static BrinkOpenTherm *global_brink_ot_ptr = nullptr;
static void IRAM_ATTR handleInterruptGlobal() {
  if (global_brink_ot_ptr != nullptr && global_brink_ot_ptr->ot != nullptr) {
    global_brink_ot_ptr->ot->handleInterrupt();
  }
}

inline void BrinkOpenTherm::setup() {
  global_brink_ot_ptr = this;
  ot = new OpenTherm(pin_in, pin_out);
  ot->begin(handleInterruptGlobal);
}

inline void BrinkOpenTherm::update() {
  if (ot == nullptr) return;
  unsigned long response = 0;

  // Podtrzymanie sesji
  ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));
  if (this->status_sensor != nullptr) this->status_sensor->publish_state("OK");

  switch(current_step) {
    case 0: // Sterowanie (Zapis do TSP 52 - Current Volume lub ID 71)
      ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
      current_step++; break;

    case 1: // T1 (TSP 57) - Offset -100
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 57 << 8));
      if (ot->isValidResponse(response) && t_supply_in_sensor) t_supply_in_sensor->publish_state((float)(response & 0xFF) - 100);
      current_step++; break;

    case 2: // T2 (TSP 69) - Offset -100
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 69 << 8));
      if (ot->isValidResponse(response) && t_supply_out_sensor) t_supply_out_sensor->publish_state((float)(response & 0xFF) - 100);
      current_step++; break;

    case 3: // T3 (TSP 58) - Offset -100
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 58 << 8));
      if (ot->isValidResponse(response) && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state((float)(response & 0xFF) - 100);
      current_step++; break;

    case 4: // T4 (TSP 70) - Offset -100
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 70 << 8));
      if (ot->isValidResponse(response) && t_exhaust_out_sensor) t_exhaust_out_sensor->publish_state((float)(response & 0xFF) - 100);
      current_step++; break;

    case 5: // Odczyt aktualnego przepływu (TSP 62)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 62 << 8));
      if (ot->isValidResponse(response) && current_flow_sensor) {
          float flow = (float)(response & 0xFF);
          current_flow_sensor->publish_state(flow);
          
          // Raportowanie biegu na podstawie przepływu (zgodnie z Twoją listą)
          if (current_gear_sensor) {
            if (flow > 250) current_gear_sensor->publish_state("Bieg 3");
            else if (flow > 150) current_gear_sensor->publish_state("Bieg 2");
            else if (flow > 50) current_gear_sensor->publish_state("Bieg 1");
            else current_gear_sensor->publish_state("Bieg 0");
          }
      }
      current_step++; break;

    case 6: // Filtr (TSP 13)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 13 << 8));
      if (ot->isValidResponse(response) && filter_status_binary) filter_status_binary->publish_state((response & 0xFF) > 0);
      current_step = 0; break;
  }
}

} // namespace brink_ventilation
} // namespace esphome
