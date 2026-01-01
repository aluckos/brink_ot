#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

// Statyczna funkcja przerwania wymagana przez bibliotekę
static void IRAM_ATTR handleInterrupt() {
    // Pusta implementacja
}

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm ot;
  
  sensor::Sensor *current_vent_sensor{nullptr};
  sensor::Sensor *supply_temp_sensor{nullptr};
  sensor::Sensor *exhaust_temp_sensor{nullptr};
  
  // Przekazanie pinów do konstruktora ot
  BrinkOpenTherm(int in_pin, int out_pin) 
      : PollingComponent(10000), ot(in_pin, out_pin) {}

  void setup() override {
    ot.begin(handleInterrupt);
  }

  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void update() override {
    // Używamy nazw sugerowanych przez kompilator: OpenThermMessageType::READ_DATA
    
    // ID 77: Relative ventilation
    unsigned long request77 = ot.buildRequest(OpenThermMessageType::READ_DATA, 77, 0);
    unsigned long response77 = ot.sendRequest(request77);
    if (ot.isValidResponse(response77) && current_vent_sensor != nullptr) {
        current_vent_sensor->publish_state(ot.getUInt(response77));
    }

    // ID 80: Supply inlet temp
    unsigned long request80 = ot.buildRequest(OpenThermMessageType::READ_DATA, 80, 0);
    unsigned long response80 = ot.sendRequest(request80);
    if (ot.isValidResponse(response80) && supply_temp_sensor != nullptr) {
        supply_temp_sensor->publish_state(ot.getFloat(response80));
    }

    // ID 82: Exhaust air temp
    unsigned long request82 = ot.buildRequest(OpenThermMessageType::READ_DATA, 82, 0);
    unsigned long response82 = ot.sendRequest(request82);
    if (ot.isValidResponse(response82) && exhaust_temp_sensor != nullptr) {
        exhaust_temp_sensor->publish_state(ot.getFloat(response82));
    }
  }

  void set_ventilation_level(float level) {
    // ID 71: Write ventilation level - używamy OpenThermMessageType::WRITE_DATA
    unsigned int data = ot.temperatureToData(level);
    unsigned long request71 = ot.buildRequest(OpenThermMessageType::WRITE_DATA, 71, data);
    ot.sendRequest(request71);
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
