#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <DHT.h>
#include <JeeLib.h>
#include <ArduinoJson.h>

#define DHTPIN A0       // Pino DATA do Sensor DHT.
#define DHTTYPE DHT22   // Define o tipo do sensor utilizado DHT 11
#define IDCOLMEIA 1//ID da Colmeia monitorada
#define TEMPOENTRECADALEITURA 60000 //Tempo entre cada leitura 
#define SENSOR "Sensor 1"
#define PORTADHT 6 // PINO DE ATIVAÇÂO DO DHT

DHT dht(DHTPIN, DHTTYPE); //Objeto do sensor de temperatura

//CE_PIN 8, CS_PIN 9
RF24 radio(8, 9);                  // nRF24L01(+) radio attached using Getting Started board
RF24Network network(radio);          // Network uses that radio

const uint16_t id_origem = 01;        // Address of our node in Octal format
const uint16_t id_destino = 00;       // Address of the other node in Octal format

volatile bool teste = false;
struct payload_t {                  // Structure of our payload
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
};

float temperatura_lida = 0;
float umidade_lida = 0;
float co2_lido = 0;
float som_lido = 0;
float tensao_lida = 0;

int SENSORSOM = A0;
//int SENSORCO2 = 0;
int SENSORTENSAO = A4;


ISR(WDT_vect) {
  Sleepy::watchdogEvent();
}

void lerDHT() {
  if (isnan(dht.readTemperature())) {
    temperatura_lida = 0;
  }

  else {
    temperatura_lida = dht.readTemperature();
  }

  if (isnan(dht.readHumidity())) {
    umidade_lida = 0;
  }

  else {
    umidade_lida = dht.readHumidity();
  }
}

void lerMQandKy() {
  som_lido = analogRead(SENSORSOM);
  co2_lido = 0;
}

void lerTensao() {
  float valor_lido_tensao = analogRead(SENSORTENSAO);
  tensao_lida = ((valor_lido_tensao * 0.00489) * 5);
}

void setup(void)
{
  SPI.begin();
  radio.begin();
  radio.setPayloadSize(32);
  radio.setRetries(15, 15);
  radio.maskIRQ(1, 1, 0);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  network.begin(/*channel*/ 120, /*node address*/ id_origem);
  attachInterrupt(0, interruptFunction, FALLING);
}

void loop() {

  network.update();                          // Check the network regularly

  //ler sensores
  //lerMQandKy();
  //lerTensao();
  Serial.begin(57600);
  Serial.println("Sleep");
  Serial.flush();
  Serial.end();
  //delay(TEMPOENTRECADALEITURA);
  Sleepy::loseSomeTime(TEMPOENTRECADALEITURA);
  attachInterrupt(0, interruptFunction, FALLING);
  Serial.begin(57600);
  Serial.println("Wake");
  Serial.flush();
  Serial.end();
  delay(1000);
  if (teste) {
    lerDHT();
    enviarDados();
  }
}
void interruptFunction() {
  teste = true;
  radio.flush_rx();
}

void enviarDados() {
  //cria payload
  payload_t payload;
  RF24NetworkHeader header(id_destino);
  payload.colmeia = IDCOLMEIA;
  payload.temperatura = temperatura_lida;
  payload.umidade = umidade_lida;
  payload.tensao_c = tensao_lida;
  payload.tensao_r = 0;
  Serial.begin(57600);
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
  Serial.flush();
  Serial.end();
  bool ok = network.write(header, &payload, sizeof(payload));
  radio.flush_tx();
}



