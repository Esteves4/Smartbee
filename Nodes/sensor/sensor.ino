// --- INCLUDES ---

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

// ----- DEFINES -----

#define DEBUG

#define E_DHT   0
#define E_NU1   1
#define E_NU2   2
#define E_NU3   3
#define E_PESO  4
#define E_NU4   5
#define E_SRAM  6

// ----- CONSTANTS -----

static const String SENSOR_NAME = "Sensor 1";

static const uint8_t HIVE_ID = 1;

// Arduino sleeps for 8 seconds, so, 38*8 = 308 ~ 5 minutes between readings.
static const uint16_t TIME_READINGS = 38;

static const uint8_t SENSORS_PWR = 6;

// * BAT Config *
static const uint8_t VOLTAGE_PIN = A2;

// * HX711 Config *
static const uint8_t DOUT = 3;
static const uint8_t CLK = 4;
static const float SCALE_FACTOR = 42990.00;
static const double offset = 149252.00;

// * DH22 Config *
static const uint8_t DHT_PIN = A1;
static const uint8_t DHT_TYPE = DHT11;

// * SRAM Config *
static const uint8_t SRAM_SS_PIN = 7;

// * AUDIO Config *
static const uint8_t AUDIO_PAYLOAD_SIZE = 50;
static const uint32_t SAMPLE_FRQ = 19200;
static const uint32_t MAX_ADDR = SAMPLE_FRQ * 4;

// * RF24 Config *
static const uint8_t PIN_CE = 8;
static const uint8_t PIN_CSN = 9;

static const uint8_t CHANNEL = 120;

static const uint16_t SOURCE_ID = 01;
static const uint16_t DEST_ID = 00;

// ----- Structs -----

struct data_t {
  char hive;
  float temperature;
  float humidity;
  float voltage_h;
  float voltage_r;
  float weight;
  char erro_vec;
};

struct audio_t {
  char hive;
  uint16_t audio[AUDIO_PAYLOAD_SIZE];
};

// ----- VARIABLES -----

data_t dataPayload;
audio_t audioPayload;

uint32_t start;
uint32_t end;

// * Audio variables *
volatile bool interrupted = false;
volatile uint8_t bufferHigh;
volatile uint8_t bufferLow;
uint16_t bufferADC;
uint16_t adcsraBackup = 0;
uint16_t adcsrbBackup = 0;

// * SRAM variable *
uint32_t memAddr = 0;

// ----- OBJECTS -----

// * HX711 Object *
HX711 scale(DOUT, CLK);

// * DHT22 Object *
DHT dht(DHT_PIN, DHT_TYPE);

// * 23LC1024 - SRAM Object *
static MicrochipSRAM memory(SRAM_SS_PIN);

// * NRF24L01 Objects *
RF24 radio(PIN_CE, PIN_CSN);

RF24Network network(radio);

// Reads the temperature from DHT sensor
float readTemperature() {
  if (isnan(dht.readTemperature())) {
    dataPayload.erro_vec |= (1 << E_DHT);
    return 0;
  } else {
    return dht.readTemperature();
  }
}

// Reads the humidity from DHT sensor
float readHumidity() {
  if (isnan(dht.readHumidity())) {
    dataPayload.erro_vec |= (1 << E_DHT);
    return 0;
  } else {
    return dht.readHumidity();
  }
}

// Reads the voltage of the battery
float readVoltage() {
  int sensorValue = 0;

  for (byte i = 0; i < 10; i++) {
    sensorValue += analogRead(VOLTAGE_PIN);
  }

  sensorValue = sensorValue / 10;

  // Convert the analog reading (which goes from 0 - 1023)
  // to a voltage (0 - 5V):
  return sensorValue * 0.00487586;
}

// Reads the weight of the hive
float readWeight() {
  scale.power_up();

  float weightReading = scale.get_units(10);

  if (weightReading < 0) {
    if (weightReading < -0.100) {
      dataPayload.erro_vec |= (1 << E_PESO);
    }
    weightReading = 0;
  }

  scale.power_down();

  return weightReading;
}

// Reads the audio samples in the free running mode of ADC
void analogRead_freeRunnig(uint8_t pin) {
  if (pin < 0 || pin > 7) {
    return;
  }
  Serial.begin(57600);

  // backup of the registers
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
  // 64 prescaler for 19.2 KHz
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1);

  // enable auto trigger
  ADCSRA |= (1 << ADATE);
  // enable interrupts when measurement complete
  ADCSRA |= (1 << ADIE);
  // enable ADC
  ADCSRA |= (1 << ADEN);
  // start ADC measurements
  ADCSRA |= (1 << ADSC);
  
  memAddr = 0;
  
  while (memAddr < MAX_ADDR) {
    if (interrupted) {
      bufferADC = (bufferHigh << 8) | bufferLow;
      memory.put(memAddr, bufferADC);
      memAddr += 2;
      interrupted = false;
    }
  }

  // Clears ADC previous configuration so we can use analogRead
  // and stop the interruptions
  ADCSRA = 0;
  ADCSRB = 0;

  // Restore the backup of the registers
  ADCSRA = adcsraBackup;
  ADCSRB = adcsrbBackup;
}

// Sends a payload to the central node, where type can be:
// - 'S' for the START flag
// - 's' for the STOP flag
// - 'D' for the data payload
// - 'A' for the audio payload
template< typename T >
bool send(T payload, uint8_t type) {
  RF24NetworkHeader header(DEST_ID, type);

  Serial.begin(57600);
  bool sent = false;
  uint8_t sent_c = 0;
  uint32_t start_a = millis();
  uint32_t end_a;

  do {
    radio.flush_tx();
    sent = network.write(header, &payload, sizeof(payload));
    end_a = millis();
    ++sent_c;
  } while (!sent && (end_a - start_a) < 300);

  if (!sent) {
    Serial.println("Pacote não enviado");
  }
  Serial.println(sent_c);
  Serial.flush();
  Serial.end();

  return sent;
}

// Performs the readings of the sensors and returns the payload
// prepared to send to the central node
data_t collectData() {
  // Turn on the sensors
  digitalWrite(SENSORS_PWR, HIGH);
  delay(200);
  data_t payload;

  payload.erro_vec = '\0';

  // Performs the readings
  payload.hive = HIVE_ID;
  payload.temperature = readTemperature();
  payload.humidity = readHumidity();
  payload.voltage_h = readVoltage();
  payload.voltage_r = 0;
  payload.weight = readWeight();

  // Turn off the sensors
  digitalWrite(SENSORS_PWR, LOW);

  return payload;
}

// Interrupt function
ISR(ADC_vect) {
  bufferLow = ADCL;
  bufferHigh = ADCH;

  interrupted = true;
}

void setup(void) {
  // SRAM configuration
  if (memory.SRAMBytes == 0) {
    dataPayload.erro_vec |= (1 << E_SRAM);
  }

  memory.clearMemory();

  // nRF24L01 configuration
  SPI.begin();
  radio.begin();
  radio.maskIRQ(1, 1, 0);
  radio.setPayloadSize(32);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);
  network.begin(CHANNEL, SOURCE_ID);

  pinMode(SENSORS_PWR, OUTPUT);
  digitalWrite(SENSORS_PWR, LOW);

  // HX711 configuration
  scale.set_scale(SCALE_FACTOR);
  scale.set_offset(offset);


}

void loop() {

  // Updates the nRF24 network and clears the RX/TX's queue
  network.update();

  if (radio.rxFifoFull()) {
    radio.flush_rx();
  } else if (radio.txFifoFull()) {
    radio.flush_tx();
  }

  // Start sampling audio
  analogRead_freeRunnig(3);
  
  // Collect the data from sensors
  dataPayload = collectData();

  Serial.begin(57600);
  Serial.println("START");
  Serial.end();
  
  // Send START flag
  send('1', 'S');

  delay(500);

  // Send the collected data from sensors
  send(dataPayload, 'D');

  delay(1000);

  bufferADC = 0;
  uint8_t i = 0;
  bool sent = false;
  start = millis();

  // Send the collected audio samples
  for (uint32_t j = 0; j < MAX_ADDR; j = j + 2) {
    memory.get(j, bufferADC);
    audioPayload.audio[i] = bufferADC;

    ++i;
    if (i == AUDIO_PAYLOAD_SIZE) {
      sent = send(audioPayload, 'A');
      if (!sent) {
        // break;
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

  delay(500);

  // Send STOP flag
  send('1', 's');
  Serial.begin(57600);
  Serial.println("STOP");
  Serial.end();

  // Start process of powering down the arduino 
  // to reduce the power consumption
  Serial.begin(57600);
  Serial.println(F("Sleeping..."));
  Serial.flush();
  Serial.end();

  radio.powerDown();

  for (int i = 0; i < TIME_READINGS; ++i) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }

  radio.powerUp();
  Serial.begin(57600);
  Serial.println(F("Waking..."));
  Serial.flush();
  Serial.end();
}
