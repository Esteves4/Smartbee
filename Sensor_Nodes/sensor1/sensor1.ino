#include <LowPower.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>
#include <DHT.h>

#include <HX711.h>                                              // You must have this library in your arduino library folder

#include <MicrochipSRAM.h>  

#define IDCOLMEIA 1                                             // ID of the Hive monitored
#define TEMPO_ENTRE_CADA_LEITURA 300                            // Time between each reading in seconds  
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
#define DHTPIN A1                                               // Pin DATA of the DHT sensor.
#define DHTTYPE DHT22                                           // Sets the type of DHT utilized, DHT 22

DHT dht(DHTPIN, DHTTYPE);                                       // Object of the temperature sensor

//INITIAL CONFIGURATION OF 23LC1024 - SRAM
#define SRAM_SS_PIN 7

static MicrochipSRAM memory(SRAM_SS_PIN);

//INITIAL CONFIGURATION OF NRF24L01
const int pinCE = 8;                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 9;                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);                                      // nRF24L01(+) radio attached using Getting Started board
RF24Network network(radio);                                     // Network uses that radio

const uint16_t id_origem = 01;                                  // Address of this node
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

/* Analogic ports for reading */
int SENSORSOM = A3;
int SENSORTENSAO = A2;
//int SENSORCO2 = A4;

/* Audio Variables */
uint32_t strAddr = 0;
uint16_t bufferADC;
volatile uint8_t bufferADC_H;
volatile uint8_t bufferADC_L;
unsigned long start;
unsigned long end;

volatile bool interrupted = false;                           // Variable to know if a interruption ocurred or not
bool sleep = false;

uint16_t ADCSRA_BKP = 0;
uint16_t ADCSRB_BKP = 0;
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
  // read the input on analog pin 0:
  int sensorValue = analogRead(A2);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  tensao_lida = sensorValue * 0.00487586;
} 

void analogRead_freeRunnig(uint8_t pin){
	if(pin < 0 || pin > 7){
		return;
	}

  ADCSRA_BKP = ADCSRA;
  ADCSRB_BKP = ADCSRB;
  
	ADCSRA = 0;             // clear ADCSRA register
	ADCSRB = 0;             // clear ADCSRB register
	ADMUX |= (pin & 0x07);    // set A0 analog input pin
	ADMUX |= (1 << REFS0);  // set reference voltage
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1);                     //  64 prescaler for 19.2 KHz

	ADCSRA |= (1 << ADATE); // enable auto trigger
	ADCSRA |= (1 << ADIE);  // enable interrupts when measurement complete
	ADCSRA |= (1 << ADEN);  // enable ADC
	ADCSRA |= (1 << ADSC);  // start ADC measurements

}

void setup(void) {

	/* SRAM configuration*/
	if(memory.SRAMBytes==0){
		payload.erro_vec |= (1<<E_SRAM);
	}

  memory.clearMemory();

	/* nRF24L01 configuration*/ 
	//SPI.begin();                                                // Start SPI protocol
	radio.begin();                                                // Start nRF24L01
	radio.maskIRQ(1, 1, 0);                                       // Create a interruption mask to only generate interruptions when receive payloads
	radio.setPayloadSize(32);                                     // Set payload Size
	radio.setPALevel(RF24_PA_HIGH);                                // Set Power Amplifier level
	radio.setDataRate(RF24_1MBPS);                              // Set transmission rate
	network.begin(/*channel*/ 120, /*node address*/ id_origem);   // Start the network

	/* Sensors pins configuration. Sets the activation pins as OUTPUT and write LOW  */
	pinMode(PORTADHT, OUTPUT);
	digitalWrite(PORTADHT, HIGH);

	/* HX711 configuration*/
	scale.set_scale(SCALE_FACTOR);
	scale.set_offset(offset);
	
	delay(2000);
	digitalWrite(PORTADHT, LOW);

	/* ADC configuration*/ 
	analogRead_freeRunnig(3);

}

ISR(ADC_vect){
	bufferADC_L = ADCL;
	bufferADC_H = ADCH;  // read 8 bit value from ADC

	interrupted = true;
}

void loop() {
	network.update();                                            // Check the network regularly
      
	if (radio.rxFifoFull()) {                                    // If the RX FIFO is full, the RX FIFO is cleared
		radio.flush_rx();
	} else if (radio.txFifoFull()) {                             // If the TX FIFO is full, the TX FIFO is cleared
		radio.flush_tx();
	}

	if(strAddr >= 76000){
		/* Clears ADC previous configuration so we can use analogRead */
		ADCSRA = 0;             
		ADCSRB = 0;

    ADCSRA = ADCSRA_BKP;
    ADCSRB = ADCSRB_BKP;

		strAddr = 0;

		/* Turn on the sensors */
		digitalWrite(PORTADHT, HIGH);
		delay(200);

		/* Performs the readings */
		payload.erro_vec = '\0';

    lerTensao();
		lerDHT();
		lerPeso();

		/* Turn off the sensors */
		digitalWrite(PORTADHT, LOW);

    enviarSTART();
    delay(500);
    
    enviarDados();                                              // Sends the data to the gateway

    Serial.begin(57600);
    Serial.println("DONE");
    Serial.flush();
    Serial.end();
    
    delay(200);
    bufferADC = 0;

    start = millis();
    uint8_t i = 0;
    
		for(uint32_t j = 0; j < 76000; j = j + 2){
      
			memory.get(j, bufferADC);
      payload_audio.audio[i] = bufferADC;
      ++i;
      if(i == audio_size){
        if( !enviarAudio() ){
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
    enviarSTOP();
		
   
	}else if(interrupted){
		bufferADC = (bufferADC_H << 8)|bufferADC_L;
		memory.put(strAddr, bufferADC);
		strAddr += 2;
		interrupted = false;
	}else if(sleep){
		Serial.begin(57600);                                                         
		Serial.println(F("Sleeping..."));
		Serial.flush();
		Serial.end();
		
		radio.powerDown();                                           // Calls the function to power down the nRF24L01
		
		for(int i = 0; i < TEMPO_ENTRE_CADA_LEITURA; ++i){
			LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);          // Function to put the arduino in sleep mode
		}
		
		radio.powerUp();                                             // Calls the function to power up the nRF24L01
		Serial.begin(57600);                                                         
		Serial.println(F("Waking..."));
		Serial.flush();
		Serial.end();
    
    sleep = false;
		analogRead_freeRunnig(3);
	}

}

bool enviarSTART(){
  RF24NetworkHeader header(id_destino, 'S');                   // Sets the header of the payload   

  unsigned long start_a = millis();
  unsigned long end_a;
  bool sent = false;
  
  /* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */
  do{
    radio.flush_tx();
    sent = network.write(header, "1", sizeof("1"));

  }while(!sent && (end_a - start_a) < 300);

 
   return sent;
}


bool enviarSTOP(){
  RF24NetworkHeader header(id_destino, 's');                   // Sets the header of the payload   

  unsigned long start_a = millis();
  unsigned long end_a;
  bool sent = false;
  
  /* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */
  do{
    radio.flush_tx();
    sent = network.write(header, "1", sizeof("1"));

  }while(!sent && (end_a - start_a) < 300);

 
   return sent;
}

bool enviarDados() {
	RF24NetworkHeader header(id_destino, 'D');                   // Sets the header of the payload   

	/* Create the payload with the collected readings */                            
	payload.colmeia = IDCOLMEIA;
	payload.temperatura = temperatura_lida;
	payload.umidade = umidade_lida;
	payload.tensao_c = tensao_lida;
	payload.tensao_r = 0;
	payload.peso = peso_lido;

	++count;
	if(count == 13){
		count = 1;
	}

  Serial.begin(57600);
  unsigned long start_a = millis();
  unsigned long end_a;
  bool sent = false;
  
  /* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */
  do{
    radio.flush_tx();
    sent = network.write(header, &payload, sizeof(payload));

  }while(!sent && (end_a - start_a) < 300);
 
  if (!sent) { 
    Serial.println("Pacote audio não enviado: ");
  }
  
  
	Serial.println(count);
	Serial.println(sizeof(payload));
	Serial.flush();
	Serial.end(); 
  
  return sent;
}

bool enviarAudio() {
	RF24NetworkHeader header(id_destino, 'A');                   // Sets the header of the payload   

	/* Create the payload with the collected readings */                            
	payload_audio.colmeia = IDCOLMEIA;

	Serial.begin(57600);
	/* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */
	
	unsigned long start_a = millis();
  unsigned long end_a;
  bool sent = false;
  int sent_c = 0;

  do{
    radio.flush_tx();
    sent = network.write(header, &payload_audio, sizeof(payload_audio));
    end_a = millis();
    ++sent_c;
    
  }while(!sent && (end_a - start_a) < 300);
 
	if (!sent) { 
		Serial.println("Pacote audio não enviado: ");
	}
  Serial.println(sent_c);
	Serial.flush();
	Serial.end(); 
  
  return sent;
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
