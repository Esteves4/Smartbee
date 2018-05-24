/*
   Smartbee Project
   2018 May 24
   Load Cell HX711 Module Interface with Arduino to measure weight in Kgs and send to a webservice
   http://smartbee.great.ufc.br/
   
  Arduino
  pin
  2 -> HX711 CLK
  3 -> DOUT
  5V -> VCC
  GND -> GND

  Most any pin on the Arduino Uno will be compatible with DOUT/CLK.
  The HX711 board can be powered from 2.7V to 5V so the Arduino 5V power should be fine.
*/

#include <SoftwareSerial.h>
#include <HX711.h>  //You must have this library in your arduino library folder
#include <JeeLib.h>

#define DOUT  3
#define CLK  2

#define TXD 4
#define RXD 5

#define TIME_READING 60000                             // Time between each reading in milliseconds (60000ms is the maximum supported)

//INITIAL CONFIGURATION OF SIM800L
SoftwareSerial SIM800L(TXD, RXD);                                  

//INITIAL CONFIGURATION OF HX711
HX711 scale(DOUT, CLK);

//STRUCTURE OF OUR PAYLOAD
struct payload_t {
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
  float peso;
};

//GLOBAL VARIABLES
payload_t payload;

float CALIBRATION_FACTOR = -21480;             // Change this value for your calibration factor found
String API_KEY = "X1H7B6RD67MHVGIZ";           // Change this value for your api-key from thingspeak
const char* APN = "claro.com.br";              // Change this value for your network APN
const char* USER = "claro";                    // Change this value for your network USER
const char* PASS = "claro";                    // Change this value for your network PASS


ISR(WDT_vect) {
  Sleepy::watchdogEvent();
}

//=============================================================================================
//                         SETUP
//=============================================================================================
void setup() {
  Serial.begin(9600);
  Serial.println("Press T to tare");
  scale.set_scale(CALIBRATION_FACTOR);                          // Calibration Factor obtained from first sketch
  scale.tare();                                                 // Reset the scale to 0

  /* SIM800L (GSM) configuration */
  Serial.println("Initializing modem...");
  SIM800L.begin(9600);                                          // Start SoftwareSerial for communication with the GSM
  delay(3000);                                                  // Delays of three seconds
  sendAT();                                                     // Send AT command to check if the GSM is responding

  #ifdef DEBUG
    SIM800L.println("AT+CMEE=2");                               // Send AT command to activate the verbose format of error messages
    delay(2000);
    gsmAnswer();
  #endif
}

//=============================================================================================
//                         LOOP
//=============================================================================================
void loop() {
  SIM800L.println("ATZ");                                       // Sends AT command to reset the GSM
  delay (9000);
  gsmAnswer();

  // Reset out payload
  payload.colmeia = 2;
  payload.temperatura = 0;
  payload.umidade = 0;
  payload.tensao_c = 0;
  payload.tensao_r = 0;

  // Measure the weight
  payload.peso = scale.get_units(10);

  Serial.print("Weight: ");
  Serial.print(payload.peso, 3);                                // Up to 3 decimal points
  Serial.println(" kg");                                        // Change this to kg and re-adjust the calibration factor if you follow lbs

  configureBearerProfile(APN, USER, PASS);                      // Configure the GSM network
  sendGET_Requisition(API_KEY);                                 // Sends the data to ThingSpeak
  
  int ok;                                                       // Local variable to know if the arduino slept the time we wanted
  for (int i = 0; i < 5; ++i) {
    do {
      ok = Sleepy::loseSomeTime(TIME_READING);                  // Function to put te arduino in sleep mode
    } while (!ok);
  }
  
  if (Serial.available())
  {
    char temp = Serial.read();
    if (temp == 't' || temp == 'T') {
      scale.tare();                                             // Reset the scale to zero
    }
  }
}
//=============================================================================================

void sendAT() {
  SIM800L.println("AT");
  delay (2000);
  gsmAnswer();
}

bool sendGET_Requisition(String apiKey) {
  SIM800L.println("AT+CSQ");
  delay (1000); //Tempo de espera
  gsmAnswer();

  SIM800L.println("AT+HTTPINIT");
  delay (2000);
  gsmAnswer();

  SIM800L.println("AT+HTTPPARA=\"CID\",1");
  delay (2000);
  gsmAnswer();

  SIM800L.println("AT+HTTPPARA=\"URL\",\"api.thingspeak.com/update?api_key="+ apiKey+ "&field1=" + String(payload.colmeia) + "&field2=" + String(payload.temperatura) + "&field3=" + String(payload.umidade) + "&field4=" + String(payload.tensao_c) + "&field5=" + String(payload.tensao_r) + "&field6=" + String(payload.peso) + "\"");
  delay (2000);
  gsmAnswer();

  SIM800L.println("AT+HTTPACTION=0");
  delay (10000);
  gsmAnswer();

  SIM800L.println("AT+HTTPREAD");
  delay (2000);
  gsmAnswer();

  SIM800L.println("AT+HTTPTERM");
  delay (2000);
  gsmAnswer();

  SIM800L.println("AT+SAPBR=0,1");
  delay (2000);
  gsmAnswer();
}

bool configureBearerProfile(const char* APN, const char* USER, const char* PASS) {

  SIM800L.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  delay (1000); //Tempo de espera
  gsmAnswer();

  SIM800L.println("AT+SAPBR=3,1,\"APN\",\"" + String(APN) + "\"");
  delay (3000); //Tempo de espera
  gsmAnswer();

  SIM800L.println("AT+SAPBR=3,1,\"USER\",\"" + String(USER) + "\"");
  delay (3000);
  gsmAnswer();

  SIM800L.println("AT+SAPBR=3,1,\"PWD\",\"" + String(PASS) + "\"");
  delay (3000); //Tempo de espera
  gsmAnswer();

  SIM800L.println("AT+SAPBR=1,1");
  delay (2000);
  gsmAnswer();

  SIM800L.println("AT+SAPBR=2,1");
  delay (2000);
  gsmAnswer();

}

void gsmAnswer() {
  while (SIM800L.available())
  {
    if (SIM800L.available() > 0)                      // If receives data
    {
      Serial.write(SIM800L.read());                   // Prints out the data

    }
  }
}

void cleanBuffer()
{
  delay( 250 );
  while ( SIM800L.available() > 0)
  {
    SIM800L.read();    // Clean the input buffer
    delay(50);
  }
}

bool waitFor(const char* expected_answer1)
{
  uint8_t x = 0;
  bool answer = false;
  char response[100];
  unsigned long previous;

  memset(response, (char)0, 100);    // Initialize the string

  delay( 250 );

  x = 0;
  previous = millis();

  // this loop waits for the answer
  do {
    // if there are data in the UART input buffer, reads it and checks for the asnwer
    if (SIM800L.available() > 0) {
      response[x] = SIM800L.read();
      x++;
      // check if the desired answer 1  is in the response of the module
      if (strstr(response, expected_answer1) != NULL)
      {
        answer = true;
      }
    }
    delay(10);
  } while ((answer == false) && ((millis() - previous) < 10000 )); // Waits for the asnwer with time out

  Serial.write(response);
  return answer;
}


