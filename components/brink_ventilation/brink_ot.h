#include "esphome.h"
#include <OpenTherm.h> // Zakładając, że biblioteka jest dostępna w frameworku

class BrinkOpenTherm : public PollingComponent, public CustomAPIDevice {
 public:
  // Sensory dla Home Assistant
  Sensor *current_vent_sensor = new Sensor();
  Sensor *supply_temp_sensor = new Sensor();
  Sensor *exhaust_temp_sensor = new Sensor();

  OpenTherm ot;

  BrinkOpenTherm(int in_pin, int out_pin) : PollingComponent(5000) {
    ot.begin(in_pin, out_pin);
  }

  void setup() override {
    // Rejestracja funkcji sterującej, którą wywołamy z YAML
    register_service(&BrinkOpenTherm::set_ventilation, "set_ventilation", {"level"});
  }

  void update() override {
    // Odczyt aktualnej prędkości (ID 77)
    unsigned long response = ot.sendRequest(ot.buildRequest(OpenThermMessageType::Read_Data, 77, 0));
    if (ot.isValidResponse(response)) {
        float value = ot.getUInt(response) / 1.0f; // Brink często zwraca % jako uint
        current_vent_sensor->publish_state(value);
    }

    // Odczyt temp nawiewu (ID 80)
    response = ot.sendRequest(ot.buildRequest(OpenThermMessageType::Read_Data, 80, 0));
    if (ot.isValidResponse(response)) {
        supply_temp_sensor->publish_state(ot.getFloat(response));
    }
    
    // Odczyt temp wywiewu (ID 82)
    response = ot.sendRequest(ot.buildRequest(OpenThermMessageType::Read_Data, 82, 0));
    if (ot.isValidResponse(response)) {
        exhaust_temp_sensor->publish_state(ot.getFloat(response));
    }
  }

  void set_ventilation(float level) {
    // Wysyłamy ID 71 (Set Ventilation Level)
    // Format f8.8: mnożymy przez 256
    uint16_t data = (uint16_t)(level * 256.0f / 100.0f);
    ot.sendRequest(ot.buildRequest(OpenThermMessageType::Write_Data, 71, data));
    ESP_LOGD("brink", "Ustawiono wentylację na: %.1f%%", level);
  }
};
