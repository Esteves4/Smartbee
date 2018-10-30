#define SerialMon Serial                                                                          // Serial communication with the computer                           

#define DUMP_AT_COMMANDS                                                                          // Comment this if you don't need to debug the commands to the gsm module
#define DEBUG                                                                                     // Comment this if you don't need to debug the arduino commands         

#include <LowPower.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>

//INITIAL CONFIGURATION OF NRF
const int pinCE = 53;                                                                             // This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 48;                                                                            // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out
const int interruptPin = 18;                                                                      // This pin is used to wake wape the arduino when a payload is received

RF24 radio(pinCE, pinCSN);                                                                        // Declare object from nRF24 library (Create your wireless SPI)
RF24Network network(radio);                                                                       // Network uses that radio

#define id_origem 00                                                                              // Address of this node

//STRUCTURE OF OUR PAYLOAD
struct payload_t {
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
  float peso;
  char erro_vec;
  char timestamp[20]; 
};

struct payload_a {
  int colmeia;
  uint16_t audio;
};

#define E_DHT   0
#define E_TEN_C 1
#define E_TEN_R 2
#define E_RTC   3
#define E_PESO  4
#define E_SD    5

//GLOBAL VARIABLES
const char ArraySize = 12;                                                                        // Amount of payloads the central node is going to save to send to the webservice
payload_t ArrayPayloads[ArraySize];                                                               // Array to save the payloads

char bufferErro[4];

char ArrayCount = 0;                                                                              // Used to store the next payload    
bool dataReceived;                                                                                // Used to know whether a payload was received or not
bool audioReceived;

payload_t payload;
payload_a payload_audio; 


void setup() { 
    
  /* nRF24L01 configuration*/
  #ifdef DEBUG
    SerialMon.begin(57600);                                                         
    SerialMon.println(F("Initializing nRF24L01..."));
  #endif
  
  SPI.begin();                                                                                    // Starts SPI protocol
  radio.begin();                                                                                  // Starts nRF24L01
  radio.maskIRQ(1, 1, 0);                                                                         // Create a interruption mask to only generate interruptions when receive payloads
  radio.setPayloadSize(32);                                                                       // Set payload Size
  radio.setPALevel(RF24_PA_LOW);                                                                  // Set Power Amplifier level
  radio.setDataRate(RF24_2MBPS);                                                                // Set transmission rate

  #ifdef DEBUG                                                       
    SerialMon.println(F("Done..."));
    SerialMon.flush();
    SerialMon.end();
  #endif
  network.begin(/*channel*/ 120, /*node address*/ id_origem);                                     // Starts the network

}

void loop() {
  network.update();                                                                               // Check the network regularly

  receiveData();

  if (dataReceived) {
  
    dataReceived = false;
        
  }else if(audioReceived){   
    audioReceived = false;
  }

}


void receiveData() {
  RF24NetworkHeader header;
 
  network.update();

  if(network.available()) {

    RF24NetworkHeader header;                            // If so, take a look at it
    network.peek(header);

    switch (header.type){                              // Dispatch the message to the correct handler.
        case 'A': handle_audio(header); break;
        case 'D': handle_data(header); break;
    }


  } 
}

void handle_audio(RF24NetworkHeader& header){
  
    network.read(header, &payload_audio, sizeof(payload_audio));                                   // Reads the payload received
     
    #ifdef DEBUG
      SerialMon.begin(57600);
    #endif 
    
    audioReceived = true;
    
    #ifdef DEBUG    
      SerialMon.print(F("\nReceived data from sensor: "));
      SerialMon.println(payload.colmeia);
  
      SerialMon.println(F("The audio: "));
      SerialMon.print(payload_audio.audio);

      SerialMon.flush();
      SerialMon.end();
    #endif 
}

void handle_data(RF24NetworkHeader& header){
  
    network.read(header, &payload, sizeof(payload));                                   // Reads the payload received
     
    ArrayPayloads[ArrayCount] = payload;                                               // Saves the payload received
    
    #ifdef DEBUG
      SerialMon.begin(57600);
    #endif 
    
    dataReceived = true;
    
    #ifdef DEBUG    
      SerialMon.print(F("\nReceived data from sensor: "));
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
      SerialMon.print(payload.tensao_r);
      SerialMon.print(F(" "));
      SerialMon.print(payload.peso);
      SerialMon.print(F(" "));
      SerialMon.println(payload.erro_vec - '\0');

      SerialMon.flush();
      SerialMon.end();
 
    #endif 
}
