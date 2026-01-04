#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

static const char *const TAG = "brink_ot";

class BrinkOpenTherm : public PollingComponent, public fan::Fan {
 public:
  OpenTherm *ot{nullptr};
  int pin_in, pin_out;
  int current_step = 0;
  
  // Wartości pomocnicze
  float current_speed_pct = 0;
  uint8_t temp_lb = 0;

  // Sensory (T1-T4 + Przepływ)
  sensor::Sensor *t1_s{nullptr}; sensor::Sensor *t2_s{nullptr};
  sensor::Sensor *t3_s{nullptr}; sensor::Sensor *t4_s{nullptr};
  sensor::Sensor *flow_s{nullptr};
  binary_sensor::BinarySensor *filter_s{nullptr};

  void set_pins(int in, int out) { pin_in = in; pin_out = out; }
  
  // Funkcje fan::Fan
  fan::FanTraits get_traits() override {
    auto traits = fan::FanTraits(false, true, false, 3); // 3 biegi + 0
    return traits;
  }

  void control(const fan::FanCall &call) override {
    if (call.get_state().has_value()) this->state = *call.get_state();
    if (call.get_speed().has_value()) {
        int speed = *call.get_speed(); // 0, 1, 2, 3
        // Mapowanie biegów na przepływ lub % (do ustalenia w WRITE_DATA)
        if (speed == 1) current_speed_pct = 25.0f; 
        else if (speed == 2) current_speed_pct = 50.0f;
        else if (speed == 3) current_speed_pct = 100.0f;
        else current_speed_pct = 10.0f;
    }
    this->publish_state();
  }

  void update() override {
    unsigned long response = 0;
    
    switch(current_step) {
      case 0: // Sterowanie (ID 71 - Ventilation Setpoint %)
        ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)current_speed_pct));
        current_step++; break;

      case 1: // T1 (TSP 57) - Atmosfera wejście
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 57 << 8));
        if (ot->isValidResponse(response) && t1_s) t1_s->publish_state((float)(response & 0xFF) - 100.0f);
        current_step++; break;

      case 2: // T2 (TSP 69) - Atmosfera wyjście
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 69 << 8));
        if (ot->isValidResponse(response) && t2_s) t2_s->publish_state((float)(response & 0xFF) - 100.0f);
        current_step++; break;

      case 3: // T3 (TSP 58) - Wnętrze wejście
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 58 << 8));
        if (ot->isValidResponse(response) && t3_s) t3_s->publish_state((float)(response & 0xFF) - 100.0f);
        current_step++; break;

      case 4: // T4 (TSP 70) - Wnętrze wyjście
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 70 << 8));
        if (ot->isValidResponse(response) && t4_s) t4_s->publish_state((float)(response & 0xFF) - 100.0f);
        current_step++; break;

      case 5: // Odczyt aktualnego przepływu (TSP 52)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8));
        if (ot->isValidResponse(response) && flow_s) flow_s->publish_state(response & 0xFF);
        current_step = 0; break;
    }
  }
};

} // namespace brink_ventilation
} // namespace esphome
