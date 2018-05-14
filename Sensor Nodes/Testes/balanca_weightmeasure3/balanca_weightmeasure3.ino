
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

float knownWeight = 2.0;
float calibration_factor = -4090; //-106600 worked for my 40Kg max scale setup

//=============================================================================================
//                         SETUP
//=============================================================================================
void setup() {
  Serial.begin(9600);
  Serial.println("Press T to tare");
  scale.set_scale(calibration_factor);  //Calibration Factor obtained from first sketch
  scale.tare();             //Reset the scale to 0

}

//=============================================================================================
//                         LOOP
//=============================================================================================
void loop() {

  Serial.print("Read: ");
  Serial.println(scale.get_units(5), 3); //Up to 3 decimal points

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

  do {
    scale.set_scale(calibration_factor); //Adjust to this calibration factor

    Serial.print("Reading: ");
    result = scale.get_units(5);
    Serial.print(result, 3);
    Serial.print(" kg"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
    Serial.print(" calibration_factor: ");
    Serial.print(calibration_factor);
    Serial.println();

    if (result > (knownWeight + 0.2 * knownWeight)) {
      calibration_factor -= 100;
    } else if (result < (knownWeight - 0.2 * knownWeight)) {
      calibration_factor += 100;
    } else if (result > (knownWeight + 0.05 * knownWeight)) {
      calibration_factor -= 10;
    } else {
      calibration_factor += 10;
    }


  } while ((result >= (knownWeight + 0.005 * knownWeight)) || (result <= (knownWeight - 0.005 * knownWeight)));
}

