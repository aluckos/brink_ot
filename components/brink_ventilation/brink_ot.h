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
  uint8_t temp_lb = 0; // Pomocniczy do składania 2-bajtowych TSP

  // Sensory - Temperatury
  sensor::Sensor *supply_in_temp{nullptr};
  sensor::Sensor *supply_out_temp{nullptr};
  sensor::Sensor *exhaust_in_temp{nullptr};
  sensor::Sensor *exhaust_out_temp{nullptr};
  
  // Sensory - Wentylatory i Przepływ
  sensor::Sensor *fan_speed_supply{nullptr};
  sensor::Sensor *fan_speed_exhaust{nullptr};
  sensor::Sensor *current_flow{nullptr};
  sensor::Sensor *pressure_in{nullptr};
  sensor::Sensor *pressure_out{nullptr};

  // Sensory Binarne / Status
  text_sensor::TextSensor *bypass_status_sensor{nullptr};

  BrinkOpenTherm(int in, int out) : PollingComponent(1000), pin_in(in), pin_out(out) {
    global_brink_ot = this;
  }

  void setup() override {
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
  }

  void update() override {
    unsigned long response = 0;
    
    // Zawsze wysyłaj Status (ID 0) na początku cyklu
    ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));
    delay(20);

    switch(current_step) {
      case 0: // Nastawa mocy (ID 71)
        ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
        current_step++;
        break;

      case 1: // Temp Supply In (ID 80)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
        if (response && supply_in_temp) supply_in_temp->publish_state(ot->getFloat(response));
        current_step++;
        break;

      case 2: // Temp Supply Out (ID 81)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)81, 0));
        if (response && supply_out_temp) supply_out_temp->publish_state(ot->getFloat(response));
        current_step++;
        break;

      case 3: // Temp Exhaust In (ID 82)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
        if (response && exhaust_in_temp) exhaust_in_temp->publish_state(ot->getFloat(response));
        current_step++;
        break;

      case 4: // Temp Exhaust Out (ID 83)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)83, 0));
        if (response && exhaust_out_temp) exhaust_out_temp->publish_state(ot->getFloat(response));
        current_step++;
        break;

      case 5: // RPM Exhaust (ID 84)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)84, 0));
        if (response && fan_speed_exhaust) fan_speed_exhaust->publish_state(ot->getUInt(response));
        current_step++;
        break;

      case 6: // RPM Supply (ID 85)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)85, 0));
        if (response && fan_speed_supply) fan_speed_supply->publish_state(ot->getUInt(response));
        current_step++;
        break;

      case 7: // PRZEPŁYW KROK 1 (TSP 52 - LB)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8));
        if (response) temp_lb = (uint8_t)(response & 0xFF);
        current_step++;
        break;

      case 8: // PRZEPŁYW KROK 2 (TSP 53 - HB)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 53 << 8));
        if (response && current_flow) {
          uint16_t val = ((uint16_t)(response & 0xFF) << 8) | temp_lb;
          current_flow->publish_state(val);
        }
        current_step++;
        break;

      case 9: // CIŚNIENIE NAWIEW (TSP 64/65)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 64 << 8));
        if (response) temp_lb = (uint8_t)(response & 0xFF);
        current_step++;
        break;

      case 10: // CIŚNIENIE NAWIEW FINAL
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 65 << 8));
        if (response && pressure_in) {
          pressure_in->publish_state(((uint16_t)(response & 0xFF) << 8) | temp_lb);
        }
        current_step++;
        break;

      case 11: // STATUS BYPASSU (TSP 55)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 55 << 8));
        if (response && bypass_status_sensor) {
          uint8_t stat = (uint8_t)(response & 0xFF);
          if (stat == 0) bypass_status_sensor->publish_state("Zamknięty");
          else if (stat == 1) bypass_status_sensor->publish_state("Auto / Otwarty");
          else bypass_status_sensor->publish_state("Minimalny");
        }
        current_step = 0;
        break;
    }
  }

  void handle_int() { if (ot) ot->handleInterrupt(); }
  void set_ventilation_level(float level) { target_ventilation = level; }
};

static void IRAM_ATTR handleInterrupt() { if (global_brink_ot) global_brink_ot->handle_int(); }

class BrinkVentilationNumber : public number::Number {
 public:
  BrinkOpenTherm *parent_;
  void set_parent(BrinkOpenTherm *parent) { parent_ = parent; }
  void control(float value) override { this->publish_state(value); parent_->set_ventilation_level(value); }
};

} // namespace brink_ventilation
} // namespace esphome
