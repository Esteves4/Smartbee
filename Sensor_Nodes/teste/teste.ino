#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>

#define IDCOLMEIA 1                                             // ID of the Hive monitored
#define TEMPO_ENTRE_CADA_LEITURA 300                            // Time between each reading in seconds  
#define SENSOR "Sensor 1"                                       // Name of the sensor
#define PORTADHT 6                                              // Activation pin of DHT
#define DEBUG

//INITIAL CONFIGURATION OF NRF24L01
const int pinCE = 8;                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 9;                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);                                      // nRF24L01(+) radio attached using Getting Started board
RF24Network network(radio);                                     // Network uses that radio

const uint16_t id_origem = 00;                                  // Address of this node
const uint16_t id_destino = 00;                                 // Addresses of the master node
uint16_t count = 0;

//STRUCTURE OF OUR PAYLOAD
struct payload_t {
	char colmeia;
	float temperatura;
	float umidade;
	float tensao_c;
	float tensao_r;
	float peso;
	char erro_vec;
};

#define audio_size 50

struct payload_a {
  char colmeia;
  uint16_t audio[audio_size];
};

#define E_DHT   0
#define E_TEN_C 1
#define E_TEN_R 2
#define E_RTC   3
#define E_PESO  4
#define E_SD    5
#define E_SRAM  6

payload_t payload;
payload_a payload_audio;   

/* Variables that hold our readings */
float temperatura_lida = 0;
float umidade_lida = 0;
float co2_lido = 0;
float som_lido = 0;
float tensao_lida = 0;
float peso_lido = 0;



void setup(void) {

	/* nRF24L01 configuration*/ 
	//SPI.begin();                                                // Start SPI protocol
	radio.begin();                                                // Start nRF24L01
	radio.maskIRQ(1, 1, 0);                                       // Create a interruption mask to only generate interruptions when receive payloads
	radio.setPayloadSize(32);                                     // Set payload Size
	radio.setPALevel(RF24_PA_HIGH);                                // Set Power Amplifier level
	radio.setDataRate(RF24_1MBPS);                              // Set transmission rate
	network.begin(/*channel*/ 120, /*node address*/ id_origem);   // Start the network
  Serial.begin(57600);
  Serial.println("Initiated");
}

void loop() {
	network.update();                                            // Check the network regularly
  
  receiveData();
}


void receiveData() {
  RF24NetworkHeader header;
  
  network.update();
  

  if(network.available()) {

    RF24NetworkHeader header;                            // If so, take a look at it
    network.peek(header);
    char c;
    switch (header.type){                              // Dispatch the message to the correct handler.
    case 'D': handle_data(header); break;
    case 'A': handle_audio(header); break;
    case 'S': network.read(header, &c, 1); Serial.println("START"); break;
    case 's': network.read(header, &c, 1); Serial.println("STOP"); break;
    }


  } 
}

void handle_audio(RF24NetworkHeader& header){

  network.read(header, &payload_audio, sizeof(payload_audio));                                   // Reads the payload received
   

  Serial.println("Audio received");

}

void handle_data(RF24NetworkHeader& header){

  network.read(header, &payload, sizeof(payload));                                   // Reads the payload received
 
  Serial.println("Data received");

}
