#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

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

  sensor::Sensor *t_supply_in_sensor{nullptr};   
  sensor::Sensor *t_supply_out_sensor{nullptr};  
  sensor::Sensor *t_exhaust_in_sensor{nullptr};  
  sensor::Sensor *t_exhaust_out_sensor{nullptr}; 
  
  sensor::Sensor *current_flow_sensor{nullptr};
  binary_sensor::BinarySensor *filter_status_binary{nullptr};
  binary_sensor::BinarySensor *connection_status_binary{nullptr};

  void set_pins(int in, int out) { pin_in = in; pin_out = out; }
  
  void set_t_supply_in_sensor(sensor::Sensor *s) { t_supply_in_sensor = s; }
  void set_t_supply_out_sensor(sensor::Sensor *s) { t_supply_out_sensor = s; } 
  void set_t_exhaust_in_sensor(sensor::Sensor *s) { t_exhaust_in_sensor = s; }
  void set_t_exhaust_out_sensor(sensor::Sensor *s) { t_exhaust_out_sensor = s; } 
  
  void set_current_flow_sensor(sensor::Sensor *s) { current_flow_sensor = s; }
  void set_filter_status_binary(binary_sensor::BinarySensor *s) { filter_status_binary = s; }
  void set_connection_status_binary(binary_sensor::BinarySensor *s) { connection_status_binary = s; }
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
  
  // Próba odczytu ID 0 (Status)
  response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));

  // Publikacja stanu połączenia na binary_sensor
  if (this->connection_status_binary != nullptr) {
    this->connection_status_binary->publish_state(ot->isValidResponse(response));
  }

  switch(current_step) {
    case 0: // Nastawa mocy (ID 71)
      ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
      current_step++; break;

    case 1: // T1
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
      if (response && t_supply_in_sensor) t_supply_in_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 2: // T2
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)81, 0));
      if (response && t_supply_out_sensor) t_supply_out_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 3: // T3
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
      if (response && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 4: // T4
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)83, 0));
      if (response && t_exhaust_out_sensor) t_exhaust_out_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 5: // Przepływ LB (TSP 52)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8));
      if (response) temp_lb = (uint8_t)(response & 0xFF);
      current_step++; break;

    case 6: // Przepływ HB (TSP 53)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 53 << 8));
      if (response && current_flow_sensor) {
        current_flow_sensor->publish_state((float)(((uint16_t)(response & 0xFF) << 8) | temp_lb));
      }
      current_step++; break;

    case 7: // Status Filtra (TSP 13)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 13 << 8));
      if (response && filter_status_binary) {
        filter_status_binary->publish_state((response & 0xFF) == 1);
      }
      current_step = 0; break;
      
    default:
      current_step = 0; break;
  }
}

} // namespace brink_ventilation
} // namespace esphome
