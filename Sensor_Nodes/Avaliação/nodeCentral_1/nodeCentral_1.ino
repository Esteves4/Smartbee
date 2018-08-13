#define TINY_GSM_MODEM_SIM800
#define Timeout 10000
#define Timeout_envios 20000

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

#define DTR_PIN 7
#define DEBUG

//INITIAL CONFIGURATION OF NRF
const int pinCE = 8;                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 9;                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);                                      // Declare object from nRF24 library (Create your wireless SPI)
RF24Network network(radio);                                     // Network uses that radio

const uint16_t id_origem = 00;                                  // Address of this node

//INITIAL CONFIGURATION OF SIM800
const char apn[]  = "timbrasil.br";
const char user[] = "tim";
const char pass[] = "tim";
SoftwareSerial SIM800L(4, 5);                                   // Serial Port configuration -(RX, TX) pins of SIM800L

//INITIAL CONFIGURATION OF MQTT/THINGSPEAK
const char* broker = "mqtt.thingspeak.com";

const char* ThingspeakUser = "esteves4";                // Can be any name.
const char* ThingspeakPass = "FWZ6OTXMYGXC5GKW";        // Change this your MQTT API Key from Account > MyProfile.
const char* WriteApiKey = "X1H7B6RD67MHVGIZ";           // Change to your channel Write API Key.

long ChannelID = 419812;

TinyGsm modem(SIM800L);
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
int resetCooldown = 1;                                          // Cooldown de tentativas de conexão, que, caso ultrapassado um certo valor, força o reset do Sim800L
int connection_retry = 0;                                       // Quantidade de tentativas de reconexão

void connection() //Função que tenta realizar todas as etapas da conexão com o broker MQTT
//Se em qualquer etapa a conexão falhar > 3 vezes, código para de tentar realizar tentativas de conexão
//Para que force, mais na frente no código, o reset do Sim800L
{
  Serial.println("Inicializando GSM...");
  modem.restart();
  Serial.println("Ok");

  Serial.println("Aguardando rede...");
  while (!modem.waitForNetwork())
  {
    connection_retry++;
    Serial.println("Sem acesso a rede...");
    Serial.print("Tentando novamente em ");
    Serial.print("Número de tentativas - ");
    Serial.println(connection_retry);
    if (connection_retry >= 3)
    {
      Serial.println("Falha.");
      break;
    }
    Serial.println("Sucesso.");
    delay(Timeout);
  }
  connection_retry = 0;
  

  Serial.print("Conectando a ");
  Serial.print(apn);
  Serial.println("...");
  while (!modem.gprsConnect(apn, user, pass))
  {
    connection_retry++;
    Serial.println("Tentando reconectar a APN...");
    Serial.print("Número de tentativas - ");
    Serial.println(connection_retry);
    if (connection_retry >= 3)
    {
      break;
    }
    delay(Timeout);
  }
  connection_retry = 0;
  Serial.println("Sucesso.");

  mqtt.setServer(broker, 1883);
  Serial.println("Conectando ao broker...");
  while (!mqtt.connect("CentralNode", ThingspeakUser, ThingspeakPass))
  {
    connection_retry++;
    Serial.println("Tentando reconectar ao broker...");
    Serial.print("Número de tentativas - ");
    Serial.println(connection_retry);
    if (connection_retry >= 3)
    {
      break;
    }
    delay(Timeout);
  }
  connection_retry = 0;
  Serial.print("Sucesso.");
}

void setup() {  
  Serial.begin(57600);                                           // Start Serial communication
  
  SIM800L.begin(57600);
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

void receberDados() {
  RF24NetworkHeader header;
  Serial.begin(57600);

  while (!network.available()) {                              // Keeps busy-waiting until the transmission of the payload is completed
    network.update();
  }

  while (network.available()) {                               // Reads the payload received
    network.read(header, &payload, sizeof(payload));

  lerTensaoGSM();
  
  #ifdef DEBUG
    printData(payload);
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
  String publishingMsg = "field1=" + String(DataOut.colmeia) + "&field2=" + String(DataOut.temperatura) + "&field3=" + String(DataOut.umidade) + "&field4=" + String(DataOut.tensao_c) + "&field5=" + String(DataOut.tensao_r);
  String topicString = "channels/" + String( ChannelID ) + "/publish/"+String(WriteApiKey);

  int length = publishingMsg.length();
  char msgBuffer[length];
  publishingMsg.toCharArray(msgBuffer,length+1);

  length = topicString.length();
  char topicBuffer[length];
  topicString.toCharArray(topicBuffer,length+1);
  
  mqtt.publish(topicBuffer, msgBuffer);
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

void printData(payload_t data){
    Serial.print("Received data from sensor: ");
    Serial.println(data.colmeia);

    Serial.println("The data: ");
    Serial.print(data.colmeia);
    Serial.print(" ");
    Serial.print(data.temperatura);
    Serial.print(" ");
    Serial.print(data.umidade);
    Serial.print(" ");
    Serial.print(data.tensao_c);
    Serial.print(" ");
    Serial.println(data.tensao_r);
}



