#include <DigitalIO.h>
#define TINY_GSM_MODEM_SIM800                                                                     // Defines the model of our gsm module
#define SerialMon Serial                                                                          // Serial communication with the computer
#define SerialAT Serial2                                                                          // Serial communication with the gsm module
#define TINY_GSM_DEBUG SerialMon                                  

#define DUMP_AT_COMMANDS                                                                          // Comment this if you don't need to debug the commands to the gsm module
#define DEBUG                                                                                     // Comment this if you don't need to debug the arduino commands         

#include <TinyGsmClient.h>
#include <PubSubClient.h>

#include <LowPower.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SdFat.h>
#include <SPI.h>

//INITIAL CONFIGURATION OF SD
const uint8_t SOFT_MISO_PIN = 10;
const uint8_t SOFT_MOSI_PIN = 11;
const uint8_t SOFT_SCK_PIN  = 12;
// Chip select may be constant or RAM variable.
const uint8_t SD_CHIP_SELECT_PIN = 13;
const int8_t DISABLE_CHIP_SELECT = -1;
// SdFat software SPI template
SdFatSoftSpi<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> SD;
// Test file.
SdFile file;

//INITIAL CONFIGURATION OF NRF
const int pinCE = 53;                                                                             // This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 48;                                                                            // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out
const int interruptPin = 18;                                                                      // This pin is used to wake wape the arduino when a payload is received

RF24 radio(pinCE, pinCSN);                                                                        // Declare object from nRF24 library (Create your wireless SPI)
RF24Network network(radio);                                                                       // Network uses that radio

#define id_origem 00                                                                              // Address of this node

//INITIAL CONFIGURATION OF SIM800

/* ### APN configurations ###  */

const char apn[]  = "claro.com.br";
const char user[] = "claro";
const char pass[] = "claro";

/*
const char apn[]  = "timbrasil.br";
const char user[] = "tim";
const char pass[] = "tim";
*/

const int DTR_PIN = 7;                                                                               // This pin is used to wake up the gsm module

//INITIAL CONFIGURATION OF MQTT 
const char* broker = "200.129.43.208";                                                            // Address of the mqtt broker
const char* user_MQTT = "teste@teste";                                                            // Username for the tcp connection
const char* pass_MQTT = "123456";                                                                 // Password for the tcp connection

#define TOPIC "sensors/coleta"                                                                    // MQTT topic where we'll be sending the payloads

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif
  TinyGsmClient client(modem);
  PubSubClient mqtt(client);

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
const char ArraySize = 12;                                                                        // Amount of payloads the central node is going to save to send to the webservice
payload_t ArrayPayloads[ArraySize];                                                               // Array to save the payloads

char ArrayCount = 0;                                                                              // Used to store the next payload    
payload_t payload;                                                                                // Used to store the payload from the sensor node
bool dataReceived;                                                                                // Used to know whether a payload was received or not

long previousMillis = 0;
long interval = 10000; //(ms)

void setup() { 
  /* SIM800L configuration */
  #ifdef DEBUG
    SerialMon.begin(57600);
    delay(10);
    
    SerialMon.println(F("Initializing SIM800L and Configuring..."));
    SerialAT.begin(57600);
    delay(3000);
    
    pinMode(DTR_PIN, OUTPUT);
    digitalWrite(DTR_PIN, LOW);

    SerialMon.println(F("Shutting SIM800L down"));
    sleepGSM();
    
    SerialMon.flush();
    SerialMon.end();
  #else

    /* SIM800L configuration*/
    
    SerialAT.begin(57600);                                                                        // Starts serial communication
    delay(3000);                                                                                  // Waits to communication be established
    
    sleepGSM();                                                                                   // Puts gsm to sleeps
  #endif
    
  /* nRF24L01 configuration*/
  #ifdef DEBUG
    SerialMon.begin(57600);                                                         
    SerialMon.println(F("Initializing nRF24L01..."));
    SerialMon.flush();
  #endif
  
  SPI.begin();                                                                                    // Starts SPI protocol
  radio.begin();                                                                                  // Starts nRF24L01
  radio.maskIRQ(1, 1, 0);                                                                         // Create a interruption mask to only generate interruptions when receive payloads
  radio.setPayloadSize(32);                                                                       // Set payload Size
  radio.setPALevel(RF24_PA_LOW);                                                                  // Set Power Amplifier level
  radio.setDataRate(RF24_250KBPS);                                                                // Set transmission rate

  /* SD configuration*/
//  radio.stopListening();
//  enableSD();
  #ifdef DEBUG
    SerialMon.println(F("Initializing SD..."));
    if (!SD.begin(SD_CHIP_SELECT_PIN))
      SerialMon.println("Initialization failed!");
    else
      SerialMon.println("Initialization done.");
    SerialMon.flush();
    SerialMon.end();
  #else
    SD.begin(SD_CHIP_SELECT_PIN);
  #endif
//  disableSD();
//  radio.startListening();

  network.begin(/*channel*/ 120, /*node address*/ id_origem);                                     // Starts the network

}

void loop() {
  network.update();                                                                               // Check the network regularly
  #ifdef DEBUG
    SerialMon.begin(57600);
  #endif
  
  unsigned long currentMillis = millis();

   if(currentMillis - previousMillis > interval) {
      #ifdef DEBUG
        SerialMon.println(F("\nShutting GSM down"));
      #endif

       /* Puts gsm to sleep again */
       sleepGSM();

      #ifdef DEBUG
        SerialMon.println(F("Shutting Arduino down"));
        SerialMon.flush();
        SerialMon.end();
      #endif

      attachInterrupt(digitalPinToInterrupt(interruptPin), interruptFunction, FALLING);           // Attachs the interrupt again after enabling interruptions
      
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);                                            // Function to put the arduino in sleep mode

      detachInterrupt(digitalPinToInterrupt(interruptPin));
      
      /* Wakes gsm up */
      #ifdef DEBUG
        SerialMon.begin(57600);
      #endif
      
      wakeGSM();                                                                                 // Wakes the gsm                                                            
      delay(1000);                                                                               // Waits for the gsm's startup
      
      #ifdef DEBUG
        SerialMon.println(F("GSM woke up"));
        SerialMon.println(F("Arduino woke up"));
        SerialMon.flush();
        SerialMon.end();
      #endif
  }   

  receiveData();                                         
 
  if (dataReceived) {
    #ifdef DEBUG
      SerialMon.begin(57600);
    #endif
    
    dataReceived = false;
    
    if(!modem.testAT()){
        wakeGSM();                                                                                 // Wakes the gsm                                                            
        delay(1000);                                                                               // Waits for the gsm's startup
    }

    /* Gets the timestamp and removes special characters of the string */
    ArrayPayloads[ArrayCount - 1].timestamp = modem.getGSMDateTime(DATE_FULL);
    ArrayPayloads[ArrayCount - 1].timestamp.remove(2,1);
    ArrayPayloads[ArrayCount - 1].timestamp.remove(4,1);
    ArrayPayloads[ArrayCount - 1].timestamp.remove(6,1);
    ArrayPayloads[ArrayCount - 1].timestamp.remove(8,1);
    ArrayPayloads[ArrayCount - 1].timestamp.remove(10,1);
    ArrayPayloads[ArrayCount - 1].timestamp.remove(12);
    
    /* Check if the array is full, if it is, sends all the payloads to the webservice */

    if(ArrayCount == ArraySize){
      #ifdef DEBUG
        SerialMon.println(F("\nConnecting to the server..."));
        connection(); 
        
        int signalQlt = modem.getSignalQuality();
        SerialMon.println("GSM Signal Quality: " + String(signalQlt));
        
        SerialMon.println(F("Publishing payload..."));
        publish();
        delay(1000);

      #else
        connection();
        publish(); 
        delay(1000);
      #endif

      /* Starts to save the payloads again whether the payloads were sent or not */
      ArrayCount = 0;
    }

    previousMillis = millis();
    
  }

  #ifdef DEBUG
      SerialMon.flush();
      SerialMon.end();
  #endif

}

void interruptFunction(){
    
}

void connection(){
  #ifdef DEBUG
    SerialMon.println(F("Inicializando GSM..."));
    modem.restart();  

    SerialMon.println(F("Aguardando rede..."));
    modem.waitForNetwork();
  
    SerialMon.print(F("Conectando a "));
    SerialMon.print(apn);
    SerialMon.println("...");
  
    modem.gprsConnect(apn, user, pass);
    mqtt.setServer(broker, 1883);
    
    SerialMon.println(F("Conectando ao broker..."));
    mqtt.connect("CentralNode", user_MQTT, pass_MQTT);
    
  #else
    modem.restart();

    modem.waitForNetwork();
       
    modem.gprsConnect(apn, user, pass);
    mqtt.setServer(broker, 1883);
    mqtt.connect("CentralNode", user_MQTT, pass_MQTT);
  #endif
}

void receiveData() {
  RF24NetworkHeader header;
  
  network.update();

  if(network.available()) {                               
    network.read(header, &payload, sizeof(payload));                                   // Reads the payload received
     
    ArrayPayloads[ArrayCount] = payload;                                               // Saves the payload received
    ++ArrayCount;                                            
    
    dataReceived = true;
    
    #ifdef DEBUG
      SerialMon.begin(57600);
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
      SerialMon.println(payload.tensao_r);
      SerialMon.print(F(" "));
      SerialMon.println(payload.timestamp);
      
      SerialMon.flush();
      SerialMon.end();
    #endif 
  }
  
}

void publish(){

  /* Sends to the webservice all the payloads saved */

  for(int i = 0; i < ArraySize; ++i){
    String msg = String(ArrayPayloads[i].colmeia) + "," + String(ArrayPayloads[i].temperatura) + "," + String(ArrayPayloads[i].umidade) + "," + String(ArrayPayloads[i].tensao_c) + "," + String(ArrayPayloads[i].tensao_r) + "," + String(ArrayPayloads[i].timestamp);
        
    int length = msg.length();
    char msgBuffer[length];
    msg.toCharArray(msgBuffer,length+1);
    
    mqtt.publish(TOPIC, msgBuffer);
    delay(1000);
  }

}

void sleepGSM() {
  modem.radioOff();
  digitalWrite(DTR_PIN, HIGH);                                                          // Puts DTR pin in HIGH mode so we can enter in sleep mode
  modem.sleepEnable(true);
  
}

void wakeGSM() {  
  digitalWrite(DTR_PIN, LOW);                                                          // Puts DTR pin in LOW mode so we can exit the sleep mode
  modem.sleepEnable(false);
}

//void enableSD() {
//  pinMode(pinCE, OUTPUT);
//  pinMode(pinCSN, OUTPUT);
//  pinMode(pinSD, OUTPUT);
//  digitalWrite(pinCE, LOW);
//  digitalWrite(pinCSN, HIGH);
//  digitalWrite(pinSD, LOW);
//}
//
//void disableSD() {
//  pinMode(pinCE, OUTPUT);
//  pinMode(pinCSN, OUTPUT);
//  pinMode(pinSD, OUTPUT);
//  digitalWrite(pinCE, HIGH);
//  digitalWrite(pinCSN, LOW);
//  digitalWrite(pinSD, HIGH);
//}
