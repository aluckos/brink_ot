#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

static const char *const TAG = "brink_ot";

class BrinkOpenTherm;

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
  float target_ventilation = 25.0f;

  sensor::Sensor *t_supply_in_sensor{nullptr};   
  sensor::Sensor *t_supply_out_sensor{nullptr};  
  sensor::Sensor *t_exhaust_in_sensor{nullptr};  
  sensor::Sensor *t_exhaust_out_sensor{nullptr}; 
  sensor::Sensor *current_flow_sensor{nullptr};  
  
  binary_sensor::BinarySensor *filter_status_binary{nullptr}; 
  text_sensor::TextSensor *status_sensor{nullptr};
  text_sensor::TextSensor *current_gear_sensor{nullptr};

  void set_pins(int in, int out) { pin_in = in; pin_out = out; }
  
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

  // Podtrzymanie
  ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));

  switch(current_step) {
    case 0: // Nastawa
      ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
      current_step++; break;

    case 1: // T1 - Czerpnia (Standard OT ID 80) - sprawdzamy standard zamiast TSP
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
      if (ot->isValidResponse(response) && t_supply_in_sensor) t_supply_in_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 2: // T3 - Wywiew (Standard OT ID 82)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
      if (ot->isValidResponse(response) && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 3: // T2 - Nawiew (TSP 41 lub 42 w niektórych HR)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 41 << 8));
      if (ot->isValidResponse(response) && t_supply_out_sensor) t_supply_out_sensor->publish_state((float)(response & 0xFF));
      current_step++; break;

    case 4: // T4 - Wyrzutnia (TSP 43 lub 44)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 43 << 8));
      if (ot->isValidResponse(response) && t_exhaust_out_sensor) t_exhaust_out_sensor->publish_state((float)(response & 0xFF));
      current_step++; break;

    case 5: // Przepływ (TSP 62) - DZIAŁA
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 62 << 8));
      if (ot->isValidResponse(response) && current_flow_sensor) {
          float flow = (float)(response & 0xFF);
          current_flow_sensor->publish_state(flow);
          if (current_gear_sensor) {
            if (flow > 200) current_gear_sensor->publish_state("Bieg 3");
            else if (flow > 120) current_gear_sensor->publish_state("Bieg 2");
            else if (flow > 40) current_gear_sensor->publish_state("Bieg 1");
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
