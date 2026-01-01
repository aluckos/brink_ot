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
  int pin_in;
  int pin_out;
  int current_step = 0;
  float target_ventilation = 0.0f;
  
  // Zmienna pomocnicza do trzymania młodszego bajtu
  uint8_t flow_lb = 0;

  sensor::Sensor *current_vent_sensor{nullptr};
  sensor::Sensor *supply_temp_sensor{nullptr};
  sensor::Sensor *exhaust_temp_sensor{nullptr};

  BrinkOpenTherm(int in, int out) : PollingComponent(1500), pin_in(in), pin_out(out) {
    global_brink_ot = this;
  }

  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }
  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }

  void setup() override {
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
  }

  void update() override {
    unsigned long response = 0;
    unsigned long request = 0;
    
    ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));
    delay(50); 

    switch(current_step) {
      case 0: // Nastawa
        request = ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation);
        ot->sendRequest(request);
        current_step++;
        break;

      case 1: // Temp Nawiewu
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0);
        response = ot->sendRequest(request);
        if (response != 0 && supply_temp_sensor != nullptr) {
           supply_temp_sensor->publish_state(ot->getFloat(response));
        }
        current_step++;
        break;

      case 2: // Temp Wywiewu
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0);
        response = ot->sendRequest(request);
        if (response != 0 && exhaust_temp_sensor != nullptr) {
           exhaust_temp_sensor->publish_state(ot->getFloat(response));
        }
        current_step++;
        break;

      case 3: // Przepływ KROK A: Pobierz LB (Indeks 52)
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8);
        response = ot->sendRequest(request);
        if (response != 0) {
           flow_lb = (uint8_t)(response & 0xFF);
        }
        current_step++;
        break;

      case 4: // Przepływ KROK B: Pobierz HB (Indeks 53) i wyślij całość
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 53 << 8);
        response = ot->sendRequest(request);
        if (response != 0 && current_vent_sensor != nullptr) {
           uint8_t flow_hb = (uint8_t)(response & 0xFF);
           
           // Składanie: HB * 256 + LB
           float total_flow = (float)((flow_hb << 8) | flow_lb);
           
           current_vent_sensor->publish_state(total_flow);
           ESP_LOGD("brink", "Przepływ: HB=%d, LB=%d, Wynik=%.0f m3/h", flow_hb, flow_lb, total_flow);
        }
        current_step = 0;
        break;
    }
  }

  void handle_int() { if (ot) ot->handleInterrupt(); }
  void set_ventilation_level(float level) { target_ventilation = level; }
};

static void IRAM_ATTR handleInterrupt() {
  if (global_brink_ot != nullptr) global_brink_ot->handle_int();
}

class BrinkVentilationNumber : public number::Number {
 public:
  BrinkOpenTherm *parent_;
  void set_parent(BrinkOpenTherm *parent) { parent_ = parent; }
  void control(float value) override {
    this->publish_state(value);
    parent_->set_ventilation_level(value);
  }
};

} // namespace brink_ventilation
} // namespace esphome
