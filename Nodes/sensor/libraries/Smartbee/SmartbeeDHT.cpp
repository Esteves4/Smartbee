/*
  SmartbeeDHT..cpp - Sensor Derived Class
*/

#include "SmartbeeDHT.h"

SmartbeeDHT::SmartbeeDHT(DHT *obj, uint8_t sensortype){
  _sensorObj = obj;
  _type = sensortype;
}

void SmartbeeDHT::read(uint8_t *data){
  float reading;

  if(_type == TEMPERATURE_INT || _type == TEMPERATURE_EXT){
    reading = _sensorObj->readTemperature();
  }else if(_type == HUMIDITY_INT || _type == HUMIDITY_EXT){
    reading = _sensorObj->readHumidity();
  }

  if(isnan(reading)){
    _error = true;
    reading = 0;
  }else{
    _error = false;
  }

  memcpy(data, &reading, sizeof(reading)); 
}

uint8_t SmartbeeDHT::getType(void){
  return _type;
}
    
void SmartbeeDHT::setType(uint8_t type){
  _type =  type;
}

bool SmartbeeDHT::hasErro(void){
  return _error;
}