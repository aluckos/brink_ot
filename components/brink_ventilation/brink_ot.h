#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

static void IRAM_ATTR handleInterrupt() { }

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot;
  int pin_in;
  int pin_out;

  sensor::Sensor *current_vent_sensor{nullptr};
  sensor::Sensor *supply_temp_sensor{nullptr};
  sensor::Sensor *exhaust_temp_sensor{nullptr};
  
  // Przechowujemy żądaną moc, aby wysyłać ją cyklicznie
  float target_ventilation = 0.0f;

  BrinkOpenTherm(int in, int out) : PollingComponent(5000), pin_in(in), pin_out(out) {}

  void setup() override {
    pinMode(pin_in, INPUT);
    pinMode(pin_out, OUTPUT);
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
    ESP_LOGI("brink", "Start logiki Brink (Interwał 5s)");
  }

  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void update() override {
    // 1. Zawsze wysyłaj Status (ID 0) - Brink tego wymaga do życia
    // Ustawiamy bit 0 na 1 (Central Heating enabled), co dla Brinka oznacza aktywność
    unsigned long status_req = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100);
    ot->sendRequest(status_req);

    delay(100); // Mała przerwa między ramkami, jak w brink_openhab

    // 2. Pobierz aktualną moc (ID 77)
    unsigned long req77 = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)77, 0);
    unsigned long res77 = ot->sendRequest(req77);
    if (ot->isValidResponse(res77)) {
        float val = ot->getUInt(res77);
        if (current_vent_sensor != nullptr) current_vent_sensor->publish_state(val);
    }

    delay(100);

    // 3. Pobierz temperatury (ID 80 i 82)
    unsigned long req80 = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0);
    unsigned long res80 = ot->sendRequest(req80);
    if (ot->isValidResponse(res80) && supply_temp_sensor != nullptr) {
        supply_temp_sensor->publish_state(ot->getFloat(res80));
    }

    delay(100);

    unsigned long req82 = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0);
    unsigned long res82 = ot->sendRequest(req82);
    if (ot->isValidResponse(res82) && exhaust_temp_sensor != nullptr) {
        exhaust_temp_sensor->publish_state(ot->getFloat(res82));
    }

    // 4. Cyklicznie wysyłaj żądaną moc (ID 71), jeśli jest ustawiona
    if (target_ventilation > 0) {
        unsigned int data = ot->temperatureToData(target_ventilation);
        unsigned long req71 = ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, data);
        ot->sendRequest(req71);
    }
  }

  void set_ventilation_level(float level) {
    target_ventilation = level;
    ESP_LOGI("brink", "Ustawiono nową moc docelową: %.1f%%", level);
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
