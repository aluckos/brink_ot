#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {

// Deklaracje wyprzedzające (Forward declarations)
// To mówi kompilatorowi: "Te klasy istnieją, nie martw się o include"
namespace sensor { class Sensor; }
namespace binary_sensor { class BinarySensor; }
namespace number { class Number; }

namespace brink_ventilation {

class BrinkOpenTherm;

class BrinkNumber : public esphome::number::Number {
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
  esphome::sensor::Sensor *t_supply_in_sensor{nullptr};   
  esphome::sensor::Sensor *t_supply_out_sensor{nullptr};  
  esphome::sensor::Sensor *t_exhaust_in_sensor{nullptr};  
  esphome::sensor::Sensor *t_exhaust_out_sensor{nullptr}; 
  esphome::sensor::Sensor *current_flow_sensor{nullptr};

  // Sensory binarne
  esphome::binary_sensor::BinarySensor *filter_status_binary{nullptr};
  esphome::binary_sensor::BinarySensor *connection_status_binary{nullptr};

  void set_pins(int in, int out) { pin_in = in; pin_out = out; }
  
  void set_t_supply_in_sensor(esphome::sensor::Sensor *s) { t_supply_in_sensor = s; }
  void set_t_supply_out_sensor(esphome::sensor::Sensor *s) { t_supply_out_sensor = s; } 
  void set_t_exhaust_in_sensor(esphome::sensor::Sensor *s) { t_exhaust_in_sensor = s; }
  void set_t_exhaust_out_sensor(esphome::sensor::Sensor *s) { t_exhaust_out_sensor = s; } 
  void set_current_flow_sensor(esphome::sensor::Sensor *s) { current_flow_sensor = s; }

  void set_filter_status_binary(esphome::binary_sensor::BinarySensor *s) { filter_status_binary = s; }
  void set_connection_status_binary(esphome::binary_sensor::BinarySensor *s) { connection_status_binary = s; }
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
  response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));

  if (this->connection_status_binary != nullptr) {
    this->connection_status_binary->publish_state(ot->isValidResponse(response));
  }

  switch(current_step) {
    case 0:
      ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
      current_step++; break;
    case 1:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
      if (response && t_supply_in_sensor) t_supply_in_sensor->publish_state(ot->getFloat(response));
      current_step++; break;
    case 2:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)81, 0));
      if (response && t_supply_out_sensor) t_supply_out_sensor->publish_state(ot->getFloat(response));
      current_step++; break;
    case 3:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
      if (response && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state(ot->getFloat(response));
      current_step++; break;
    case 4:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)83, 0));
      if (response && t_exhaust_out_sensor) t_exhaust_out_sensor->publish_state(ot->getFloat(response));
      current_step++; break;
    case 5:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8));
      if (response) temp_lb = (uint8_t)(response & 0xFF);
      current_step++; break;
    case 6:
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 53 << 8));
      if (response && current_flow_sensor) {
        current_flow_sensor->publish_state((float)(((uint16_t)(response & 0xFF) << 8) | temp_lb));
      }
      current_step++; break;
    case 7:
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
