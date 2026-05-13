// Definicje sensorów - zmieniamy na inicjalizację obiektów
  sensor::Sensor *t_supply_in_sensor = new sensor::Sensor();
  sensor::Sensor *t_supply_out_sensor = new sensor::Sensor();
  sensor::Sensor *t_exhaust_in_sensor = new sensor::Sensor();
  sensor::Sensor *t_exhaust_out_sensor = new sensor::Sensor();
  
  sensor::Sensor *current_flow_sensor = new sensor::Sensor();
  binary_sensor::BinarySensor *filter_status_binary = new binary_sensor::BinarySensor();
  text_sensor::TextSensor *status_text_sensor = new text_sensor::TextSensor();
