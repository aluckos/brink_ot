#include "esphome.h"
#include "OpenTherm.h"

#pragma once

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

  BrinkOpenTherm(int in, int out) : PollingComponent(500), pin_in(in), pin_out(out) {}

  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void setup() override {
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
    ESP_LOGI("brink", "Tryb diagnostyczny RAW startuje...");
  }

  void log_raw_response(unsigned long request, unsigned long response, const char* name) {
    if (response == 0) {
      ESP_LOGW("brink", "Sensor %s: TIMEOUT (brak odpowiedzi)", name);
    } else {
      bool valid = ot->isValidResponse(response);
      ESP_LOGD("brink", "Sensor %s: RAW [%08lX] | Valid: %s | Val: %.2f", 
               name, response, valid ? "TAK" : "NIE", ot->getFloat(response));
    }
  }

  void update() override {
    unsigned long response = 0;
    unsigned long request = 0;
    
    switch(current_step) {
      case 0:
        // ID 71: Zapis wentylacji
        request = ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation);
        response = ot->sendRequest(request);
        // Nie logujemy zapisu, bo on zazwyczaj działa
        current_step++;
        break;

      case 1:
        // ID 80: Temp Nawiewu
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0);
        response = ot->sendRequest(request);
        log_raw_response(request, response, "TempNawiewu");
        if (ot->isValidResponse(response) && supply_temp_sensor != nullptr)
           supply_temp_sensor->publish_state(ot->getFloat(response));
        current_step++;
        break;

      case 2:
        // ID 82: Temp Wywiewu
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0);
        response = ot->sendRequest(request);
        log_raw_response(request, response, "TempWywiewu");
        if (ot->isValidResponse(response) && exhaust_temp_sensor != nullptr)
           exhaust_temp_sensor->publish_state(ot->getFloat(response));
        current_step++;
        break;

      case 3:
        // ID 89, TSP 52: Przepływ
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8);
        response = ot->sendRequest(request);
        log_raw_response(request, response, "Przeplyw");
        if (ot->isValidResponse(response) && current_vent_sensor != nullptr)
           current_vent_sensor->publish_state(ot->getUInt(response) & 0xFF);
        current_step = 0;
        break;
    }
  }

  void set_ventilation_level(float level) {
    target_ventilation = level;
    ESP_LOGI("brink", "Nowa nastawa: %.0f%% (zostanie wyslana w nastepnym cyklu)", level);
  }
};

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
