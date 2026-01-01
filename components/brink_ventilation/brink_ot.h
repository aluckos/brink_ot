#pragma once

#include "esphome.h"
#include <opentherm.h>

namespace esphome {
namespace brink_ventilation {

// Definicja statycznych zmiennych dla obsługi przerwań biblioteki
static int global_in_pin;
static void IRAM_ATTR handleInterrupt() {
    // Pusta funkcja dla biblioteki, aby kompilator nie zgłaszał błędów
}

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm ot;
  int in_pin, out_pin;

  sensor::Sensor *current_vent_sensor{nullptr};
  sensor::Sensor *supply_temp_sensor{nullptr};
  sensor::Sensor *exhaust_temp_sensor{nullptr};
  
  BrinkOpenTherm(int in, int out) : PollingComponent(10000), in_pin(in), out_pin(out) {
    global_in_pin = in;
  }

  void setup() override {
    // Inicjalizacja biblioteki Ihora Melnyka z podaniem pinów i funkcji przerwania
    ot.begin(in_pin, out_pin, handleInterrupt);
    ESP_LOGD("brink", "OpenTherm zainicjalizowany");
  }

  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void update() override {
    // ID 77: Relative ventilation (%)
    unsigned long request77 = ot.buildRequest(OpenThermMessageType::Read_Data, 77, 0);
    unsigned long response77 = ot.sendRequest(request77);
    if (ot.isValidResponse(response77) && current_vent_sensor != nullptr) {
        current_vent_sensor->publish_state(ot.getUInt(response77));
    }

    // ID 80: Supply inlet temp
    unsigned long request80 = ot.buildRequest(OpenThermMessageType::Read_Data, 80, 0);
    unsigned long response80 = ot.sendRequest(request80);
    if (ot.isValidResponse(response80) && supply_temp_sensor != nullptr) {
        supply_temp_sensor->publish_state(ot.getFloat(response80));
    }

    // ID 82: Exhaust air temp
    unsigned long request82 = ot.buildRequest(OpenThermMessageType::Read_Data, 82, 0);
    unsigned long response82 = ot.sendRequest(request82);
    if (ot.isValidResponse(response82) && exhaust_temp_sensor != nullptr) {
        exhaust_temp_sensor->publish_state(ot.getFloat(response82));
    }
  }

  void set_ventilation_level(float level) {
    // W tej bibliotece temperatureToData przelicza float na format f8.8 (używany też dla %)
    unsigned int data = ot.temperatureToData(level);
    unsigned long request71 = ot.buildRequest(OpenThermMessageType::Write_Data, 71, data);
    ot.sendRequest(request71);
    ESP_LOGD("brink", "Wysłano żądanie ustawienia wentylacji: %.1f%%", level);
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
