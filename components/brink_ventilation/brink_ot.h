#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

// Musimy użyć zmiennych globalnych, aby handler przerwań widział obiekt OT
static int static_in_pin;
static int static_out_pin;

// Pusta funkcja obsługi przerwań
static void IRAM_ATTR handleInterrupt() { }

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm ot;
  int pin_in;
  int pin_out;

  sensor::Sensor *current_vent_sensor{nullptr};
  sensor::Sensor *supply_temp_sensor{nullptr};
  sensor::Sensor *exhaust_temp_sensor{nullptr};
  
  BrinkOpenTherm(int in, int out) : PollingComponent(10000), pin_in(in), pin_out(out) {
      static_in_pin = in;
      static_out_pin = out;
  }

  void setup() override {
    // Ręczne ustawienie pinów - to często rozwiązuje problem na ESP8266
    pinMode(pin_in, INPUT);
    pinMode(pin_out, OUTPUT);
    
    // Inicjalizacja z handlerem (identycznie jak w brink_openhab)
    ot.begin(handleInterrupt);
    
    // Nadpisujemy piny bezpośrednio w obiekcie, jeśli konstruktor nie zadziałał
    ot.in_pin = pin_in;
    ot.out_pin = pin_out;

    ESP_LOGI("brink", "Inicjalizacja pinu IN:%d OUT:%d", pin_in, pin_out);
  }

  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void update() override {
    // Sprawdzamy status komunikacji
    unsigned long response = ot.sendRequest(ot.buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)77, 0));
    
    if (ot.isValidResponse(response)) {
        float val = ot.getUInt(response);
        if (current_vent_sensor != nullptr) current_vent_sensor->publish_state(val);
        ESP_LOGD("brink", "Odczytano moc: %.1f", val);
    } else {
        // Jeśli nie działa, spróbujmy wysłać pusty status (niezbędny do wybudzenia niektórych Brinków)
        ot.sendRequest(ot.buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0));
        ESP_LOGW("brink", "Brak poprawnej odpowiedzi. Status linii: %d", digitalRead(pin_in));
    }

    // Temperatura (ID 80)
    response = ot.sendRequest(ot.buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
    if (ot.isValidResponse(response) && supply_temp_sensor != nullptr) {
        supply_temp_sensor->publish_state(ot.getFloat(response));
    }
  }

  void set_ventilation_level(float level) {
    unsigned int data = ot.temperatureToData(level);
    unsigned long request = ot.buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, data);
    ot.sendRequest(request);
    ESP_LOGI("brink", "Wysłano żądanie mocy: %.1f", level);
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
