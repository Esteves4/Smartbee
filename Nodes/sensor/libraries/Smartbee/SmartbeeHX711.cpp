/*
  SmartbeeHX711.cpp - Sensor Derived Class
*/

#include "SmartbeeHX711.h"

SmartbeeHX711::SmartbeeHX711(HX711 *obj, uint8_t sensortype){
  _sensorObj = obj;
  _type = sensortype;
}

void SmartbeeHX711::read(uint8_t *data){
  float reading;

  _sensorObj->power_up();

  if (_sensorObj->wait_ready_timeout(1000)) {
    reading = _sensorObj->get_units(10);
  } else {
    reading = -1.0;
  }
  
  if (reading < 0) {

    if (reading < -0.100) {
      _error = true;
    }else{
      _error = false;
    }

    reading = 0;

  }else{
    _error = false;

  }

  _sensorObj->power_down();

  memcpy(data, &reading, sizeof(reading)); 
}

uint8_t SmartbeeHX711::getType(void){
  return _type;
}
    
void SmartbeeHX711::setType(uint8_t type){
  _type =  type;
}

bool SmartbeeHX711::hasErro(void){
  return _error;
}