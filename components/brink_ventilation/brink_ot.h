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
  uint8_t temp_lb = 0;

  // Sensory
  sensor::Sensor *t_supply_in_sensor{nullptr};   // T1
  sensor::Sensor *t_supply_out_sensor{nullptr};  // T2 (TSP 41)
  sensor::Sensor *t_exhaust_in_sensor{nullptr};  // T3
  sensor::Sensor *t_exhaust_out_sensor{nullptr}; // T4 (TSP 43)
  sensor::Sensor *current_flow_sensor{nullptr};
  
  text_sensor::TextSensor *status_sensor{nullptr};
  text_sensor::TextSensor *current_gear_sensor{nullptr};

  void set_pins(int in, int out) { pin_in = in; pin_out = out; }
  
  // Funkcje set_ wywoływane przez Python
  void set_t_supply_in_sensor(sensor::Sensor *s) { t_supply_in_sensor = s; }
  void set_t_supply_out_sensor(sensor::Sensor *s) { t_supply_out_sensor = s; } 
  void set_t_exhaust_in_sensor(sensor::Sensor *s) { t_exhaust_in_sensor = s; }
  void set_t_exhaust_out_sensor(sensor::Sensor *s) { t_exhaust_out_sensor = s; } 
  void set_current_flow_sensor(sensor::Sensor *s) { current_flow_sensor = s; }
  
  void set_status_sensor(text_sensor::TextSensor *s) { status_sensor = s; }
  void set_current_gear_sensor(text_sensor::TextSensor *s) { current_gear_sensor = s; }
  
  void set_ventilation_number(BrinkNumber *n) { n->set_parent(this); }

  void setup() override;
  void update() override;
};

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

  unsigned long response = 0;
  ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));

  if (this->status_sensor != nullptr) {
    this->status_sensor->publish_state("Połączono");
  }

  switch(current_step) {
    case 0:
      ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
      current_step++; break;

    case 1: // T1 (Standard OT ID 80)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
      if (ot->isValidResponse(response) && t_supply_in_sensor) t_supply_in_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 2: // T2 (TSP 41)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 41 << 8));
      if (ot->isValidResponse(response) && t_supply_out_sensor) {
          t_supply_out_sensor->publish_state((float)(response & 0xFF));
      }
      current_step++; break;

    case 3: // T3 (Standard OT ID 82)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
      if (ot->isValidResponse(response) && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 4: // T4 (TSP 43)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 43 << 8));
      if (ot->isValidResponse(response) && t_exhaust_out_sensor) {
          t_exhaust_out_sensor->publish_state((float)(response & 0xFF));
      }
      current_step++; break;

    case 5: // Przepływ LB
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8));
      if (ot->isValidResponse(response)) temp_lb = (uint8_t)(response & 0xFF);
      current_step++; break;

    case 6: // Przepływ HB + Gear Logic
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 53 << 8));
      if (ot->isValidResponse(response) && current_flow_sensor) {
        int flow = ((uint16_t)(response & 0xFF) << 8) | temp_lb;
        current_flow_sensor->publish_state(flow);

        if (current_gear_sensor) {
            if (flow >= 350) current_gear_sensor->publish_state("Bieg 3");
            else if (flow >= 200) current_gear_sensor->publish_state("Bieg 2");
            else if (flow >= 80) current_gear_sensor->publish_state("Bieg 1");
            else if (flow > 0) current_gear_sensor->publish_state("Bieg 0");
            else current_gear_sensor->publish_state("Wyłączony");
        }
      }
      current_step = 0; break;

    default: current_step = 0; break;
  }
}

} // namespace brink_ventilation
} // namespace esphome
