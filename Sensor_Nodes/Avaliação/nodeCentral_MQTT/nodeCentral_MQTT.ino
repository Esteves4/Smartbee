#define DEBUG
#define SerialMon Serial

#include <LowPower.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>

//INITIAL CONFIGURATION OF NRF
const int pinCE = 53;                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)

const int pinCSN = 48;                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);                                      // Declare object from nRF24 library (Create your wireless SPI)
RF24Network network(radio);                                     // Network uses that radio

#define id_origem 00                                  // Address of this node

//INITIAL CONFIGURATION OF SIM800
/*const char apn[]  = "claro.com.br";
const char user[] = "claro";
const char pass[] = "claro";*/

const char apn[]  = "timbrasil.br";
const char user[] = "tim";
const char pass[] = "tim";

//INITIAL CONFIGURATION OF MQTT
const char* broker = "200.129.43.208";
const char* user_MQTT = "teste@teste";               
const char* pass_MQTT = "123456";    

//STRUCTURE OF OUR PAYLOAD
struct payload_t {
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
  String  timestamp;
};

//GLOBAL VARIABLES
const char ArraySize = 1;
payload_t ArrayPayloads[ArraySize];
char ArrayCount = 0;
payload_t payload;                                              // Used to store the payload from the sensor node

bool dataReceived;                                              // Used to know whether a payload was received or not

void setup() { 

  /* nRF24L01 configuration*/
  #ifdef DEBUG
    SerialMon.begin(57600);                                           // Start Serial communication
    SerialMon.println(F("Initializing nRF24L01..."));
    SerialMon.flush();
    SerialMon.end();
  #endif
  
  SPI.begin();                                                  // Start SPI protocol
  radio.begin();                                                // Start nRF24L01
  radio.maskIRQ(1, 1, 0);                                       // Create a interruption mask to only generate interruptions when receive payloads
  radio.setPayloadSize(32);                                     // Set payload Size
  radio.setPALevel(RF24_PA_LOW);                                // Set Power Amplifier level
  radio.setDataRate(RF24_250KBPS);                              // Set transmission rate

  attachInterrupt(digitalPinToInterrupt(21), receiveData, FALLING );                    // Attach the pin where the interruption is going to happen
  network.begin(/*channel*/ 120, /*node address*/ id_origem);   // Start the network

}

void loop() {
  network.update();                                            // Check the network regularly

  #ifdef DEBUG
      SerialMon.begin(57600);
      SerialMon.println(F("Shutting Arduino down"));
      SerialMon.flush();
      SerialMon.end();
  #endif
  
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);                  // Function to put the arduino in sleep mode
  
  attachInterrupt(digitalPinToInterrupt(21), receiveData, FALLING );    

  #ifdef DEBUG
    SerialMon.begin(57600);
    SerialMon.println(F("Arduino woke up"));
    SerialMon.flush();
    SerialMon.end();
  #endif

  dataReceived = true;
  if (dataReceived) {
    dataReceived = false;
    savePayload();
  }

}

void savePayload(){
  ArrayPayloads[ArrayCount] = payload;
}

void receiveData() {
  RF24NetworkHeader header;
  
  while (!network.available()) {                              // Keeps busy-waiting until the transmission of the payload is completed
    network.update();
  }

  while (network.available()) {                               // Reads the payload received
    network.read(header, &payload, sizeof(payload));
    
    dataReceived = true;
    #ifdef DEBUG
      SerialMon.begin(57600);
      SerialMon.print(F("Received data from sensor: "));
      SerialMon.println(payload.colmeia);
  
      SerialMon.println(F("The data: "));
      SerialMon.print(payload.colmeia);
      SerialMon.print(F(" "));
      SerialMon.print(payload.temperatura);
      SerialMon.print(F(" "));
      SerialMon.print(payload.umidade);
      SerialMon.print(F(" "));
      SerialMon.print(payload.tensao_c);
      SerialMon.print(F(" "));
      SerialMon.println(payload.tensao_r);
      SerialMon.print(F(" "));
      SerialMon.println(payload.timestamp);
      
      SerialMon.flush();
      SerialMon.end();
    #endif 
  }
  
}
