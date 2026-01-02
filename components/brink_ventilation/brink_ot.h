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
  sensor::Sensor *t_supply_out_sensor{nullptr};
  sensor::Sensor *t_exhaust_in_sensor{nullptr};
  sensor::Sensor *t_exhaust_out_sensor{nullptr};
  sensor::Sensor *current_flow_sensor{nullptr};
  sensor::Sensor *pressure_in_sensor{nullptr};
  text_sensor::TextSensor *status_text_sensor{nullptr};

  void set_pins(int in, int out) { pin_in = in; pin_out = out; }
  void set_t_supply_in_sensor(sensor::Sensor *s) { t_supply_in_sensor = s; }
  void set_t_supply_out_sensor(sensor::Sensor *s) { t_supply_out_sensor = s; }
  void set_t_exhaust_in_sensor(sensor::Sensor *s) { t_exhaust_in_sensor = s; }
  void set_t_exhaust_out_sensor(sensor::Sensor *s) { t_exhaust_out_sensor = s; }
  void set_current_flow_sensor(sensor::Sensor *s) { current_flow_sensor = s; }
  void set_pressure_in_sensor(sensor::Sensor *s) { pressure_in_sensor = s; }
  void set_status_text_sensor(text_sensor::TextSensor *s) { status_text_sensor = s; }
  void set_ventilation_number(BrinkNumber *n) { n->set_parent(this); }

  void setup() override {
    global_brink_ot = this;
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
  }

  void update() override {
    unsigned long response = 0;
    // Podtrzymanie komunikacji
    ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));
    delay(20);

    switch(current_step) {
      case 0: // Nastawa (ID 71)
        ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
        current_step++; break;

      case 1: // T1 (ID 80)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
        if (response && t_supply_in_sensor) t_supply_in_sensor->publish_state(ot->getFloat(response));
        current_step++; break;

      case 2: // RPM Nawiew (ID 85 z Twojej listy) - mapujemy na T2 dla testu
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)85, 0));
        if (response && t_supply_out_sensor) t_supply_out_sensor->publish_state((float)ot->getUInt(response));
        current_step++; break;

      case 3: // T3 (ID 82)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
        if (response && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state(ot->getFloat(response));
        current_step++; break;

      case 4: // RPM Wywiew (ID 84 z Twojej listy) - mapujemy na T4 dla testu
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)84, 0));
        if (response && t_exhaust_out_sensor) t_exhaust_out_sensor->publish_state((float)ot->getUInt(response));
        current_step++; break;

      case 5: // Odczyt przepływu m3/h (ID 77 z Twojej listy)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)77, 0));
        if (response && current_flow_sensor) current_flow_sensor->publish_state((float)(response & 0xFF));
        current_step = 0; break;
    }
  }
};

inline void BrinkNumber::control(float value) { publish_state(value); parent_->target_ventilation = value; }
static void IRAM_ATTR handleInterrupt() { if (global_brink_ot && global_brink_ot->ot) global_brink_ot->ot->handleInterrupt(); }

} // namespace brink_ventilation
} // namespace esphome
