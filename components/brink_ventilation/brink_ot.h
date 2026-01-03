void update() override {
    unsigned long response = 0;
    
    // 1. Podtrzymanie sesji (Standard)
    ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)0, 0x0100));

    // 2. ODCZYT FILTRA - Dokładnie jak w Twoim getDiagnosticIndication()
    // ID 70 (VentStatus), maska 0x20 (bit 5)
    response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)70, 0));
    if (response && filter_status_binary) {
        // response & 0xFF wyciąga dolny bajt (LB)
        // 0x20 to szesnastkowo 32, czyli bit 5
        bool filter_dirty = (response & 0x20); 
        filter_status_binary->publish_state(filter_dirty);
        
        if (filter_dirty) {
            ESP_LOGW("custom", "ALARM FILTRA! Wykryto bit 5 w ID 70: %08lX", response);
        }
    }

    if (this->status_text_sensor != nullptr) {
      this->status_text_sensor->publish_state("Połączono");
    }

    // 3. Pętla pozostałych parametrów
    switch(current_step) {
      case 0: // Nastawa mocy
        ot->sendRequest(ot->buildRequest(OpenThermMessageType::WRITE_DATA, (OpenThermMessageID)71, (unsigned int)target_ventilation));
        current_step++; break;

      case 1: // T1 Czerpnia
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)80, 0));
        if (response && t_supply_in_sensor) t_supply_in_sensor->publish_state(ot->getFloat(response));
        current_step++; break;

      case 2: // T3 Wywiew
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)82, 0));
        if (response && t_exhaust_in_sensor) t_exhaust_in_sensor->publish_state(ot->getFloat(response));
        current_step++; break;

      case 3: // Przepływ (ID 89, TSP 52)
        response = ot->sendRequest(ot->buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)89, 52 << 8));
        if (response && current_flow_sensor) {
          current_flow_sensor->publish_state((float)(response & 0xFF));
        }
        current_step = 0; break;
    }
  }
