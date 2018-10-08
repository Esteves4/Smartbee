#include <LowPower.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>
#include <DHT.h>

#include <HX711.h>                                              // You must have this library in your arduino library folder
 
#define IDCOLMEIA 1                                             // ID of the Hive monitored
#define TEMPO_ENTRE_CADA_LEITURA 3                              // Time between each reading in seconds  
#define SENSOR "Sensor 1"                                       // Name of the sensor
#define PORTADHT 6                                              // Activation pin of DHT
#define DEBUG

//INITIAL CONFIGURATION OF HX711
#define DOUT  3
#define CLK  4

HX711 scale(DOUT, CLK);

float SCALE_FACTOR = -175400.00;                                // Change this value for your calibration factor found
double offset = -60234.00;                                           // Set offset for the balance to work properly

//INITIAL CONFIGURATION OF DHT
#define DHTPIN A0                                               // Pin DATA of the DHT sensor.
#define DHTTYPE DHT22                                           // Sets the type of DHT utilized, DHT 22

DHT dht(DHTPIN, DHTTYPE);                                       // Object of the temperature sensor

//INITIAL CONFIGURATION OF NRF24L01
const int pinCE = 8;                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 9;                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);                                      // nRF24L01(+) radio attached using Getting Started board
RF24Network network(radio);                                     // Network uses that radio

const uint16_t id_origem = 01;                                  // Address of this node
const uint16_t id_destino = 00;                                 // Addresses of the master node
uint16_t count = 0;

volatile bool interrupted = false;                           // Variable to know if a interruption ocurred or not

//STRUCTURE OF OUR PAYLOAD
struct payload_t {
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
  float peso;
  char erro_vec;
  char  timestamp[20];
  
};

#define E_DHT   0
#define E_TEN_C 1
#define E_TEN_R 2
#define E_RTC   3
#define E_PESO  4
#define E_SD    5

#define payload_size sizeof(payload) - sizeof(payload.timestamp)

payload_t payload;   

/* Variables that hold our readings */
float temperatura_lida = 0;
float umidade_lida = 0;
float co2_lido = 0;
float som_lido = 0;
float tensao_lida = 0;
float peso_lido = 0;

/* Analogic ports for reading */
int SENSORSOM = A4;
//int SENSORCO2 = 0;
int SENSORTENSAO = A1;

/* Reads the temperature and the humidity from DHT sensor */
void lerDHT() {
  if (isnan(dht.readTemperature())) {
    payload.erro_vec |= (1<<E_DHT);
    temperatura_lida = 0;
  }

  else {
    temperatura_lida = dht.readTemperature();
  }

  if (isnan(dht.readHumidity())) {
    payload.erro_vec |= (1<<E_DHT);
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
  radio.setPayloadSize(payload_size);                        // Set payload Size
  radio.setPALevel(RF24_PA_LOW);                                // Set Power Amplifier level
  radio.setDataRate(RF24_250KBPS);                              // Set transmission rate
  network.begin(/*channel*/ 120, /*node address*/ id_origem);   // Start the network

  /* Sensors pins configuration. Sets the activation pins as OUTPUT and write LOW  */
  pinMode(PORTADHT, OUTPUT);
  digitalWrite(PORTADHT, HIGH);
  /* HX711 configuration*/
  scale.set_scale(SCALE_FACTOR);
  scale.set_offset(offset);
    
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
  //scale.power_up();
  delay(200);

  /* Performs the readings */
  payload.erro_vec = '\0';
  lerDHT();
  lerTensao();
  lerPeso();
  
  enviarDados();                                              // Sends the data to the gateway

  /* Turn off the sensors */
  digitalWrite(PORTADHT, LOW);
  //scale.power_down();
}

void interruptFunction() {
  interrupted = true;
  radio.flush_rx();
}

void enviarDados() {
  RF24NetworkHeader header(id_destino);                   // Sets the header of the payload   

  /* Create the payload with the collected readings */                            
  payload.colmeia = IDCOLMEIA;
  payload.temperatura = temperatura_lida;
  payload.umidade = umidade_lida;
  payload.tensao_c = 0;
  payload.tensao_r = 0;
  payload.peso = peso_lido;
 
  delay(50);
  ++count;
  if(count == 13){
    count = 1;
  }
  Serial.begin(57600);
  /* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */
  if (!network.write(header, &payload, payload_size)) { 
    radio.flush_tx();
    Serial.print("Pacote não enviado: ");
  }else{    
    Serial.print("Pacote enviado: "); 
  }

  Serial.println(count);
  Serial.println(sizeof(payload));
  Serial.flush();
  Serial.end(); 
}

void lerPeso(){
  peso_lido = scale.get_units(10);

  if(peso_lido < 0){
    if(peso_lido < -0.100){
      payload.erro_vec |= (1<<E_PESO);
    }
    peso_lido = 0;
  }

  Serial.begin(57600);
  Serial.print("Weight: ");
  Serial.print(peso_lido, 3);                                // Up to 3 decimal points
  Serial.println(" kg");                                        // Change this to kg and re-adjust the calibration factor if you follow lbs
  Serial.flush();
  Serial.end();
   
}

float lerBateria(byte pin) {
  unsigned int soma = 0;
  
  for (byte i = 0; i < 10; i++) {
    soma += analogRead(pin);
  }

  return ((soma / 10) * (5.0 / 1023.0));
}
