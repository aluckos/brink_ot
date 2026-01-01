#include "esphome.h"
#include "OpenTherm.h"

#pragma once

namespace esphome {
namespace brink_ventilation {

class BrinkOpenTherm; 
static BrinkOpenTherm *global_brink_ot = nullptr;
static void IRAM_ATTR handleInterrupt();

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot = nullptr;
  int pin_in, pin_out;
  int current_step = 0;
  float target_ventilation = 0.0f;
  uint8_t temp_lb = 0; 

  sensor::Sensor *supply_in_temp = new sensor::Sensor();
  sensor::Sensor *supply_out_temp = new sensor::Sensor();
  sensor::Sensor *exhaust_in_temp = new sensor::Sensor();
  sensor::Sensor *exhaust_out_temp = new sensor::Sensor();
  sensor::Sensor *current_flow = new sensor::Sensor();
  sensor::Sensor *pressure_in = new sensor::Sensor();

  BrinkOpenTherm(int in, int out) : PollingComponent(1500), pin_in(in), pin_out(out) {
    global_brink_ot = this;
  }

  void setup() override {
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
  }

  void update() override {
    unsigned long response = 0;
    // Status ID 0 dla utrzymania połączenia
    ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));
    delay(20);

    switch(current_step) {
      case 0: // Nastawa mocy
        ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
        current_step++;
        break;
      case 1: // T1 (ID 80)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
        if (response) supply_in_temp->publish_state(ot->getFloat(response));
        current_step++;
        break;
      case 2: // T2 (ID 81)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)81, 0));
        if (response) supply_out_temp->publish_state(ot->getFloat(response));
        current_step++;
        break;
      case 3: // T3 (ID 82)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
        if (response) exhaust_in_temp->publish_state(ot->getFloat(response));
        current_step++;
        break;
      case 4: // T4 (ID 83)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)83, 0));
        if (response) exhaust_out_temp->publish_state(ot->getFloat(response));
        current_step++;
        break;
      case 5: // Przepływ LB (TSP 52)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8));
        if (response) temp_lb = (uint8_t)(response & 0xFF);
        current_step++;
        break;
      case 6: // Przepływ HB (TSP 53)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 53 << 8));
        if (response) current_flow->publish_state(((uint16_t)(response & 0xFF) << 8) | temp_lb);
        current_step++;
        break;
      case 7: // Ciśnienie LB (TSP 64)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 64 << 8));
        if (response) temp_lb = (uint8_t)(response & 0xFF);
        current_step++;
        break;
      case 8: // Ciśnienie HB (TSP 65)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 65 << 8));
        if (response) pressure_in->publish_state(((uint16_t)(response & 0xFF) << 8) | temp_lb);
        current_step = 0;
        break;
    }
  }

  void handle_int() { if (ot) ot->handleInterrupt(); }
};

static void IRAM_ATTR handleInterrupt() { if (global_brink_ot) global_brink_ot->handle_int(); }

class BrinkVentilationNumber : public number::Number {
 public:
  BrinkOpenTherm *parent_;
  void set_parent(BrinkOpenTherm *parent) { parent_ = parent; }
  void control(float value) override {
    this->publish_state(value);
    parent_->target_ventilation = value;
  }
};

} // namespace brink_ventilation
} // namespace esphome
