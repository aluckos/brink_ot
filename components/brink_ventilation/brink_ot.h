#include "esphome.h"
#include "OpenTherm.h" // Biblioteka Ihora Melnyka

#pragma once

namespace esphome {
namespace brink_ventilation {

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot = nullptr;
  int pin_in, pin_out;
  int current_step = 0;
  float target_ventilation = 25.0f;

  // Definicje sensorów
  sensor::Sensor *t_supply_in_sensor{nullptr};
  sensor::Sensor *t_exhaust_in_sensor{nullptr};
  sensor::Sensor *current_flow_sensor{nullptr};
  binary_sensor::BinarySensor *filter_status_binary{nullptr};

  BrinkOpenTherm(int in, int out) : PollingComponent(2000) { // Aktualizacja co 2 sekundy
    pin_in = in;
    pin_out = out;
  }

  void setup() override {
    ot = new OpenTherm(pin_in, pin_out, false); // false = nie sterujemy kotłem (master mode)
    // Nie potrzebujemy globalnej funkcji i interrupta tutaj, 
    // biblioteka Ihora poradzi sobie z tym wewnątrz lub przez prosty delay w tym przypadku
  }

  void update() override {
    // 1. Podtrzymanie statusu (ID 0)
    ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));

    // 2. Odczyt Filtra (ID 70)
    unsigned long response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)70, 0));
    if (response != 0 && filter_status_binary != nullptr) {
        // Sprawdzamy bit 5 (0x20)
        filter_status_binary->publish_state(response & 0x20);
    }

    // 3. Pętla parametrów (rotacyjnie co update)
    switch(current_step) {
      case 0: // T1 - Czerpnia
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
        if (response != 0 && t_supply_in_sensor) t_supply_in_sensor->publish_state(ot->getFloat(response));
        current_step++; break;

      case 1: // T3 - Wywiew
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
        if (response != 0 && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state(ot->getFloat(response));
        current_step++; break;

      case 2: // Nastawa mocy (ID 71)
        ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
        current_step++; break;

      case 3: // Przepływ (ID 89, TSP 52)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8));
        if (response != 0 && current_flow_sensor) {
          current_flow_sensor->publish_state((float)(response & 0xFF));
        }
        current_step = 0; break;
    }
  }
};

} // namespace brink_ventilation
} // namespace esphome
