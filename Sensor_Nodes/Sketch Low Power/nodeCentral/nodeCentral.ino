
#include <SoftwareSerial.h>

#include <LowPower.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>

#define TEMPOENTRECADALEITURA 20000                             // Time between each reading in milliseconds 
#define PORTADTR 7
#define DEBUG

//INITIAL CONFIGURATION OF NRF
const int pinCE = 8;                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 9;                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);                                      // Declare object from nRF24 library (Create your wireless SPI)
RF24Network network(radio);                                     // Network uses that radio

const uint16_t id_origem = 00;                                  // Address of this node
const uint16_t ids_destino[3] = {01, 02, 03};                   // Addresses of the others nodes

//STRUCTURE OF OUR PAYLOAD
struct payload_t {
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
};

//GLOBAL VARIABLES
payload_t payload;                                              // Used to store the payload from the sensor node
bool dataReceived;                                              // Used to know whether a payload was received or not


void setup() {  
  Serial.begin(57600);                                           // Start Serial communication
  
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


  Serial.println("Shutting Arduino down");
  Serial.end();

  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);                  // Function to put the arduino in sleep mode
  
  attachInterrupt(0, receberDados, FALLING);

  Serial.begin(57600);
  Serial.println("Arduino woke up");


  if (dataReceived) {
    //Envia dados pelo GSM
    dataReceived = false;
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
