
/*
   circuits4you.com
   2016 November 25
   Load Cell HX711 Module Interface with Arduino to measure weight in Kgs
  Arduino
  pin
  2 -> HX711 CLK
  3 -> DOUT
  5V -> VCC
  GND -> GND

  Most any pin on the Arduino Uno will be compatible with DOUT/CLK.
  The HX711 board can be powered from 2.7V to 5V so the Arduino 5V power should be fine.
*/

#include <HX711.h>  //You must have this library in your arduino library folder

#define DOUT  3
#define CLK  2

HX711 scale(DOUT, CLK);


//Change this calibration factor as per your load cell once it is found you many need to vary it in thousands

float knownWeight = 1.0;
float calibration_factor1 = -77440; //-106600 worked for my 40Kg max scale setup
float calibration_factor2 = -2210; //-106600 worked for my 40Kg max scale setup
float offset1, offset2;
//=============================================================================================
//                         SETUP
//=============================================================================================
void setup() {
  Serial.begin(9600);
  Serial.println("Press T to tare");

  scale.set_gain(64);
  scale.set_scale(calibration_factor1);
  offset1 = scale.read_average(5);
  
  scale.set_gain(32);
  scale.set_scale(calibration_factor2);
  offset2 = scale.read_average(5);
}

//=============================================================================================
//                         LOOP
//=============================================================================================
void loop() {
  
  Serial.print("Read 1: ");
  scale.set_gain(64);
  scale.set_scale(calibration_factor1);
  scale.set_offset(offset1);
  Serial.println(scale.get_units(5),3);  //Up to 3 decimal points

  Serial.print("Read 2: ");
  scale.set_gain(32);
  scale.set_scale(calibration_factor2);
  scale.set_offset(offset2);
  Serial.println(scale.get_units(5),3);  //Up to 3 decimal points


  if (Serial.available())
  {
    char temp = Serial.read();
    if (temp == 't' || temp == 'T') {
      scale.tare();  //Reset the scale to zero
    }
    if (temp == 'c' || temp == 'C') {
      calibration();  //Calibra a balança
    }
  }
}
//=============================================================================================

void calibration() {
  float result = 0;
  calibration_factor1 = -77440;
  scale.set_gain(64);

  do {
    scale.set_scale(calibration_factor1); //Adjust to this calibration factor

    Serial.print("Reading: ");
    result = scale.get_units(5);
    Serial.print(result, 3);
    Serial.print(" kg"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
    Serial.print(" calibration_factor: ");
    Serial.print(calibration_factor1);
    Serial.println();

    if (result > (knownWeight + 0.2 * knownWeight)) {
      calibration_factor1 -= 100;
    } else if (result < (knownWeight - 0.2 * knownWeight)) {
      calibration_factor1 += 100;
    } else if (result > (knownWeight + 0.05 * knownWeight)) {
      calibration_factor1 -= 10;
    } else {
      calibration_factor1 += 10;
    }


  } while ((result >= (knownWeight + 0.001 * knownWeight)) || (result <= (knownWeight - 0.001 * knownWeight)));
}

