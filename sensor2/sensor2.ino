#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>
#include <DHT.h>
#include <JeeLib.h>

#define DHTPIN A0       // Pino DATA do Sensor DHT.
#define DHTTYPE DHT22   // Define o tipo do sensor utilizado DHT 11
#define IDCOLMEIA 2//ID da Colmeia monitorada
#define TEMPOENTRECADALEITURA 60000 //Tempo entre cada leitura 
#define PORTADHT 7 // PINO DE ATIVAÇÂO DO DHT
#define PORTATENSAO 5 // PINO DE ATIVAÇÂO DO SENSOR DE TENSÃO
#define SENSOR "Sensor 2"

DHT dht(DHTPIN, DHTTYPE); //Objeto do sensor de temperatura

const int pinCE = 8; //This pin is used to set the nRF24 to standby (0) or active mode (1)

const int pinCSN = 9; //This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);           // nRF24L01(+) radio attached using Getting Started board
RF24Network network(radio);          // Network uses that radio

const uint16_t id_origem = 02; // Address of this node
const uint16_t id_destino = 00; //Addresses of the master node

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

int SENSORSOM = A2;
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
  valor_lido_tensao = analogRead(SENSORTENSAO);
  tensao_lida = ((valor_lido_tensao * 0.00489) * 5);
  
}

void setup(void)
{
  SPI.begin();
  radio.begin();
  radio.setRetries(15, 15);
  radio.maskIRQ(1, 1, 0);
  radio.setPayloadSize(32);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  attachInterrupt(0, interruptFunction, FALLING);
  network.begin(/*channel*/ 120, /*node address*/ id_origem);

  pinMode(PORTATENSAO, OUTPUT);
  pinMode(PORTADHT, OUTPUT);

  digitalWrite(PORTATENSAO, LOW);
  digitalWrite(PORTADHT, LOW);
}

void loop() {
  network.update(); // Check the network regularly
  Serial.begin(57600);
  Serial.println("Sleep");
  Serial.flush();
  Serial.end();
    
  teste = false;

  Sleepy::loseSomeTime(TEMPOENTRECADALEITURA);
  attachInterrupt(0, interruptFunction, FALLING);

  Serial.begin(57600);
  Serial.println("Wake");
  Serial.flush();
  Serial.end();
  digitalWrite(PORTADHT, HIGH);
  digitalWrite(PORTATENSAO, HIGH);
  if (teste) {
    //Ler sensores

    //lerMQandKy();
    delay(1000);
    lerTensao();
    lerDHT();

    //Enviar Dados
    enviarDados();
  }
  digitalWrite(PORTADHT, LOW);
  digitalWrite(PORTATENSAO, LOW);


}
void interruptFunction() {
  teste = true;
  radio.flush_rx();
}

void enviarDados() {
  Serial.begin(57600);
  Serial.println("Sending data");
  Serial.flush();
  Serial.end();
  delay(50);
    
  RF24NetworkHeader header(id_destino); 
  payload_t payload;
  payload.colmeia = IDCOLMEIA;
  payload.temperatura = temperatura_lida;
  payload.umidade = umidade_lida;
  payload.tensao_c = tensao_lida;
  payload.tensao_r = 0;

  if (!network.write(header,&payload, sizeof(payload))) { //if the write fails let the user know over serial monitor
    radio.flush_tx();
    Serial.begin(57600);
    Serial.println("Data delivery failed");
    Serial.flush();
    Serial.end();

  } else {
    Serial.begin(57600);
    Serial.println("Success sending data: ");
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
   }
}



