#pragma once

#include "esphome.h"
#include "OpenTherm.h"

namespace esphome {
namespace brink_ventilation {

static void IRAM_ATTR handleInterrupt() { }

class BrinkOpenTherm : public PollingComponent {
 public:
  OpenTherm *ot = nullptr;
  int pin_in;
  int pin_out;

  sensor::Sensor *current_vent_sensor{nullptr};
  sensor::Sensor *supply_temp_sensor{nullptr};
  sensor::Sensor *exhaust_temp_sensor{nullptr};
  
  float target_ventilation = 0.0f;

  BrinkOpenTherm(int in, int out) : PollingComponent(5000), pin_in(in), pin_out(out) {}

  void set_current_vent_sensor(sensor::Sensor *s) { current_vent_sensor = s; }
  void set_supply_temp_sensor(sensor::Sensor *s) { supply_temp_sensor = s; }
  void set_exhaust_temp_sensor(sensor::Sensor *s) { exhaust_temp_sensor = s; }

  void setup() override {
    ot = new OpenTherm(pin_in, pin_out);
    ot->begin(handleInterrupt);
    ESP_LOGI("brink", "Inicjalizacja Brinka (Emulacja ID 71/77/80/82/89)");
  }

  // Funkcja emulująca ot.getBrinkTSP(CurrentVol)
  // W Twoim pliku .h: VentTSPEntry = 89, CurrentVol = 52
  uint16_t readBrinkVolume() {
    unsigned long request = ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8);
    unsigned long response = ot->sendRequest(request);
    if (ot->isValidResponse(response)) {
        return ot->getUInt(response) & 0xFF; // Pobieramy dolny bajt danych
    }
    return 0;
  }

  void update() override {
    // 1. Utrzymanie połączenia (Master Status)
    ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));
    delay(50);

    // 2. Odczyt Temp. Nawiewu (ID 80)
    unsigned long res80 = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
    if (ot->isValidResponse(res80) && supply_temp_sensor != nullptr) {
        supply_temp_sensor->publish_state(ot->getFloat(res80));
    }
    delay(50);

    // 3. Odczyt Temp. Wywiewu (ID 82)
    unsigned long res82 = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
    if (ot->isValidResponse(res82) && exhaust_temp_sensor != nullptr) {
        exhaust_temp_sensor->publish_state(ot->getFloat(res82));
    }
    delay(50);

    // 4. Odczyt Aktualnego Przepływu (TSP 52 przez ID 89)
    uint16_t volume = readBrinkVolume();
    if (volume > 0 && current_vent_sensor != nullptr) {
        current_vent_sensor->publish_state(volume);
    }
    delay(50);

    // 5. Zapis nastawy wentylacji (ID 71)
    if (target_ventilation > 0) {
        // W Brinku dane dla ID 71 to zazwyczaj dolny bajt (0-100)
        unsigned long req71 = ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation);
        ot->sendRequest(req71);
    }
  }

  void set_ventilation_level(float level) {
    target_ventilation = level;
    // Natychmiastowa reakcja
    unsigned long req71 = ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)level);
    ot->sendRequest(req71);
    ESP_LOGI("brink", "Wysłano nastawę: %.0f%%", level);
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
