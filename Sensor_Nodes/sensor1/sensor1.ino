#include <LowPower.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>
#include <DHT.h>

#define DHTPIN A0                                               // Pin DATA of the DHT sensor.
#define DHTTYPE DHT11                                           // Sets the type of DHT utilized, DHT 22
#define IDCOLMEIA 1                                             // ID of the Hive monitored
#define TEMPO_ENTRE_CADA_LEITURA 30                             // Time between each reading in seconds  
#define SENSOR "Sensor 1"                                       // Name of the sensor
#define PORTADHT 6                                              // Activation pin of DHT
#define PORTATENSAO 5                                           // Activation pin of the voltage sensor
#define DEBUG

DHT dht(DHTPIN, DHTTYPE);                                       // Object of the temperature sensor

const int pinCE = 8;                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)

const int pinCSN = 9;                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);                                      // nRF24L01(+) radio attached using Getting Started board
RF24Network network(radio);                                     // Network uses that radio

const uint16_t id_origem = 01;                                  // Address of this node
const uint16_t id_destino = 00;                                 // Addresses of the master node

volatile bool interrupted = false;                           // Variable to know if a interruption ocurred or not

struct payload_t {                                              // Structure of our payload
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
  byte checksum;
};

/* Variables that hold our readings */
float temperatura_lida = 0;
float umidade_lida = 0;
float co2_lido = 0;
float som_lido = 0;
float tensao_lida = 0;

/* Analogic ports for reading */
int SENSORSOM = A0;
//int SENSORCO2 = 0;
int SENSORTENSAO = A4;

/* Reads the temperature and the humidity from DHT sensor */
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

/* Reads the sound */
void lerMQandKy() {
  som_lido = analogRead(SENSORSOM);
  co2_lido = 0;
}

/*Reads the voltage of the battery */
void lerTensao() {
  float valor_lido_tensao = analogRead(SENSORTENSAO);
  tensao_lida = ((valor_lido_tensao * 0.00489) * 5);
}

void setup(void) {
  /* nRF24L01 configuration*/ 
  SPI.begin();                                                  // Start SPI protocol
  radio.begin();                                                // Start nRF24L01
  radio.maskIRQ(1, 1, 0);                                       // Create a interruption mask to only generate interruptions when receive payloads
  radio.setPayloadSize(32);                                     // Set payload Size
  radio.setPALevel(RF24_PA_LOW);                                // Set Power Amplifier level
  radio.setDataRate(RF24_250KBPS);                              // Set transmission rate
  network.begin(/*channel*/ 120, /*node address*/ id_origem);   // Start the network

  /* Sensors pins configuration. Sets the activation pins as OUTPUT and write LOW  */
  pinMode(PORTADHT, OUTPUT);
  digitalWrite(PORTADHT, HIGH);
  delay(2000);
  digitalWrite(PORTADHT, LOW);

}

void loop() {
  network.update();                                            // Check the network regularly

  if (radio.rxFifoFull()) {                                    // If the RX FIFO is full, the RX FIFO is cleared
    radio.flush_rx();
  } else if (radio.txFifoFull()) {                             // If the TX FIFO is full, the TX FIFO is cleared
    radio.flush_tx();
  }
  
  radio.powerDown();                                           // Calls the function to power down the nRF24L01

  for(int i = 0; i < TEMPO_ENTRE_CADA_LEITURA; ++i){
      LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);          // Function to put the arduino in sleep mode
  }

  radio.powerUp();                                             // Calls the function to power up the nRF24L01
 
  /* Turn on the sensors */
  digitalWrite(PORTADHT, HIGH);
  delay(200);

  /* Performs the readings */
  lerDHT();
  
  enviarDados();                                              // Sends the data to the gateway

  /* Turn off the sensors */
  digitalWrite(PORTADHT, LOW);
}

void interruptFunction() {
  interrupted = true;
  radio.flush_rx();
}

void enviarDados() {
  RF24NetworkHeader header(id_destino);                   // Sets the header of the payload   

  /* Create the payload with the collected readings */
  payload_t payload;                                
  payload.colmeia = IDCOLMEIA;
  payload.temperatura = temperatura_lida;
  payload.umidade = umidade_lida;
  payload.tensao_c = tensao_lida;
  payload.tensao_r = 0;
  payload.checksum = getCheckSum((byte*) &payload);

  delay(50);
  /* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */
  if (!network.write(header, &payload, sizeof(payload))) { 
    radio.flush_tx();
  }
}

byte getCheckSum(byte* payload) {
  byte payload_size = sizeof(payload_t);
  byte sum = 0;

  for (byte i = 0; i < payload_size - 1; i++) {
    sum += payload[i];
  }

  return sum;
}


