#include <LowPower.h>

#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>

#include <SPI.h>
#include <DHT.h>

#define DHTPIN A0                                               // Pin DATA of the DHT sensor.
#define DHTTYPE DHT22                                           // Sets the type of DHT utilized, DHT 22
#define IDCOLMEIA 1                                             // ID of the Hive monitored
#define TEMPO_ENTRE_CADA_LEITURA 3                             // Time between each reading in seconds  
#define SENSOR "Sensor 1"                                       // Name of the sensor
#define PORTADHT 6                                              // Activation pin of DHT
#define DEBUG

DHT dht(DHTPIN, DHTTYPE);                                       // Object of the temperature sensor

//INITIAL CONFIGURATION OF NRF
const int pinCE = 8;                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 9;                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);                                      // nRF24L01(+) radio attached using Getting Started board
RF24Network network(radio);                                     // Network uses that radio
RF24Mesh mesh(radio, network);

#define id_origem 1                                  // Address of this node
#define id_destino 0                                 // Addresses of the master node

uint16_t count = 0;
volatile bool interrupted = false;                           // Variable to know if a interruption ocurred or not

struct payload_t {                                              // Structure of our payload
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
  char  timestamp[20];
};

/* Variables that hold our readings */
float temperatura_lida = 0;
float umidade_lida = 0;
float co2_lido = 0;
float som_lido = 0;
float tensao_lida = 0;

/* Analogic ports for reading */
int SENSORSOM = A4;
//int SENSORCO2 = 0;
int SENSORTENSAO = A1;

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
  Serial.begin(57600);
  SPI.begin();
  Serial.println("Setando ID origem");
  mesh.setNodeID(id_origem); 
  Serial.println("Iniciando Mesh");
  mesh.begin();

  Serial.println("Configurando sensores");
  /* Sensors pins configuration. Sets the activation pins as OUTPUT and write LOW  */
  pinMode(PORTADHT, OUTPUT);
  digitalWrite(PORTADHT, HIGH);
  delay(2000);
  digitalWrite(PORTADHT, LOW);
  Serial.flush();
  Serial.end();

}

void loop() {
  mesh.update();                                            // Check the network regularly

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
  lerTensao();
  
  enviarDados();                                              // Sends the data to the gateway

  /* Turn off the sensors */
  digitalWrite(PORTADHT, LOW);
}

void interruptFunction() {
  interrupted = true;
  radio.flush_rx();
}

void enviarDados() {
  /* Create the payload with the collected readings */
  payload_t payload;                                
  payload.colmeia = IDCOLMEIA;
  payload.temperatura = temperatura_lida;
  payload.umidade = umidade_lida;
  payload.tensao_c = 0;
  payload.tensao_r = 0;
 
  delay(50);
  ++count;
  if(count == 13){
    count = 1;
  }
  Serial.begin(57600);
  /* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */
  if (!mesh.write(&payload,'P',sizeof(payload))) { 
    radio.flush_tx();
    
    if ( ! mesh.checkConnection() ) {
      
        Serial.println("Renewing Address");
        mesh.renewAddress();
        
      } else {
        Serial.println("Pacote não enviado, Teste OK@");
      }
     
  }else{
        
    Serial.print("Pacote enviado: ");
    Serial.println(count);
    
    Serial.flush();
    Serial.end();  
  }
}


float lerBateria(byte pin) {
  unsigned int soma = 0;
  
  for (byte i = 0; i < 10; i++) {
    soma += analogRead(pin);
  }

  return ((soma / 10) * (5.0 / 1023.0));
}
