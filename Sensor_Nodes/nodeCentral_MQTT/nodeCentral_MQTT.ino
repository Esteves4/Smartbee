#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialAT Serial2
#define TINY_GSM_DEBUG SerialMon
#define DUMP_AT_COMMANDS
#define DEBUG

#define DTR_PIN 7

const int interruptPin = 18;

#define TOPIC "sensors/coleta"

#include <TinyGsmClient.h>
#include <PubSubClient.h>

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
const char ArraySize = 1;
payload_t ArrayPayloads[ArraySize];
char ArrayCount = 0;
payload_t payload;                                              // Used to store the payload from the sensor node

bool dataReceived;                                              // Used to know whether a payload was received or not

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
    SerialAT.begin(57600);
    delay(3000);
    sleepGSM();
  #endif
    
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

  attachInterrupt(digitalPinToInterrupt(interruptPin), receiveData, FALLING);                    // Attach the pin where the interruption is going to happen
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
  
  attachInterrupt(digitalPinToInterrupt(interruptPin), receiveData, FALLING);    

  #ifdef DEBUG
    SerialMon.begin(57600);
    SerialMon.println(F("Arduino woke up"));
    SerialMon.flush();
    SerialMon.end();
  #endif

 
  if (dataReceived) {
    dataReceived = false;

    wakeGSM();

    ArrayPayloads[ArrayCount - 1].timestamp = modem.getGSMDateTime(DATE_FULL);
    ArrayPayloads[ArrayCount - 1].timestamp.remove(2,1);
    ArrayPayloads[ArrayCount - 1].timestamp.remove(4,1);
    ArrayPayloads[ArrayCount - 1].timestamp.remove(6,1);
    ArrayPayloads[ArrayCount - 1].timestamp.remove(8,1);
    ArrayPayloads[ArrayCount - 1].timestamp.remove(10,1);
    ArrayPayloads[ArrayCount - 1].timestamp.remove(12);
    ArrayPayloads[ArrayCount - 1].tensao_r = (analogRead(0)* (5.0 / 1023.0));

    if(ArrayCount == ArraySize){
      #ifdef DEBUG
        SerialMon.begin(57600);
        SerialMon.println(F("Waking GSM"));

        SerialMon.println(F("Connecting to the server..."));
        connection(); 
        
        int signalQlt = modem.getSignalQuality();
        SerialMon.println("GSM Signal Quality: " + String(signalQlt));
        SerialMon.println(F("Publishing payload..."));
        publish();
        delay(1000);
        SerialMon.println(F("Shutting SIM800L down"));
        SerialMon.flush();
        SerialMon.end();
      #else
        connection();
        publish(); 
        delay(1000);
      #endif
      
      ArrayCount = 0;
    }

    sleepGSM();
  }

}

bool connection(){
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
    return mqtt.connect("CentralNode", user_MQTT, pass_MQTT);
    
  #else
    modem.restart();

    modem.waitForNetwork();
       
    modem.gprsConnect(apn, user, pass);
    mqtt.setServer(broker, 1883);
    return mqtt.connect("CentralNode", user_MQTT, pass_MQTT);
  #endif
}

void receiveData() {
  RF24NetworkHeader header;
  
  while (!network.available()) {                              // Keeps busy-waiting until the transmission of the payload is completed
    network.update();
  }
  
  while (network.available()) {                               // Reads the payload received
    network.read(header, &payload, sizeof(payload));
     
    ArrayPayloads[ArrayCount] = payload;
    ++ArrayCount;
    
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

void publish(){
  for(int i = 0; i < ArraySize; ++i){
    String msg = String(ArrayPayloads[i].colmeia) + "," + String(ArrayPayloads[i].temperatura) + "," + String(ArrayPayloads[i].umidade) + "," + String(ArrayPayloads[i].tensao_c) + "," + String(ArrayPayloads[i].tensao_r) + "," + ArrayPayloads[i].timestamp;
    
    int length = msg.length();
    char msgBuffer[length];
    msg.toCharArray(msgBuffer,length+1);
      
    mqtt.publish(TOPIC, msgBuffer);
  }
  
}

void sleepGSM() {
  modem.radioOff();
  digitalWrite(DTR_PIN, HIGH);                                // Puts DTR pin in HIGH mode so we can enter in sleep mode
  modem.sleepEnable(true);
}

void wakeGSM() {  
  digitalWrite(DTR_PIN, LOW);                                // Puts DTR pin in LOW mode so we can exit the sleep mode
  modem.sleepEnable(false);
}
