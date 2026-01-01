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

  // 1000ms - spokojna komunikacja, raz na sekunde jedno zapytanie
  BrinkOpenTherm(int in, int out) : PollingComponent(1000), pin_in(in), pin_out(out) {}

  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void setup() override {
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
    ESP_LOGI("brink", "Startujemy z wymuszonym ID 0 (Status)");
  }

  void update() override {
    unsigned long response = 0;
    unsigned long request = 0;
    
    // ZAWSZE przed każdym krokiem wyślij status, żeby utrzymać komunikację
    // To jest to "żądanie", o którym wspomniałeś.
    // 0x0100 oznacza MasterStatus = 1 (aktywne połączenie)
    ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));
    delay(100); // krótkie czekanie po statusie

    switch(current_step) {
      case 0:
        // Żądanie zapisu mocy (ID 71)
        request = ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation);
        response = ot->sendRequest(request);
        if (response != 0) ESP_LOGD("brink", "Zapis mocy OK");
        current_step++;
        break;

      case 1:
        // Żądanie temperatury nawiewu (ID 80)
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0);
        response = ot->sendRequest(request);
        if (response != 0 && ot->isValidResponse(response)) {
           supply_temp_sensor->publish_state(ot->getFloat(response));
           ESP_LOGD("brink", "Odczyt temp nawiewu: %.2f", ot->getFloat(response));
        } else {
           ESP_LOGW("brink", "Brak odpowiedzi na ID 80 (Temp Nawiewu)");
        }
        current_step++;
        break;

      case 2:
        // Żądanie przepływu (ID 89, TSP 52)
        request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8);
        response = ot->sendRequest(request);
        if (response != 0) {
           current_vent_sensor->publish_state(ot->getUInt(response) & 0xFF);
           ESP_LOGD("brink", "Odczyt przepływu: %d", (int)(ot->getUInt(response) & 0xFF));
        } else {
           ESP_LOGW("brink", "Brak odpowiedzi na ID 89 (Przepływ)");
        }
        current_step = 0;
        break;
    }
  }

  void set_ventilation_level(float level) {
    target_ventilation = level;
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
