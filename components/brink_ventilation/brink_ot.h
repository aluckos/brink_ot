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

  sensor::Sensor *current_vent_sensor{nullptr};
  sensor::Sensor *supply_temp_sensor{nullptr};
  sensor::Sensor *exhaust_temp_sensor{nullptr};

  BrinkOpenTherm(int in, int out) : PollingComponent(1000), pin_in(in), pin_out(out) {
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
    
    // Utrzymanie sesji
    ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));
    delay(20); 

    switch(current_step) {
      case 0: // Nastawa mocy
        request = ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation);
        ot->sendRequest(request);
        current_step++;
        break;

      case 1: // Temp Nawiewu (ID 80)
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0);
        response = ot->sendRequest(request);
        if (response != 0 && supply_temp_sensor != nullptr) {
           supply_temp_sensor->publish_state(ot->getFloat(response));
        }
        current_step++;
        break;

      case 2: // Temp Wywiewu (ID 82 - Zmienione z 81 na 82 wg Twojego pliku .h)
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0);
        response = ot->sendRequest(request);
        if (response != 0 && exhaust_temp_sensor != nullptr) {
           exhaust_temp_sensor->publish_state(ot->getFloat(response));
        }
        current_step++;
        break;

      case 3: // Przepływ m3/h (ID 89, Index 52)
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8);
        response = ot->sendRequest(request);
        if (response != 0 && current_vent_sensor != nullptr) {
           // Odczytujemy 2 bajty danych (HB i LB)
           // W logu widzieliśmy 40593490 -> 34 to index, 90 to wartość.
           // Jeśli 144 (0x90) to za mało, sprawdzamy czy HB (index) nie niesie danych.
           // Na razie publikujemy czyste LB, ale jako unsigned int.
           uint16_t raw_val = ot->getUInt(response) & 0xFF; 
           
           // Jeśli to Brink 400 i 144 to wartość max, to może to być skala 0-255?
           // Sprawdźmy co się stanie jak pomnożymy przez 2.5 (144 * 2.77 = 400)
           current_vent_sensor->publish_state(raw_val); 
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
