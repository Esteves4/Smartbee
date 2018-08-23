#define TINY_GSM_MODEM_SIM800
#define Timeout 2000
#define Timeout_envios 2000;
#define SerialMon Serial
#define SerialAT Serial1

#include <StreamDebugger.h>
#include <TinyGsmClient.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>

#include <LowPower.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>

#define TEMPOENTRECADALEITURA 20000                             // Time between each reading in milliseconds 
#define DTR_PIN 7
#define DEBUG

//INITIAL CONFIGURATION OF NRF
const int pinCE = 8;                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 9;                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);                                      // Declare object from nRF24 library (Create your wireless SPI)
RF24Network network(radio);                                     // Network uses that radio

const uint16_t id_origem = 00;                                  // Address of this node
const uint16_t ids_destino[3] = {01, 02, 03};                   // Addresses of the others nodes

//INITIAL CONFIGURATION OF SIM800
const char apn[]  = "claro.com.br";
const char user[] = "claro";
const char pass[] = "claro";
SoftwareSerial SerialAT(4, 5);                                   // Serial Port configuration -(RX, TX) pins of SIM800L

//INITIAL CONFIGURATION OF MQTT/THINGSPEAK
const char* broker = "mqtt.thingspeak.com";

const char* ThingspeakUser = "esteves4";                // Can be any name.
const char* ThingspeakPass = "FWZ6OTXMYGXC5GKW";        // Change this your MQTT API Key from Account > MyProfile.
const char* WriteApiKey = "X1H7B6RD67MHVGIZ";           // Change to your channel Write API Key.

long ChannelID = 419812;

StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

//STRUCTURE OF OUR PAYLOAD
struct payload_t {
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
};

//GLOBAL VARIABLES
const uint8_t ArraySize = 1;
uint8_t ArrayCount = 0;
payload_t ArrayPayloads[ArraySize];
payload_t payload;                                              // Used to store the payload from the sensor node

bool dataReceived;                                              // Used to know whether a payload was received or not

void setup() {  
  SerialMon.begin(57600);                                           // Start Serial communication
  delay(10);

  SerialAT.begin(57600);
  delay(3000);
    
  /* nRF24L01 configuration*/
  Serial.println("Initializing nRF24L01...");
  SPI.begin();                                                  // Start SPI protocol
  radio.begin();                                                // Start nRF24L01
  radio.maskIRQ(1, 1, 0);                                       // Create a interruption mask to only generate interruptions when receive payloads
  radio.setPayloadSize(32);                                     // Set payload Size
  radio.setPALevel(RF24_PA_LOW);                                // Set Power Amplifier level
  radio.setDataRate(RF24_250KBPS);                              // Set transmission rate
  attachInterrupt(0, receberDados, FALLING);                    // Attach the pin where the interruption is going to happen
  network.begin(/*channel*/ 120, /*node address*/ id_origem);   // Start the network

  pinMode(DTR_PIN, OUTPUT);
  digitalWrite(DTR_PIN, LOW);
  
  Serial.flush();
  Serial.end();
}

void loop() {
  network.update();                                            // Check the network regularly

  Serial.begin(57600);

  if (radio.rxFifoFull()) {                                     // If the RX FIFO is full, the RX FIFO is cleared
    radio.flush_rx();
  } else if (radio.txFifoFull()) {                              // If the TX FIFO is full, the TX FIFO is cleared
    radio.flush_tx();
  }

  Serial.println("Shutting SIM800L down");
  sleepGSM();
  
  
  Serial.println("Shutting Arduino down");
  Serial.end();

  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);                  // Function to put the arduino in sleep mode
  
  attachInterrupt(0, receberDados, FALLING);

  Serial.begin(57600);
  Serial.println("Arduino woke up");

  Serial.println("Waking GSM");
  wakeGSM();


  if (dataReceived) {
    dataReceived = false;
    ArrayPayloads[ArrayCount] = payload;
    ++ArrayCount;
  }


  if(ArrayCount == ArraySize){
    detachInterrupt(0);
    
    for(int i = 0; i < ArraySize; ++i){
      connection();
      publicar(ArrayPayloads[i]); 
    }
    
    ArrayCount = 0;
    
    attachInterrupt(0, receberDados, FALLING);
  }

  
  Serial.flush();
  Serial.end();

}

void connection(){
  Serial.println("Inicializando GSM...");
  modem.restart();
 
  Serial.println("Aguardando rede...");
  modem.waitForNetwork();
    

  Serial.print("Conectando a ");
  Serial.print(apn);
  Serial.println("...");

  modem.gprsConnect(apn, user, pass);
  mqtt.setServer(broker, 1883);
  
  Serial.println("Conectando ao broker...");

  mqtt.connect("CentralNode", ThingspeakUser, ThingspeakPass);
}

void receberDados() {
  RF24NetworkHeader header;
  Serial.begin(57600);

  while (!network.available()) {                              // Keeps busy-waiting until the transmission of the payload is completed
    network.update();
  }

  while (network.available()) {                               // Reads the payload received
    network.read(header, &payload, sizeof(payload));

#ifdef DEBUG
    Serial.print("Received data from sensor: ");
    Serial.println(payload.colmeia);

    Serial.println("The data: ");
    //Serial.print("Colmeia: ");
    Serial.print(payload.colmeia);
    Serial.print(" ");
    //Serial.print("Temperatura: ");
    Serial.print(payload.temperatura);
    Serial.print(" ");
    //Serial.print("Umidade: ");
    Serial.print(payload.umidade);
    Serial.print(" ");
    //Serial.print("Tensao sensor: ");
    Serial.print(payload.tensao_c);
    Serial.print(" ");
    //Serial.print("Tensao repetidor: ");
    Serial.println(payload.tensao_r);
#endif

  Serial.flush();
  Serial.end();

    dataReceived = true;
  }

}


void lerTensaoGSM()
{
   payload.tensao_r = modem.getBattVoltage()/1000.0;
}

void publicar(payload_t DataOut){
  String publishingMsg = "field1=" + String(DataOut.colmeia, DEC) + "&field2=" + String(DataOut.temperatura, DEC) + "&field3=" + String(DataOut.umidade, DEC) + "&field4=" + String(DataOut.tensao_c, DEC) + "&field5=" + String(DataOut.tensao_r, DEC);
  String topicString = "channels/" + String( ChannelID ) + "/publish/"+String(WriteApiKey);

  int length = publishingMsg.length();
  char msgBuffer[length];
  publishingMsg.toCharArray(msgBuffer,length+1);

  length = topicString.length();
  char topicBuffer[length];
  topicString.toCharArray(topicBuffer,length+1);
  
  mqtt.publish(topicBuffer, msgBuffer);
  Serial.println("Erro: " + mqtt.state());

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

