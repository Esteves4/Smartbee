// ### INCLUDES ###
#include <LowPower.h>
#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>
#include <DHT.h>

#include <HX711.h>

#include <MicrochipSRAM.h>

// ### DEFINES ###
#define IDCOLMEIA 1
#define TEMPO_ENTRE_CADA_LEITURA 300
#define SENSOR "Sensor 1"
#define PORTADHT 6
#define DEBUG

#define DOUT  3
#define CLK  4

#define DHTPIN A1
#define DHTTYPE DHT22

#define SRAM_SS_PIN 7

#define audio_size 50
#define SAMPLE_FRQ 19200
#define MAX_ADDR 76800 //(SAMPLE_FRQ*4)

#define E_DHT   0
#define E_TEN_C 1
#define E_TEN_R 2
#define E_RTC   3
#define E_PESO  4
#define E_SD    5
#define E_SRAM  6

// ### CONST ###
float SCALE_FACTOR = 42990.00;
double offset = 149252;

const int pinCE = 8;
const int pinCSN = 9;

const uint16_t sourceID = 01;
const uint16_t destinationID = 00;

int VOLTAGE_PIN = A2;

// ### GLOBAL VARIABLES ###
uint16_t count = 0;
bool isFreeRunning = false;

struct payload_t {
  char hive;
  float temperature;
  float humidity;
  float voltage_h;
  float voltage_r;
  float weight;
  char erro_vec;
};

struct payload_a {
  char hive;
  uint16_t audio[audio_size];
};

payload_t payload;
payload_a audioPayload;

float temperatureReading = 0;
float humidityReading = 0;
float voltageReading = 0;
float weightReading = 0;

uint32_t strAddr = 0;
uint16_t bufferADC;
volatile uint8_t bufferADC_H;
volatile uint8_t bufferADC_L;
unsigned long start;
unsigned long end;

volatile bool interrupted = false;
bool sleep = false;

uint16_t adcsraBackup = 0;
uint16_t adcsrbBackup = 0;

// ### OBJECTS ###
HX711 scale(DOUT, CLK);

//INITIAL CONFIGURATION OF DHT
DHT dht(DHTPIN, DHTTYPE);

//INITIAL CONFIGURATION OF 23LC1024 - SRAM
static MicrochipSRAM memory(SRAM_SS_PIN);

//INITIAL CONFIGURATION OF NRF24L01
RF24 radio(pinCE,pinCSN);

RF24Network network(radio);

/* Reads the temperature and the humidity from DHT sensor */
void readDHT() {
  if (isnan(dht.readTemperature())) {
    payload.erro_vec |= (1 << E_DHT);
    temperatureReading = 0;
  } else {
    temperatureReading = dht.readTemperature();
  }

  if (isnan(dht.readHumidity())) {
    payload.erro_vec |= (1 << E_DHT);
    humidityReading = 0;
  } else {
    humidityReading = dht.readHumidity();
  }
}

/*Reads the voltage of the battery */
void readVoltage() {
// read the input on analog pin 0:
  int sensorValue = 0;

  for (byte i = 0; i < 10; i++) {
    sensorValue += analogRead(VOLTAGE_PIN);
  }

  sensorValue = sensorValue / 10;

// Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  voltageReading = sensorValue * 0.00487586;
}

void readWeight() {
  scale.power_up();

  weightReading = scale.get_units(10);

  if (weightReading < 0) {
    if (weightReading < -0.100) {
      payload.erro_vec |= (1 << E_PESO);
    }
    weightReading = 0;
  }

  scale.power_down();

  Serial.begin(57600);
  Serial.print("#Weight: ");
  Serial.print(weightReading, 3);
  Serial.println(" kg");
  Serial.flush();
  Serial.end();
}

void analogRead_freeRunnig(uint8_t pin) {
  if (pin < 0 || pin > 7) {
    return;
  }

  adcsraBackup = ADCSRA;
  adcsrbBackup = ADCSRB;

// clear ADCSRA register
  ADCSRA = 0;
// clear ADCSRB register        
  ADCSRB = 0;             
// set A0 analog input pin
  ADMUX |= (pin & 0x07); 
// set reference voltage 
  ADMUX |= (1 << REFS0);  
//  64 prescaler for 19.2 KHz
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1);                     

// enable auto trigger
  ADCSRA |= (1 << ADATE); 
// enable interrupts when measurement complete
  ADCSRA |= (1 << ADIE);  
// enable ADC
  ADCSRA |= (1 << ADEN);  
// start ADC measurements
  ADCSRA |= (1 << ADSC);  
}

bool sendSTART() {
  RF24NetworkHeader header(destinationID, 'S');

  unsigned long start_a = millis();
  unsigned long end_a;
  bool sent = false;

/* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */
  do {
    radio.flush_tx();
    sent = network.write(header, "1", sizeof("1"));

  } while (!sent && (end_a - start_a) < 300);


  return sent;
}

bool sendSTOP() {
  RF24NetworkHeader header(destinationID, 's');

  unsigned long start_a = millis();
  unsigned long end_a;
  bool sent = false;

/* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */
  do {
    radio.flush_tx();
    sent = network.write(header, "1", sizeof("1"));

  } while (!sent && (end_a - start_a) < 300);


  return sent;
}

bool sendData() {
  RF24NetworkHeader header(destinationID, 'D');

/* Create the payload with the collected readings */
  payload.hive = IDCOLMEIA;
  payload.temperature = temperatureReading;
  payload.humidity = humidityReading;
  payload.voltage_h = voltageReading;
  payload.voltage_r = 0;
  payload.weight = weightReading;

  ++count;
  if (count == 13) {
    count = 1;
  }

  Serial.begin(57600);
  unsigned long start_a = millis();
  unsigned long end_a;
  bool sent = false;

/* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */
  do {
    radio.flush_tx();
    sent = network.write(header, &payload, sizeof(payload));

  } while (!sent && (end_a - start_a) < 300);

  if (!sent) {
    Serial.println("Pacote audio não enviado: ");
  }


  Serial.println(count);
  Serial.println(sizeof(payload));
  Serial.flush();
  Serial.end();

  return sent;
}

bool sendAudio() {
  RF24NetworkHeader header(destinationID, 'A');

/* Create the payload with the collected readings */
  audioPayload.hive = IDCOLMEIA;

  Serial.begin(57600);
/* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */

  unsigned long start_a = millis();
  unsigned long end_a;
  bool sent = false;
  int sent_c = 0;

  do {
    radio.flush_tx();
    sent = network.write(header, &audioPayload, sizeof(audioPayload));
    end_a = millis();
    ++sent_c;

  } while (!sent && (end_a - start_a) < 300);

  if (!sent) {
    Serial.println("Pacote audio não enviado: ");
  }
  Serial.println(sent_c);
  Serial.flush();
  Serial.end();

  return sent;
}

ISR(ADC_vect){
  bufferADC_L = ADCL;
  bufferADC_H = ADCH;

  interrupted = true;
}

void setup(void) {

  /* SRAM configuration*/
  if (memory.SRAMBytes == 0) {
    payload.erro_vec |= (1 << E_SRAM);
  }

  memory.clearMemory();

  /* nRF24L01 configuration*/
  SPI.begin();
  radio.begin();
  // Create a interruption mask to only generate interruptions when receive payloads
  radio.maskIRQ(1, 1,0);
  radio.setPayloadSize(32);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);
  network.begin(/*channel*/ 120, /*node address*/ sourceID);

  /* Sensors pins configuration. Sets the activation pins as OUTPUT and write LOW  */
  pinMode(PORTADHT, OUTPUT);
  digitalWrite(PORTADHT, HIGH);

  /* HX711 configuration*/
  scale.set_scale(SCALE_FACTOR);
  scale.set_offset(offset);

  delay(2000);
  digitalWrite(PORTADHT, LOW);

  /* ADC configuration*/
  isFreeRunning = true;
  analogRead_freeRunnig(3);

}

void loop() {

  if (interrupted) {
    bufferADC = (bufferADC_H << 8) | bufferADC_L;
    memory.put(strAddr, bufferADC);
    strAddr += 2;
    interrupted = false;
  } else if (!isFreeRunning && !sleep) {

    network.update();

    if (radio.rxFifoFull()) {
      radio.flush_rx();
    } else if (radio.txFifoFull()) {
      radio.flush_tx();
    }

  } else if (strAddr >= MAX_ADDR) {
/* Clears ADC previous configuration so we can use analogRead */
    ADCSRA = 0;
    ADCSRB = 0;

    isFreeRunning = false;

    ADCSRA = adcsraBackup;
    ADCSRB = adcsrbBackup;

    strAddr = 0;

/* Turn on the sensors */
    digitalWrite(PORTADHT, HIGH);
    delay(200);

/* Performs the readings */
    payload.erro_vec = '\0';

    readVoltage();
    readDHT();
    readWeight();

/* Turn off the sensors */
    digitalWrite(PORTADHT, LOW);

    sendSTART();
    delay(500);

    sendData();

    Serial.begin(57600);
    Serial.println("DONE");
    Serial.flush();
    Serial.end();

    delay(1000);
    bufferADC = 0;

    start = millis();
    uint8_t i = 0;

    for (uint32_t j = 0; j < MAX_ADDR; j = j + 2) {

      memory.get(j, bufferADC);
      audioPayload.audio[i] = bufferADC;

      ++i;
      if (i == audio_size) {
        if (!sendAudio()) {
//break;
        }
        i = 0;
      }

    }
    end = millis();
    Serial.begin(57600);
    Serial.print(end - start);
    Serial.println(" milisegundos");
    Serial.flush();
    Serial.end();
    sleep = true;

    delay(500);
    sendSTOP();


  } else if (sleep) {
    Serial.begin(57600);
    Serial.println(F("Sleeping..."));
    Serial.flush();
    Serial.end();

    radio.powerDown();

    for (int i = 0; i < TEMPO_ENTRE_CADA_LEITURA; ++i) {
      LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
    }

    radio.powerUp();
    Serial.begin(57600);
    Serial.println(F("Waking..."));
    Serial.flush();
    Serial.end();

    sleep = false;
    isFreeRunning = true;
    analogRead_freeRunnig(3);
  }
}