/*
  SensorInterface.h - Interface Class
*/

#ifndef SensorInterface_h
#define SensorInterface_h

#include <stdint.h>

#define TEMPERATURE_INT 1
#define HUMIDITY_INT 2
#define AUDIO 3
#define VOLTAGE_BAT 4
#define CURRENT 5
#define WEIGHT 6
#define TEMPERATURE_EXT 7
#define HUMIDITY_EXT 8


class SensorInterface
{
  // user-accessible "public" interface
  public:
    virtual void read(uint8_t *data) = 0;
    virtual uint8_t getType(void) = 0;
    virtual void setType(uint8_t type) = 0;
    virtual bool hasError(void) = 0;
};

#endif