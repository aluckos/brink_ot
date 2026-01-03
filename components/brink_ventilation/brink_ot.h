#include "esphome.h"
#include "OpenTherm.h"

#pragma once

namespace esphome {
namespace brink_ventilation {

class BrinkOpenTherm;

static BrinkOpenTherm *global_brink_ot = nullptr;
static void IRAM_ATTR handleInterrupt();

class BrinkNumber : public number::Number {
 public:
  BrinkOpenTherm *parent_;
  void set_parent(BrinkOpenTherm *parent) { parent_ = parent; }
  void control(float value) override;
};

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot = nullptr;
  int pin_in, pin_out;
  int current_step = 0;
  float target_ventilation = 25.0f;

  sensor::Sensor *t_supply_in_sensor{nullptr};
  sensor::Sensor *t_exhaust_in_sensor{nullptr};
  sensor::Sensor *current_flow_sensor{nullptr};
  binary_sensor::BinarySensor *filter_status_binary{nullptr};
  text_sensor::TextSensor *status_text_sensor{nullptr};

  void set_pins(int in, int out) { pin_in = in; pin_out = out; }
  void set_t_supply_in_sensor(sensor::Sensor *s) { t_supply_in_sensor = s; }
  void set_t_exhaust_in_sensor(sensor::Sensor *s) { t_exhaust_in_sensor = s; }
  void set_current_flow_sensor(sensor::Sensor *s) { current_flow_sensor = s; }
  void set_filter_status_binary(binary_sensor::BinarySensor *s) { filter_status_binary = s; }
  void set_status_text_sensor(text_sensor::TextSensor *s) { status_text_sensor = s; }
  void set_ventilation_number(BrinkNumber *n) { n->set_parent(this); }

  void setup() override {
    global_brink_ot = this;
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
  }

  void update() override {
    unsigned long response = 0;
    // Podtrzymanie komunikacji (ID 0)
    ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));

    switch(current_step) {
      case 0: // Set Ventilation (ID 71)
        ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
        current_step++; break;

      case 1: // T1 - Czerpnia (Standard ID 80)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
        if (response && t_supply_in_sensor) t_supply_in_sensor->publish_state(ot->getFloat(response));
        current_step++; break;

      case 2: // T3 - Wywiew (Standard ID 82)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
        if (response && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state(ot->getFloat(response));
        current_step++; break;

      case 3: // Przepływ (TSP 52) - Renovent HR czyta to bezpośrednio z TSP
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8));
        if (response && current_flow_sensor) {
          // W Renovent HR wartość przepływu jest często bezpośrednio w bajcie danych (U8)
          current_flow_sensor->publish_state((float)(response & 0xFF));
        }
        current_step++; break;

      case 4: // Status Filtra (TSP 13 - I13 w Twoim kodzie)
        // Zapytanie o TSP index 13
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 13 << 8));
        if (response && filter_status_binary) {
          // Jeśli wartość TSP 13 == 1, filtr jest brudny/wymaga serwisu
          bool filter_dirty = (response & 0xFF) == 1;
          filter_status_binary->publish_state(filter_dirty);
          
          if (filter_dirty) {
             ESP_LOGW("custom", "Brink: Wykryto zabrudzenie filtra (TSP 13)");
          }
        }
        current_step = 0; break;
    }
  }
};

inline void BrinkNumber::control(float value) { publish_state(value); parent_->target_ventilation = value; }
static void IRAM_ATTR handleInterrupt() { if (global_brink_ot && global_brink_ot->ot) global_brink_ot->ot->handleInterrupt(); }

} // namespace brink_ventilation
} // namespace esphome
