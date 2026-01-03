#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

static const char *const TAG = "brink_ot";

class BrinkOpenTherm;

class BrinkNumber : public number::Number {
 public:
  BrinkOpenTherm *parent_{nullptr};
  void set_parent(BrinkOpenTherm *parent) { parent_ = parent; }
  void control(float value) override;
};

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot{nullptr};
  int pin_in, pin_out;
  int current_step = 0;
  float target_ventilation = 25.0f;
  uint8_t temp_lb = 0;

  sensor::Sensor *t_supply_in_sensor{nullptr};   
  sensor::Sensor *t_supply_out_sensor{nullptr};  
  sensor::Sensor *t_exhaust_in_sensor{nullptr};  
  sensor::Sensor *t_exhaust_out_sensor{nullptr}; 
  sensor::Sensor *current_flow_sensor{nullptr};
  binary_sensor::BinarySensor *filter_status_binary{nullptr};
  text_sensor::TextSensor *status_text_sensor{nullptr};

  void set_pins(int in, int out) { pin_in = in; pin_out = out; }
  void set_t_supply_in_sensor(sensor::Sensor *s) { t_supply_in_sensor = s; }
  void set_t_supply_out_sensor(sensor::Sensor *s) { t_supply_out_sensor = s; }
  void set_t_exhaust_in_sensor(sensor::Sensor *s) { t_exhaust_in_sensor = s; }
  void set_t_exhaust_out_sensor(sensor::Sensor *s) { t_exhaust_out_sensor = s; }
  void set_current_flow_sensor(sensor::Sensor *s) { current_flow_sensor = s; }
  void set_filter_status_binary(binary_sensor::BinarySensor *s) { filter_status_binary = s; }
  void set_status_text_sensor(text_sensor::TextSensor *s) { status_text_sensor = s; }
  void set_ventilation_number(BrinkNumber *n) { n->set_parent(this); }

  void setup() override;
  void update() override;

 private:
  unsigned long requestValue(byte id, unsigned int data = 0) {
    unsigned long request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)id, data);
    request &= ~(7ul << 28); // Force type 0 (READ)
    
    bool p = false;
    unsigned long temp = request & 0x7FFFFFFF;
    while (temp > 0) { if (temp & 1) p = !p; temp >>= 1; }
    if (p) request |= (1ul << 31); else request &= ~(1ul << 31);

    return ot->sendRequest(request);
  }

  // Funkcja pomocnicza do logowania odpowiedzi w formacie HEX
  void debugResponse(const char *label, unsigned long response) {
    if (response == 0) {
      ESP_LOGD(TAG, "%s: TIMEOUT (Brak odpowiedzi)", label);
    } else {
      bool valid = ot->isValidResponse(response);
