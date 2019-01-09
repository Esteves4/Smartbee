#include <LowPower.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>
#include <DHT.h>

#include <MicrochipSRAM.h>  

#include "TimerOne.h"

#define IDCOLMEIA 1                                             // ID of the Hive monitored
#define TEMPO_ENTRE_CADA_LEITURA 3                              // Time between each reading in seconds  
#define SENSOR "Sensor 1"                                       // Name of the sensor
#define PORTADHT 6                                              // Activation pin of DHT
#define DEBUG

//INITIAL CONFIGURATION OF HX711
#define DOUT  3
#define CLK  4


volatile uint16_t i = 0;
uint16_t memTeste = 0;

//INITIAL CONFIGURATION OF DHT
#define DHTPIN A1                                               // Pin DATA of the DHT sensor.
#define DHTTYPE DHT11                                           // Sets the type of DHT utilized, DHT 22

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
	int colmeia;
	float temperatura;
	float umidade;
	float tensao_c;
	float tensao_r;
	float peso;
	char erro_vec;
	char timestamp[20];
};

struct payload_a {
	int colmeia;
	uint16_t audio;
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
int SENSORSOM = A0;
int SENSORTENSAO = A2;
//int SENSORCO2 = A4;

/* Audio Variables */
uint32_t strAddr = 0;
uint16_t bufferADC;
volatile uint8_t bufferADC_H;
volatile uint8_t bufferADC_L;

volatile bool interrupted = false;                           // Variable to know if a interruption ocurred or not
bool sleep = false;

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

void adc_freeRunning(){
	ADCSRA = 0;             // clear ADCSRA register
	ADCSRB = 0;             // clear ADCSRB register
	ADMUX |= (0 & 0x07);    // set A0 analog input pin
	ADMUX |= (1 << REFS0);  // set reference voltage
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1);                     //  64 prescaler for 19.2 KHz

	ADCSRA |= (1 << ADATE); // enable auto trigger
	ADCSRA |= (1 << ADIE);  // enable interrupts when measurement complete
	ADCSRA |= (1 << ADEN);  // enable ADC
	ADCSRA |= (1 << ADSC);  // start ADC measurements

}

void setup(void) {
	Serial.begin(57600);   
	/* SRAM configuration*/
	if(memory.SRAMBytes==0){
		Serial.println("Erro memoria");
		payload.erro_vec |= (1<<E_SRAM);
	}

	/* nRF24L01 configuration*/ 
	
	Serial.println(F("Initializing nRF24L01..."));
	//SPI.begin();
	radio.begin();                                                // Start nRF24L01
	radio.maskIRQ(1, 1, 0);                                       // Create a interruption mask to only generate interruptions when receive payloads
	radio.setPayloadSize(32);                        // Set payload Size
	radio.setPALevel(RF24_PA_LOW);                                // Set Power Amplifier level
	radio.setDataRate(RF24_2MBPS);                              // Set transmission rate
	network.begin(/*channel*/ 120, /*node address*/ id_origem);   // Start the network
	
	Serial.println(F("Done"));
	Serial.flush();
	Serial.end();

	/* Sensors pins configuration. Sets the activation pins as OUTPUT and write LOW  */
	pinMode(PORTADHT, OUTPUT);
	digitalWrite(PORTADHT, HIGH);

	delay(2000);
	digitalWrite(PORTADHT, LOW);

	/* ADC configuration*/ 
	adc_freeRunning();

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

	if(strAddr >= 8000){

		ADCSRA = 0;             
		ADCSRB = 0; 

		strAddr = 0;

		/* Turn on the sensors */
		digitalWrite(PORTADHT, HIGH);
		delay(200);

		/* Performs the readings */
		payload.erro_vec = '\0';
		
		lerDHT();

		lerTensao();

		/* Turn off the sensors */
		digitalWrite(PORTADHT, LOW);
		
		enviarDados();                                              // Sends the data to the gateway

		for(uint32_t j = 0; j < 8000; j = j + 2){
			memory.get(j, bufferADC);
			enviarAudio();
		}
		sleep = true;
		
	}else if(interrupted){
		//bufferADC = (bufferADC_H << 8)|bufferADC_L;
		memory.put(strAddr, ++bufferADC);
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
		adc_freeRunning();
	}

}

void enviarDados() {
	RF24NetworkHeader header(id_destino, 'D');                   // Sets the header of the payload   

	/* Create the payload with the collected readings */                            
	payload.colmeia = IDCOLMEIA;
	payload.temperatura = temperatura_lida;
	payload.umidade = umidade_lida;
	payload.tensao_c = 0;
	payload.tensao_r = 0;
	payload.peso = peso_lido;

	++count;
	if(count == 13){
		count = 1;
	}
	Serial.begin(57600);
	/* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */
	if (!network.write(header, &payload, sizeof(payload))) { 
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

void enviarAudio() {
	RF24NetworkHeader header(id_destino, 'A');                   // Sets the header of the payload   

	/* Create the payload with the collected readings */                            
	payload_audio.colmeia = IDCOLMEIA;
	payload_audio.audio = bufferADC;

	Serial.begin(57600);

	/* Sends the data collected to the gateway, if delivery fails let the user know over serial monitor */
	if (!network.write(header, &payload_audio, sizeof(payload_audio))) { 
		radio.flush_tx();
		Serial.print("Pacote audio não enviado: ");
	}else{    
		Serial.print("Pacote audio enviado: "); 
	}
	Serial.println(bufferADC); 
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
