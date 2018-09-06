#include <LowPower.h>

#include <SPI.h>
#include <DHT.h>

#define DHTPIN A0                                               // Pin DATA of the DHT sensor.
#define DHTTYPE DHT22                                           // Sets the type of DHT utilized, DHT 22
#define IDCOLMEIA 1                                             // ID of the Hive monitored
#define TEMPO_ENTRE_CADA_LEITURA 10                             // Time between each reading in seconds  
#define SENSOR "Sensor 1"                                       // Name of the sensor
#define PORTADHT 6                                              // Activation pin of DHT
#define PORTATENSAO 5                                           // Activation pin of the voltage sensor
#define DEBUG

DHT dht(DHTPIN, DHTTYPE);                                       // Object of the temperature sensor

uint16_t count = 0;

struct payload_t {                                              // Structure of our payload
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
  String timestamp;
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
 
  /* Sensors pins configuration. Sets the activation pins as OUTPUT and write LOW  */
  pinMode(PORTADHT, OUTPUT);
  digitalWrite(PORTADHT, HIGH);
  delay(2000);
  digitalWrite(PORTADHT, LOW);

}

void loop() {

  for(int i = 0; i < TEMPO_ENTRE_CADA_LEITURA; ++i){
      LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);          // Function to put the arduino in sleep mode
  }

  /* Turn on the sensors */
  digitalWrite(PORTADHT, HIGH);
  delay(200);

  /* Performs the readings */
  lerDHT();

  /* Turn off the sensors */
  digitalWrite(PORTADHT, LOW);
}
