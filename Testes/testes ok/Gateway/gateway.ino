#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>


RF24 radio(8, 9);               // nRF24L01(+) radio attached using Getting Started board

RF24Network network(radio);      // Network uses that radio
const uint16_t id_origem = 00;   // Address of the other node in Octal format
const uint16_t id_destino = 01;
unsigned long time;

struct payload_t {                  // Structure of our payload
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
};

void setup(void)
{
  Serial.begin(57600);
  SPI.begin();
  radio.begin();
  radio.setRetries(15, 15);
  radio.maskIRQ(1, 1, 0);
  radio.setPayloadSize(32);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  network.begin(/*channel*/ 120, /*node address*/ id_origem);
  attachInterrupt(0, receberDados, FALLING);
}

void loop(void) {

  if (millis() - time >= 5000) {
    time = millis();
    enviarDados();

  }
}

void enviarDados() {
  network.update();
  //cria payload
  payload_t payload;
  RF24NetworkHeader header(id_destino);
  payload.colmeia = 0;
  payload.temperatura = 0;
  payload.umidade = 0;
  payload.tensao_c = 0;
  payload.tensao_r = 0;
  Serial.println("Say HELLO");
  bool ok = network.write(header, &payload, sizeof(payload));
  Serial.println(ok);
  radio.flush_tx();
}

void receberDados() {
  Serial.println("the data");
  network.update();                  // Check the network regularly
  delay(500);
  while ( network.available() ) {     // Is there anything ready for us?

    RF24NetworkHeader header;        // If so, grab it and print it out
    payload_t payload;
    network.read(header, &payload, sizeof(payload));
    radio.flush_rx();

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
  }
}


