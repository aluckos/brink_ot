#include "OpenTherm.h"

OpenTherm::OpenTherm(int inPin, int outPin) : inPin(inPin), outPin(outPin), activeDelay(5), inactiveDelay(1000) {
  response = 0;
  responseTimestamp = 0;
  responseStatus = 0;
  request = 0;
  requestTimestamp = 0;
  requestStatus = 0;
  parity = 0;
  bitCounter = 0;
  callback = NULL;
}

void OpenTherm::begin(void(*cb)()) {
  pinMode(inPin, INPUT);
  pinMode(outPin, OUTPUT);
  digitalWrite(outPin, HIGH);
  attachInterrupt(digitalPinToInterrupt(inPin), cb, CHANGE);
}

void OpenTherm::begin(int inPin, int outPin, void(*cb)()) {
  this->inPin = inPin;
  this->outPin = outPin;
  begin(cb);
}

void OpenTherm::setActiveDelay(unsigned long ms) {
  activeDelay = ms;
}

void OpenTherm::setInactiveDelay(unsigned long ms) {
  inactiveDelay = ms;
}

uint32_t OpenTherm::buildRequest(OpenThermMessageType type, OpenThermMessageID id, uint32_t data) {
  uint32_t request = data;
  request |= ((uint32_t)type) << 28;
  request |= ((uint32_t)id) << 16;
  
  // Calculate parity
  uint32_t pt = request >> 31;
  uint8_t p = pt;
  for (int i = 0; i < 31; i++) {
    p ^= (request >> i) & 1;
  }
  request |= (uint32_t)p << 31;
  return request;
}

bool OpenTherm::isValidResponse(uint32_t response) {
  if (response == 0) return false;
  
  uint8_t msgType = (response >> 28) & 7;
  return msgType >= 4;
}

bool OpenTherm::getBoolean(uint32_t response, int bit) {
  return ((response >> bit) & 1) == 1;
}

uint8_t OpenTherm::getUInt8(uint32_t response, int byteNum) {
  return (response >> (byteNum * 8)) & 0xff;
}

uint16_t OpenTherm::getUInt(uint32_t response) {
  return (response >> 0) & 0xffff;
}

int16_t OpenTherm::getInt(uint32_t response) {
  int16_t value = (response >> 0) & 0xffff;
  return value;
}

float OpenTherm::getFloat(uint32_t response) {
  uint16_t u = getUInt(response);
  
  uint8_t sign = (u >> 15) & 1;
  uint8_t exp = (u >> 11) & 0xf;
  uint32_t mant = u & 0x7ff;
  
  float value = (float)mant;
  value /= (1 << 11);
  value += 1.0;
  
  if (exp == 0) {
    value *= (1.0 / (1 << 3));
  } else {
    while (exp > 0) {
      value *= 2.0;
      exp--;
    }
  }
  
  if (sign == 1) {
    value *= -1;
  }
  
  return value;
}

uint32_t OpenTherm::sendRequest(uint32_t request) {
  this->request = request;
  this->requestStatus = 1;
  unsigned long start = millis();
  
  while (this->requestStatus != 0 && millis() - start < OPENTHERM_TIMEOUT) {
    delay(activeDelay);
  }
  
  uint32_t response = this->response;
  this->responseStatus = 0;
  
  return response;
}

unsigned long OpenTherm::sendRequestAsync(uint32_t request) {
  this->request = request;
  this->requestStatus = 1;
  this->requestTimestamp = millis();
  return this->requestTimestamp;
}

void OpenTherm::handleInterrupt() {
  static uint8_t state = 0; // 0=idle, 1=start, 2=data, 3=stop
  
  if (digitalRead(inPin)) { // rising edge
    if (state == 0) {
      // Start bit detected
      state = 1;
      bitCounter = 0;
      response = 0;
      parity = 0;
    } else if (state == 1) {
      // Data bit
      response |= ((uint32_t)1 << (31 - bitCounter));
      parity ^= 1;
      bitCounter++;
      if (bitCounter == 32) {
        state = 2;
      }
    }
  } else { // falling edge
    if (state == 1) {
      // Data bit is 0
      bitCounter++;
      if (bitCounter == 32) {
        state = 2;
      }
    } else if (state == 2) {
      // Stop bit
      state = 0;
      responseStatus = 1;
      responseTimestamp = millis();
      if (callback) {
        callback();
      }
    }
  }
  
  // Simplified interrupt handling for request sending
  if (requestStatus == 1) {
    // Send request bits
    if (bitCounter < 32) {
      uint8_t bit = (request >> (31 - bitCounter)) & 1;
      writeBit(bit);
      bitCounter++;
    } else if (bitCounter == 32) {
      // Send stop bit
      writeBit(1);
      bitCounter++;
    } else {
      // Done sending
      requestStatus = 0;
      bitCounter = 0;
    }
  }
}

void OpenTherm::writeBit(uint8_t high) {
  if (high) {
    digitalWrite(outPin, HIGH);
  } else {
    digitalWrite(outPin, LOW);
  }
}

uint8_t OpenTherm::readBit() {
  return digitalRead(inPin);
}
