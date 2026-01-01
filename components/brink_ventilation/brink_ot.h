#include "esphome.h"
#include "OpenTherm.h"

#pragma once

namespace esphome {
namespace brink_ventilation {

class BrinkOpenTherm;

static BrinkOpenTherm *global_brink_ot = nullptr;
static void IRAM_ATTR handleInterrupt();

class BrinkNumber : public number::Number {
 public:
  BrinkOpenTherm *parent_;
  void set_parent(BrinkOpenTherm *parent) { parent_ = parent; }
  void control(float value) override;
};

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot = nullptr;
  int pin_in, pin_out;
  int current_step = 0;
  float target_ventilation = 28.0f; 

  sensor::Sensor *t_supply_in_sensor{nullptr};
  sensor::Sensor *t_supply_out_sensor{nullptr};
  sensor::Sensor *t_exhaust_in_sensor{nullptr};
  sensor::Sensor *t_exhaust_out_sensor{nullptr};
  sensor::Sensor *current_flow_sensor{nullptr};
  sensor::Sensor *pressure_in_sensor{nullptr};
  text_sensor::TextSensor *status_text_sensor{nullptr};

  void set_pins(int in, int out) { pin_in = in; pin_out = out; }
  void set_t_supply_in_sensor(sensor::Sensor *s) { t_supply_in_sensor = s; }
  void set_t_supply_out_sensor(sensor::Sensor *s) { t_supply_out_sensor = s; }
  void set_t_exhaust_in_sensor(sensor::Sensor *s) { t_exhaust_in_sensor = s; }
  void set_t_exhaust_out_sensor(sensor::Sensor *s) { t_exhaust_out_sensor = s; }
  void set_current_flow_sensor(sensor::Sensor *s) { current_flow_sensor = s; }
  void set_pressure_in_sensor(sensor::Sensor *s) { pressure_in_sensor = s; }
  void set_status_text_sensor(text_sensor::TextSensor *s) { status_text_sensor = s; }
  void set_ventilation_number(BrinkNumber *n) { n->set_parent(this); }

  void setup() override {
    global_brink_ot = this;
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
  }

  void update() override {
    // Korzystamy z dedykowanych funkcji Twojej biblioteki
    switch(current_step) {
      case 0: // Nastawa mocy - dedykowana funkcja setVentilation
        ot->setVentilation((unsigned int)target_ventilation);
        current_step++; break;

      case 1: // T1 - Supply Inlet
        t_supply_in_sensor->publish_state(ot->getVentSupplyInTemperature());
        current_step++; break;

      case 2: // T2 - Supply Outlet (Jeśli 0, spróbujemy TSP w następnym kroku)
        t_supply_out_sensor->publish_state(ot->getVentSupplyOutTemperature());
        current_step++; break;

      case 3: // T3 - Exhaust Inlet
        t_exhaust_in_sensor->publish_state(ot->getVentExhaustInTemperature());
        current_step++; break;

      case 4: // T4 - Exhaust Outlet
        t_exhaust_out_sensor->publish_state(ot->getVentExhaustOutTemperature());
        current_step++; break;

      case 5: // PRZEPŁYW - używamy dedykowanej funkcji getBrink2TSP dla CurrentVol (52)
        {
          uint16_t flow = ot->getBrink2TSP(BrinkTSPindex::CurrentVol);
          if (flow > 0 && flow < 1000) current_flow_sensor->publish_state(flow);
        }
        current_step++; break;

      case 6: // CIŚNIENIE - używamy CPID (64)
        {
          uint16_t pressure = ot->getBrink2TSP(BrinkTSPindex::CPID);
          if (pressure < 1000) pressure_in_sensor->publish_state(pressure);
        }
        current_step = 0; break;
    }
  }
};

inline void BrinkNumber::control(float value) { publish_state(value); parent_->target_ventilation = value; }
static void IRAM_ATTR handleInterrupt() { if (global_brink_ot && global_brink_ot->ot) global_brink_ot->ot->handleInterrupt(); }

} // namespace brink_ventilation
} // namespace esphome
