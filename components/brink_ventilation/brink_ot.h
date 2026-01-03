#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

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

  sensor::Sensor *t_supply_in_sensor{nullptr};   // T1
  sensor::Sensor *t_supply_out_sensor{nullptr};  // T2
  sensor::Sensor *t_exhaust_in_sensor{nullptr};  // T3
  sensor::Sensor *t_exhaust_out_sensor{nullptr}; // T4
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
  // Ręczne budowanie ramki zgodnie z logiką Brink (typ 0 = READ)
  unsigned long requestValue(byte id, unsigned int data = 0) {
    unsigned long request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)id, data);
    // Wymuszamy typ wiadomości 0 (READ) zamiast 1 (READ_DATA) jeśli biblioteka go zmienia
    request &= ~(7ul << 28); 
    
    // Przeliczenie parzystości po zmianie typu
    bool p = false;
    unsigned long temp = request & 0x7FFFFFFF;
    while (temp > 0) { if (temp & 1) p = !p; temp >>= 1; }
    if (p) request |= (1ul << 31); else request &= ~(1ul << 31);

    return ot->sendRequest(request);
  }
};

static BrinkOpenTherm *global_brink_ot = nullptr;
static void IRAM_ATTR handleInterrupt() {
  if (global_brink_ot != nullptr && global_brink_ot->ot != nullptr) {
    global_brink_ot->ot->handleInterrupt();
  }
}

inline void BrinkOpenTherm::setup() {
  global_brink_ot = this;
  ot = new OpenTherm(pin_in, pin_out);
  ot->begin(handleInterrupt);
}

inline void BrinkNumber::control(float value) {
  this->publish_state(value);
  if (this->parent_ != nullptr) {
    this->parent_->target_ventilation = value;
  }
}

inline void BrinkOpenTherm::update() {
  if (ot == nullptr) return;

  // Cykl życia OpenTherm: Zawsze zaczynamy od Statusu
  // 0x0100 informuje rekuperator, że Master jest gotowy do pracy
  ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));

  unsigned long response = 0;
  float temp_val = 0;

  switch(current_step) {
    case 0: // Nastawa mocy (ID 71)
      ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
      current_step++; break;

    case 1: // T1 Czerpnia
      response = requestValue(80);
      if (ot->isValidResponse(response) && t_supply_in_sensor) t_supply_in_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 2: // T2 Nawiew
      response = requestValue(81);
      if (ot->isValidResponse(response) && t_supply_out_sensor) {
          temp_val = ot->getFloat(response);
          if (temp_val < 100.0f && temp_val > -30.0f) t_supply_out_sensor->publish_state(temp_val);
      }
      current_step++; break;

    case 3: // T3 Wywiew
      response = requestValue(82);
      if (ot->isValidResponse(response) && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 4: // T4 Wyrzutnia
      response = requestValue(83);
      if (ot->isValidResponse(response) && t_exhaust_out_sensor) {
          temp_val = ot->getFloat(response);
          if (temp_val < 100.0f && temp_val > -30.0f) t_exhaust_out_sensor->publish_state(temp_val);
      }
      current_step++; break;

    case 5: // Przepływ Low Byte (TSP 52)
      response = requestValue(89, 52 << 8);
      if (ot->isValidResponse(response)) temp_lb = (uint8_t)(response & 0xFF);
      current_step++; break;

    case 6: // Przepływ High Byte (TSP 53)
      response = requestValue(89, 53 << 8);
      if (ot->isValidResponse(response) && current_flow_sensor) {
        current_flow_sensor->publish_state(((uint16_t)(response & 0xFF) << 8) | temp_lb);
      }
      current_step++; break;

    case 7: // Status Filtra (TSP 13)
      response = requestValue(89, 13 << 8);
      if (ot->isValidResponse(response) && filter_status_binary) {
        filter_status_binary->publish_state((response & 0xFF) == 1);
      }
      current_step = 0; break;
      
    default:
      current_step = 0;
  }
}

} // namespace brink_ventilation
} // namespace esphome
