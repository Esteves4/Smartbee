#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>

const int pinCE = 8; //This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 9; //This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN); // Declare object from nRF24 library (Create your wireless SPI)
RF24Network network(radio); // Network uses that radio

const uint16_t id_origem = 00; // Address of this node
const uint16_t ids_destino[3] = {01, 02, 03}; //Addresses of the others nodes

struct payload_t {                  // Structure of our payload
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
};

void setup(void) {
  Serial.begin(9600);
  SPI.begin();
  radio.begin();
  radio.maskIRQ(1, 1, 0);
  radio.setPayloadSize(32);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  attachInterrupt(0, receberDados, FALLING);
  network.begin(/*channel*/ 120, /*node address*/ id_origem);
}

void loop(void) {
  network.update(); // Check the network regularly

  for (int i = 0; i < 3; ++i) {
    delay(5000);
    acordarSensores(i);
  }
}

void acordarSensores(int i) {

  RF24NetworkHeader header(ids_destino[i]);
  char msg = 'H';                            //Cria mensagem para acordar Node do sensor

  Serial.println("Say HI");

  //if the write fails let the user know over serial monitor
  if (!network.write(header, &msg, sizeof(msg))) {
    radio.flush_tx();
    Serial.println("Command delivery failed");
  } else {
    radio.flush_tx();
    Serial.println("Success sending command");
    delay(1000);
    
  }
}

bool receberDados() {
  RF24NetworkHeader header;
  payload_t payload; //used to store payload from sensor node
  
  while(!network.available()){
    network.update();
  }

  while (network.available()) {
    network.read(header, &payload, sizeof(payload));

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
    return true;
  }
  return false;
}



