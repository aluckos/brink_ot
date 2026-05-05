#ifndef OpenTherm_h
#define OpenTherm_h

#include <Arduino.h>
#include <inttypes.h>

const float OPENTHERM_TIMEOUT = 900; // ms

enum OpenThermMessageType {
  READ_DATA = B000,
  WRITE_DATA = B001,
  INVALID_DATA = B010,
  READ_ACK = B100,
  WRITE_ACK = B101,
  DATA_INVALID = B110,
  UNKNOWN_DATA_ID = B111
};

enum OpenThermMessageID {
  STATUS = 0,
  CONTROL_SETPOINT = 1,
  COMMAND_CODE = 2,
  FEED_TEMP = 3,
  RET_TEMP = 4,
  DHW_SETPOINT = 5,
  DHW_FLOW_RATE = 6,
  DHW_FLOW_TEMP = 7,
  DHW_RETURN_TEMP = 8,
  DHW_PUMP_FLOW = 9,
  EXHAUST_TEMP = 10,
  BOILER_HEAT_EXCHANGER_TEMP = 11,
  BOILER_RETURN_TEMP = 12,
  DHW_HEAT_EXCHANGER_TEMP = 13,
  BOILER_FAN_SPEED = 14,
  EXHAUST_FAN_SPEED = 15,
  ROOM_SETPOINT = 16,
  MODULATION_LEVEL = 17,
  CH_WATER_PRESSURE = 18,
  DHW_FLOW_RATE_CH2 = 19,
  ROOM_TEMPERATURE = 20,
  FEEDING_SYSTEM_TEMP = 21,
  RETURN_SYSTEM_TEMP = 22,
  SYSTEM_PUMP_FLOW_RATE = 23,
  POWER_PRODUCTION = 24,
  POWER_PRODUCTION2 = 25,
  EXHAUST_TEMP_CH2 = 26,
  BOILER_HEAT_EXCHANGER_TEMP_CH2 = 27,
  BOILER_RETURN_TEMP_CH2 = 28,
  BOILER_FAN_SPEED_CH2 = 29,
  ROOM_TEMPERATURE_CH2 = 30,
  ROOM_HUMIDITY = 31,
  BOILER_CONTROL_TYPE = 32,
  OPENTHERM_VERSION_MASTER = 124,
  OPENTHERM_VERSION_SLAVE = 125,
  MASTER_VERSION = 126,
  SLAVE_VERSION = 127
};

class OpenTherm {
  public:
    OpenTherm(int inPin, int outPin);
    void begin(void(*cb)());
    void begin(int inPin, int outPin, void(*cb)());
    
    uint32_t sendRequest(uint32_t request);
    unsigned long sendRequestAsync(uint32_t request);
    uint32_t buildRequest(OpenThermMessageType type, OpenThermMessageID id, uint32_t data);
    
    bool isValidResponse(uint32_t response);
    bool getBoolean(uint32_t response, int bit = 0);
    uint8_t getUInt8(uint32_t response, int byteNum);
    uint16_t getUInt(uint32_t response);
    int16_t getInt(uint32_t response);
    float getFloat(uint32_t response);
    
    void handleInterrupt();
    
    void setActiveDelay(unsigned long ms);
    void setInactiveDelay(unsigned long ms);
    
  private:
    int inPin, outPin;
    volatile uint32_t response;
    volatile unsigned long responseTimestamp;
    volatile uint8_t responseStatus;
    volatile uint32_t request;
    volatile unsigned long requestTimestamp;
    volatile uint8_t requestStatus;
    volatile uint8_t parity;
    volatile uint32_t bitCounter;
    
    unsigned long activeDelay;
    unsigned long inactiveDelay;
    
    void writeBit(uint8_t high);
    uint8_t readBit();
    
    void (*callback)();
};

#endif
