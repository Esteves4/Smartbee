#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>
#include <DHT.h>
#include <JeeLib.h>

#define DHTPIN A0                                               // Pin DATA of the DHT sensor.
#define DHTTYPE DHT22                                           // Sets the type of DHT utilized, DHT 22
#define IDCOLMEIA 2                                             // ID of the Hive monitored
#define TEMPO_ENTRE_CADA_LEITURA 60000                          // Time between each reading in milliseconds  
#define SENSOR "Sensor 2"                                       // Name of the sensor
#define PORTADHT 7                                              // Activation pin of DHT
#define PORTATENSAO 5                                           // Activation pin of the voltage sensor
#define DEBUG

DHT dht(DHTPIN, DHTTYPE);                                       // Object of the temperature sensor

const int pinCE = 8;                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)

const int pinCSN = 9;                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);                                      // nRF24L01(+) radio attached using Getting Started board
RF24Network network(radio);                                     // Network uses that radio

const uint16_t id_origem = 02;                                  // Address of this node
const uint16_t id_destino = 00;                                 // Addresses of the master node

volatile bool wasInterrupted = false;                           // Variable to know if a interruption ocurred or not

struct payload_t {                                              // Structure of our payload
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
};

/* Variables that hold our readings */
float temperatura_lida = 0;
float umidade_lida = 0;
float co2_lido = 0;
float som_lido = 0;
float tensao_lida = 0;

/* Analagic ports for reading */
int SENSORSOM = A2;
//int SENSORCO2 = 0;
int SENSORTENSAO = A4;

ISR(WDT_vect) {
  Sleepy::watchdogEvent();
}

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
  #ifdef DEBUG
  Serial.begin(57600);
  Serial.println("Initializing nRF24L01...");
  Serial.flush();
  Serial.end();
  #endif
  
  SPI.begin();                                                  // Start SPI protocol
  radio.begin();                                                // Start nRF24L01
  radio.maskIRQ(1, 1, 0);                                       // Create a interruption mask to only generate interruptions when receive payloads
  radio.setPayloadSize(32);                                     // Set payload Size
  radio.setPALevel(RF24_PA_LOW);                                // Set Power Amplifier level
  radio.setDataRate(RF24_250KBPS);                              // Set transmission rate
  attachInterrupt(0, interruptFunction, FALLING);               // Attach the pin where the interruption is going to happen
  network.begin(/*channel*/ 120, /*node address*/ id_origem);   // Start the network

  /* Sensors pins configuration. Sets the activation pins as OUTPUT and write LOW  */
  pinMode(PORTATENSAO, OUTPUT);
  pinMode(PORTADHT, OUTPUT);

  digitalWrite(PORTATENSAO, LOW);
  digitalWrite(PORTADHT, LOW);

  /* Diables peripherals that we don't use */
  disableTimer1();
  disableTimer2();
  disableWire();
}

void loop() {
  network.update();                                            // Check the network regularly

  #ifdef DEBUG
  Serial.begin(57600);
  Serial.println("Sleep");
  Serial.flush();
  Serial.end();
  #endif

  wasInterrupted = false;

  if (radio.rxFifoFull()) {                                    // If the RX FIFO is full, the RX FIFO is cleared
    radio.flush_rx();
  } else if (radio.txFifoFull()) {                             // If the TX FIFO is full, the TX FIFO is cleared
    radio.flush_tx();
  }

  /* Disable the peripherals we're not using during sleep */
  disableSerial();
  disableADC();

  Sleepy::loseSomeTime(TEMPO_ENTRE_CADA_LEITURA);                 // Function to put the arduino in sleep mode

  attachInterrupt(0, interruptFunction, FALLING);

  /* Enable the peripherals we're going to use */
  enableSerial();
  enableADC();

  #ifdef DEBUG
  Serial.begin(57600);
  Serial.println("Wake");
  Serial.flush();
  Serial.end();
  #endif

  /* If the arduino was interrupted, collects the readings and sends the data back to the gateway */
  if (wasInterrupted) {

    /* Turn on the sensors */
    digitalWrite(PORTADHT, HIGH);
    digitalWrite(PORTATENSAO, HIGH);

    delay(500);

    /* Performs the readings */
    //lerMQandKy();
    lerTensao();
    lerDHT();
    
    enviarDados();                                              // Sends the data to the gateway

    /* Turn off the sensors */
    digitalWrite(PORTADHT, LOW);
    digitalWrite(PORTATENSAO, LOW);
  }

}

void interruptFunction() {
  wasInterrupted = true;
  radio.flush_rx();
}

void enviarDados() {
  #ifdef DEBUG
  Serial.begin(57600);
  Serial.println("Sending data");
  Serial.flush();
  Serial.end();
  delay(50);
  #endif

  RF24NetworkHeader header(id_destino);                   // Sets the header of the payload   

  /* Create the payload with the collected readings */
  payload_t payload;                                
  payload.colmeia = IDCOLMEIA;
  payload.temperatura = temperatura_lida;
  payload.umidade = umidade_lida;
  payload.tensao_c = tensao_lida;
  payload.tensao_r = 0;

  /* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */
  if (!network.write(header, &payload, sizeof(payload))) { 
    radio.flush_tx();
    #ifdef DEBUG
    Serial.begin(57600);
    Serial.println("Data delivery failed");
    Serial.flush();
    Serial.end();
    #endif

  } else {
    #ifdef DEBUG
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
    #endif
  }
}

void disableWire() {
#ifdef PRTWI
  PRR |= _BV(PRTWI);
#endif
#ifdef PRUSI
  PRR |= _BV(PRUSI);
#endif
}
void disableTimer2() {
#ifdef PRTIM2
  PRR |= _BV(PRTIM2);
#endif
}
void disableTimer1() {
  PRR |= _BV(PRTIM1);
}
void disableMillis() {
  PRR |= _BV(PRTIM0);
}
void disableSerial() {
#ifdef PRUSART0
  PRR |= _BV(PRUSART0);
#endif
#ifdef PRUSART
  PRR |= _BV(PRUSART);
#endif
}
void disableADC() {
#ifdef PRADC
  PRR |= _BV(PRADC);
#endif
}
void disableSPI() {
#ifdef PRSPI
  PRR |= _BV(PRSPI);
#endif
}

void enableWire() {
#ifdef PRTWI
  PRR &= ~_BV(PRTWI);
#endif
#ifdef PRUSI
  PRR &= ~_BV(PRUSI);
#endif
}
void enableTimer2() {
#ifdef PRTIM2
  PRR &= ~_BV(PRTIM2);
#endif
}
void enableTimer1() {
  PRR &= ~_BV(PRTIM1);
}
void enableMillis() {
  PRR &= ~_BV(PRTIM0);
}
void enableSerial() {
#ifdef PRUSART0
  PRR &= ~_BV(PRUSART0);
#endif
#ifdef PRUSART
  PRR &= ~_BV(PRUSART);
#endif
}
void enableADC() {
#ifdef PRADC
  PRR &= ~_BV(PRADC);
#endif
}
void enableSPI() {
#ifdef PRSPI
  PRR &= ~_BV(PRSPI);
#endif
}


