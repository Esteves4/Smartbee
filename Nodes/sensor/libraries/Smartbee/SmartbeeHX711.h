/*
  SmartbeeHX711.h - Sensor Derived Class
*/

#ifndef SmartbeeHX711_h
#define SmartbeeHX711_h

#include "SensorInterface.h"
#include "HX711.h"
#include <Arduino.h>

class SmartbeeHX711: public SensorInterface
{
  // user-accessible "public" interface
  public:
    SmartbeeHX711(HX711 *obj, uint8_t type);
    void read(uint8_t *data);
    uint8_t getType(void);
    void setType(uint8_t type);
    bool hasError(void);

  // library-accessible "private" interface
  private:
    HX711* _sensorObj = new HX711();
    uint8_t _type;
    bool _error;
};

#endif