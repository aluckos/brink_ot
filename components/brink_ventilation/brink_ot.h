#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

static void IRAM_ATTR handleInterrupt() { }

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

  // Interwał 200ms - każdy krok wykonuje się błyskawicznie
  BrinkOpenTherm(int in, int out) : PollingComponent(200), pin_in(in), pin_out(out) {}

  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void setup() override {
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
  }

  void update() override {
    unsigned long response = 0;
    
    // Krok 0 zawsze wysyła nastawę, aby Brink czuł, że Master żyje
    if (current_step == 0) {
      // ID 71: Write ventilation level
      ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
      current_step++;
    } 
    else if (current_step == 1) {
      // ID 80: Supply Temp
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
      if (ot->isValidResponse(response) && supply_temp_sensor != nullptr) {
        supply_temp_sensor->publish_state(ot->getFloat(response));
      }
      current_step++;
    }
    else if (current_step == 2) {
      // ID 82: Exhaust Temp
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
      if (ot->isValidResponse(response) && exhaust_temp_sensor != nullptr) {
        exhaust_temp_sensor->publish_state(ot->getFloat(response));
      }
      current_step++;
    }
    else if (current_step == 3) {
      // ID 89, TSP 52: Current Flow
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8));
      if (ot->isValidResponse(response) && current_vent_sensor != nullptr) {
        // Spróbujmy wyciągnąć wartość z obu bajtów jeśli 0xFF zawiedzie
        current_vent_sensor->publish_state(ot->getUInt(response));
      }
      current_step = 0;
    }
  }

  void set_ventilation_level(float level) {
    target_ventilation = level;
    // Nie wysyłamy tutaj - czekamy na krok 0 w update()
  }
};
