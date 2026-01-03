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
  // Metoda pomocnicza do logowania wyników w konsoli
  void log_debug(const char *name, unsigned long res) {
    if (res == 0) {
      ESP_LOGD(TAG, "Sensor %s: TIMEOUT (Brak odpowiedzi)", name);
    } else {
      bool valid = ot->isValidResponse(res);
      float val = ot->getFloat(res);
      ESP_LOGD(TAG, "Sensor %s: RAW=%08lX, Valid=%s, Value=%.2f", 
               name, res, valid ? "TAK" : "NIE", val);
    }
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

  unsigned long response = 0;
  // ID 0: Master Status
  ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));

  if (this->status_text_sensor != nullptr) {
    this->status_text_sensor->publish_state("Połączono");
  }

  switch(current_step) {
    case 0: // Nastawa mocy (ID 71)
      ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
      current_step++; break;

    case 1: // T1 Czerpnia (ID 80)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
      if (response && t_supply_in_sensor) t_supply_in_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 2: // T2 Nawiew (ID 81) - DEBUG
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)81, 0));
      log_debug("T2_NAWIEW", response);
      if (response && t_supply_out_sensor) t_supply_out_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 3: // T3 Wywiew (ID 82)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
      if (response && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 4: // T4 Wyrzutnia (ID 83) - DEBUG
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)83, 0));
      log_debug("T4_WYRZUTNIA", response);
      if (response && t_exhaust_out_sensor) t_exhaust_out_sensor->publish_state(ot->getFloat(response));
      current_step++; break;

    case 5: // Przepływ Low Byte (TSP 52)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8));
      if (response) temp_lb = (uint8_t)(response & 0xFF);
      current_step++; break;

    case 6: // Przepływ High Byte (TSP 53)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 53 << 8));
      if (response && current_flow_sensor) {
        current_flow_sensor->publish_state(((uint16_t)(response & 0xFF) << 8) | temp_lb);
      }
      current_step++; break;

    case 7: // Status Filtra (TSP 13)
      response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 13 << 8));
      if (response && filter_status_binary) {
        filter_status_binary->publish_state((response & 0xFF) == 1);
      }
      current_step = 0; break;
      
    default:
      current_step = 0; break;
  }
}

} // namespace brink_ventilation
} // namespace esphome
