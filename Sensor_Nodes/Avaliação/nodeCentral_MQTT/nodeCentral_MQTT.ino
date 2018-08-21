#include <LowPower.h>

//INITIAL CONFIGURATION OF SIM800

#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
 
 //DEBUG Config
#define DUMP_AT_COMMANDS
#define DEBUG

#include <TinyGsmClient.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>

#define DTR_PIN 7

/*const char apn[]  = "claro.com.br";
const char user[] = "claro";
const char pass[] = "claro";*/

const char apn[]  = "timbrasil.br";
const char user[] = "tim";
const char pass[] = "tim";

SoftwareSerial SerialAT(4, 5);                                   // Serial Port configuration -(RX, TX) pins of SIM800L

//INITIAL CONFIGURATION OF MQTT
const char* broker = "200.129.43.208";
const char* user_MQTT = "teste@teste";               
const char* pass_MQTT = "123456";    

#define TOPIC "sensors/coleta"

//INITIAL CONFIGURATION OF NRF

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>

#define pinCE  8                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)
#define pinCSN  9                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);                                      // Declare object from nRF24 library (Create your wireless SPI)
RF24Network network(radio);                                     // Network uses that radio

#define id_origem 00                                  // Address of this node


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
  byte checksum;
};

//GLOBAL VARIABLES
const char ArraySize = 5;
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
    
    pinMode(DTR_PIN, OUTPUT);
    digitalWrite(DTR_PIN, LOW);
    
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
  attachInterrupt(0, receiveData, FALLING);                    // Attach the pin where the interruption is going to happen
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
  
  attachInterrupt(0, receiveData, FALLING);

  #ifdef DEBUG
    SerialMon.begin(57600);
    SerialMon.println(F("Arduino woke up"));
    SerialMon.flush();
    SerialMon.end();
  #endif

  if (dataReceived) {
    dataReceived = false;
    savePayload();

    if(ArrayCount == ArraySize){
      #ifdef DEBUG
        SerialMon.begin(57600);
        SerialMon.println(F("Waking GSM"));
        wakeGSM();
        SerialMon.println(F("Connecting to the server..."));
        connection();
        SerialMon.println(F("Publishing payload..."));
        publish(); 
        SerialMon.flush();
        SerialMon.end();
      #else
        wakeGSM();
        connection();
        publish(); 
      #endif
      
      ArrayCount = 0;
    }
  }

}

void savePayload(){
  ArrayPayloads[ArrayCount] = payload;
  ++ArrayCount;
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
  
  while (!network.available()) {                              // Keeps busy-waiting until the transmission of the payload is completed
    network.update();
  }

  while (network.available()) {                               // Reads the payload received
    network.read(header, &payload, sizeof(payload));
    
    bool checksum_OK = payload.checksum == getCheckSum((byte*) &payload);
    
    if(!checksum_OK){
      sendNotOK(header.from_node);
      dataReceived = false;
      return;
    }else{
      dataReceived = true;
    }

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
      
      if (checksum_OK) {
        SerialMon.println(F("Checksum matched!"));
      } else {
        SerialMon.println(F("Checksum didn't match!"));
      }
      SerialMon.flush();
      SerialMon.end();
    #endif
    
  }

}

byte getCheckSum(byte* payload) {
  byte payload_size = sizeof(payload_t);
  byte sum = 0;

  for (byte i = 0; i < payload_size - 1; i++) {
    sum += payload[i];
  }

  return sum;
}

void readVoltageGSM()
{
   payload.tensao_r = modem.getBattVoltage()/1000.0;
}

void publish(){
  for(int i = 0; i < ArraySize; ++i){
    String msg = String(ArrayPayloads[i].colmeia) + "," + String(ArrayPayloads[i].temperatura) + "," + String(ArrayPayloads[i].umidade) + "," + String(ArrayPayloads[i].tensao_c) + "," + String(ArrayPayloads[i].tensao_r);
    
    int length = msg.length();
    char msgBuffer[length];
    msg.toCharArray(msgBuffer,length+1);
      
    mqtt.publish(TOPIC, msgBuffer);
  }
  
}

void sendNotOK(uint16_t id_destino) {
  RF24NetworkHeader header(id_destino);                   // Sets the header of the payload   

  /* Create the payload to the sensor to resend the payload */
  const char payload[] = "NOT_OKAY"; 

  delay(50);
  /* Sends the data collected to the gateway, if delivery fails clears the fifo tx */
  if (!network.write(header, &payload, sizeof(payload))) { 
    radio.flush_tx();
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

