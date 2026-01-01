#include "esphome.h"
#include "OpenTherm.h"

#pragma once

namespace esphome {
namespace brink_ventilation {

class BrinkOpenTherm;

// Globalny wskaźnik dla przerwań (Interrupt Handler)
static BrinkOpenTherm *global_brink_ot = nullptr;
static void IRAM_ATTR handleInterrupt();

// --- Klasa pomocnicza dla Suwaka (Number) ---
class BrinkNumber : public number::Number {
 public:
  BrinkOpenTherm *parent_;
  void set_parent(BrinkOpenTherm *parent) { parent_ = parent; }
  void control(float value) override;
};

// --- Główna Klasa Komponentu ---
class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot = nullptr;
  int pin_in, pin_out;
  int current_step = 0;
  float target_ventilation = 0.0f;
  uint8_t temp_lb = 0; // Bufor na młodszy bajt (Low Byte) danych TSP

  // Wskaźniki na sensory (będą ustawione przez Python/to_code)
  sensor::Sensor *t_supply_in_sensor{nullptr};
  sensor::Sensor *t_supply_out_sensor{nullptr};
  sensor::Sensor *t_exhaust_in_sensor{nullptr};
  sensor::Sensor *t_exhaust_out_sensor{nullptr};
  sensor::Sensor *current_flow_sensor{nullptr};
  sensor::Sensor *pressure_in_sensor{nullptr};
  text_sensor::TextSensor *status_text_sensor{nullptr};

  // Metody ustawiające (wywoływane przez ESPHome przy starcie)
  void set_pins(int in, int out) { pin_in = in; pin_out = out; }
  void set_t_supply_in_sensor(sensor::Sensor *s) { t_supply_in_sensor = s; }
  void set_t_supply_out_sensor(sensor::Sensor *s) { t_supply_out_sensor = s; }
  void set_t_exhaust_in_sensor(sensor::Sensor *s) { t_exhaust_in_sensor = s; }
  void set_t_exhaust_out_sensor(sensor::Sensor *s) { t_exhaust_out_sensor = s; }
  void set_current_flow_sensor(sensor::Sensor *s) { current_flow_sensor = s; }
  void set_pressure_in_sensor(sensor::Sensor *s) { pressure_in_sensor = s; }
  void set_status_text_sensor(text_sensor::TextSensor *s) { status_text_sensor = s; }
  
  // Metoda dla suwaka mocy
  void set_ventilation_number(BrinkNumber *n) { n->set_parent(this); }

  void setup() override {
    global_brink_ot = this;
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
    ESP_LOGD("brink", "Inicjalizacja OpenTherm na pinach In:%d Out:%d", pin_in, pin_out);
  }

  void update() override {
    unsigned long response = 0;
    
    // Zawsze wysyłaj Status (ID 0) na początku, aby utrzymać sesję OpenTherm
    ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x
