#include <SoftwareSerial.h>

#include <JeeLib.h>

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

ISR(WDT_vect) {
  Sleepy::watchdogEvent();
}

void setup() {  
  Serial.begin(57600);                                           // Start Serial communication
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

  /* Starts the process of waking up the sensors, receive the data and send to the web service */
  for (int i = 0; i < 1; ++i) {
    wakeSensors(i);                                            // Calls the function which sends a message to the sensor to wake up
    if (dataReceived) {                                        // Check if a payload was received. If true, sends the data to the web service.
      
    }
  }

  Serial.println("Powering down the nRF24L01");
  radio.powerDown();                                           // Calls the function to power down the nRF24L01

  Serial.println("Shutting Arduino down");
  Serial.end();

  int ok;                                                        // Local variable to know if the arduino slept the time we wanted
  for (int i = 0; i < 1; ++i) {
    do {
      ok = Sleepy::loseSomeTime(TEMPOENTRECADALEITURA);          // Function to put te arduino in sleep mode
    } while (!ok);
  }

  attachInterrupt(0, receberDados, FALLING);

  Serial.begin(57600);

  Serial.println("Arduino woke up");

  Serial.println("Powering up the nRF24L01");
  radio.powerUp();                                              // Calls the function to power up the nRF24L01

  Serial.flush();
  Serial.end();

}

void wakeSensors(int i) {
  dataReceived = false;
  RF24NetworkHeader header(ids_destino[i]);                   // Set the header of the payload
  char msg = 'H';                                             // Set the message to wake up the sensor node

  Serial.println("Say HI");

  //If the write fails let the user know over serial monitor
  if (!network.write(header, &msg, sizeof(msg))) {
    radio.flush_tx();                                         // Clean up the TX queue
    Serial.println("Command delivery failed");
  } else {
    radio.flush_tx();
    Serial.println("Success sending command");
    delay(2000);                                              // Waits for one second to receive the payload from the sensor
  }
}

void receberDados() {
  RF24NetworkHeader header;


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

    dataReceived = true;
  }

}
