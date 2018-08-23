#define TINY_GSM_MODEM_SIM800
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

//STRUCTURE OF OUR PAYLOAD
struct payload_t {
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
};

//INITIAL CONFIGURATION OF SIM800
/*const char apn[]  = "claro.com.br";
const char user[] = "claro";
const char pass[] = "claro";*/
const char apn[]  = "timbrasil.br";
const char user[] = "tim";
const char pass[] = "tim";

SoftwareSerial SerialAT(4, 5);                                   // Serial Port configuration -(RX, TX) pins of SIM800L

//INITIAL CONFIGURATION OF MQTT/THINGSPEAK
const char* broker = "200.129.43.208";

const char* DeviceId = "MQTT_FX_Client"; 
const char* ThingspeakUser = "teste@teste";               // Can be any name.
const char* ThingspeakPass = "123456";        // Change this your MQTT API Key from Account > MyProfile.

char* topic = "sensors/temperature";

StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
TinyGsmClient client(modem);
PubSubClient mqtt(client);
payload_t payload;    


void setup() {  
  SerialMon.begin(57600);                                           // Start Serial communication
  delay(10);

  SerialAT.begin(57600);
  delay(3000);
    
  pinMode(DTR_PIN, OUTPUT);
  digitalWrite(DTR_PIN, LOW);
  
  SerialMon.flush();
  SerialMon.end();
}

void loop() {
  SerialMon.begin(57600);

    
  SerialMon.println("Shutting Arduino down");
  SerialMon.end();

  //LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);                  // Function to put the arduino in sleep mode
  delay(15000);
    

  SerialMon.begin(57600);
  SerialMon.println("Arduino woke up");

  connection();
  publicar(payload); 

  SerialMon.flush();
  SerialMon.end();

}

void connection(){
  SerialMon.println("Inicializando GSM...");
  modem.restart();
 
  SerialMon.println("Aguardando rede...");
  modem.waitForNetwork();
    

  SerialMon.print("Conectando a ");
  SerialMon.print(apn);
  SerialMon.println("...");

  modem.gprsConnect(apn, user, pass);
  mqtt.setServer(broker,1883);
  
  SerialMon.println("Conectando ao broker...");

  mqtt.connect(DeviceId, ThingspeakUser, ThingspeakPass);
}

void lerTensaoGSM()
{
   payload.tensao_r = modem.getBattVoltage()/1000.0;
}

void publicar(payload_t DataOut){
  //String publishingMsg = "field1=" + String(DataOut.colmeia, DEC) + "&field2=" + String(DataOut.temperatura, DEC) + "&field3=" + String(DataOut.umidade, DEC) + "&field4=" + String(DataOut.tensao_c, DEC) + "&field5=" + String(DataOut.tensao_r, DEC) + "&status=MQTTPUBLISH";

  mqtt.publish(topic, "1-1-20");

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

