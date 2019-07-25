/*
  SmartbeeDHT.h - Sensor Derived Class
*/

#ifndef SmartbeeDHT_h
#define SmartbeeDHT_h

#include "SensorInterface.h"
#include "DHT.h"
#include <Arduino.h>

class SmartbeeDHT: public SensorInterface
{
  // user-accessible "public" interface
  public:
    SmartbeeDHT(DHT *obj, uint8_t type);
    void read(uint8_t *data);
    uint8_t getType(void);
    void setType(uint8_t type);
    bool hasError(void);

  // library-accessible "private" interface
  private:
    DHT* _sensorObj = new DHT(0,0);
    uint8_t _type;
    bool _error;
};

#endif