#include "esphome.h"
#include "OpenTherm.h"

#pragma once

namespace esphome {
namespace brink_ventilation {

class BrinkOpenTherm; 

// Globalny wskaźnik dla przerwania
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

  // PRZYWRÓCONE METODY DLA ESPHOME
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }
  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }

  void setup() override {
    ot = new OpenTherm(pin_in, pin_out);
    // Rejestracja przerwania
    ot->begin(handleInterrupt);
    ESP_LOGI("brink", "Inicjalizacja systemu przerwań i sensorów...");
  }

  void update() override {
    unsigned long response = 0;
    unsigned long request = 0;
    
    // Utrzymanie sesji
    ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));
    delay(50); 

    switch(current_step) {
      case 0: // STEROWANIE
        request = ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation);
        ot->sendRequest(request);
        current_step++;
        break;

      case 1: // TEMP NAWIEWU (ID 80)
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0);
        response = ot->sendRequest(request);
        if (response != 0) {
           float val = ot->getFloat(response);
           if (supply_temp_sensor != nullptr) supply_temp_sensor->publish_state(val);
           ESP_LOGD("brink", "ID 80 RAW: %08lX, Temp: %.1f", response, val);
        } else {
           ESP_LOGW("brink", "ID 80: TIMEOUT");
        }
        current_step++;
        break;

      case 2: // PRZEPŁYW (ID 89, TSP 52)
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8);
        response = ot->sendRequest(request);
        if (response != 0) {
           int vol = ot->getUInt(response) & 0xFF;
           if (current_vent_sensor != nullptr) current_vent_sensor->publish_state(vol);
           ESP_LOGD("brink", "ID 89 RAW: %08lX, Vol: %d", response, vol);
        } else {
           ESP_LOGW("brink", "ID 89: TIMEOUT");
        }
        current_step = 0;
        break;
    }
  }

  void handle_int() {
    if (ot) ot->handleInterrupt();
  }

  void set_ventilation_level(float level) {
    target_ventilation = level;
  }
};

// Obsługa przerwania
static void IRAM_ATTR handleInterrupt() {
  if (global_brink_ot != nullptr) {
    global_brink_ot->handle_int();
  }
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
