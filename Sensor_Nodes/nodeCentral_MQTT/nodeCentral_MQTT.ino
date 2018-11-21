#define TINY_GSM_MODEM_SIM800                                                                     // Defines the model of our gsm module
#define SerialMon Serial                                                                          // Serial communication with the computer
#define SerialAT Serial3                                                                          // Serial communication with the gsm module
#define TINY_GSM_DEBUG SerialMon                                  

#define DUMP_AT_COMMANDS                                                                          // Comment this if you don't need to debug the commands to the gsm module
#define DEBUG                                                                                     // Comment this if you don't need to debug the arduino commands         

#include <TinyGsmClient.h>
#include <PubSubClient.h>

#include <Wire.h>           // Biblioteca para manipulação do protocolo I2C
#include <RtcDS3231.h>      // Biblioteca para manipulação do DS3231

#include <LowPower.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <MicrochipSRAM.h> 

#include <nRF24L01.h>
#include <RF24.h>

#include <SdFat.h>
#include <SPI.h>

//INITIAL CONFIGURATION OF SD
const uint8_t SOFT_MISO_PIN = 10;
const uint8_t SOFT_MOSI_PIN = 11;
const uint8_t SOFT_SCK_PIN  = 12;
const uint8_t SD_CHIP_SELECT_PIN = 13;
const int8_t  DISABLE_CHIP_SELECT = -1;

// SdFat software SPI template
SdFatSoftSpi<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> SD;

// File
SdFile file;
SdFile file2;

//INITIAL CONFIGURATION OF RTC 
RtcDS3231<TwoWire> Rtc(Wire);                                                                                       // Criação do objeto do tipo DS3231
#define countof(a) (sizeof(a) / sizeof(a[0]))

//INITIAL CONFIGURATION OF 23LC1024 - SRAM
#define SRAM_SS_PIN 6

static MicrochipSRAM memory(SRAM_SS_PIN);

//INITIAL CONFIGURATION OF NRF
const int pinCE = 53;                                                                             // This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 48;                                                                            // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out
const int interruptPin = 18;                                                                      // This pin is used to wake wape the arduino when a payload is received

RF24 radio(pinCE, pinCSN);                                                                        // Declare object from nRF24 library (Create your wireless SPI)
RF24Network network(radio);                                                                       // Network uses that radio

#define id_origem 00                                                                              // Address of this node

//INITIAL CONFIGURATION OF SIM800

/* ### APN configurations ###  */

const char apn[]  = "claro.com.br";
const char user[] = "claro";
const char pass[] = "claro";

/*
const char apn[]  = "timbrasil.br";
const char user[] = "tim";
const char pass[] = "tim";
*/

const int DTR_PIN = 7;                                                                               // This pin is used to wake up the gsm module

//INITIAL CONFIGURATION OF MQTT 
const char* broker = "200.129.43.208";                                                            // Address of the mqtt broker
const char* user_MQTT = "teste@teste";                                                            // Username for the tcp connection
const char* pass_MQTT = "123456";                                                                 // Password for the tcp connection

#define TOPIC "sensors/coleta"                                                                    // MQTT topic where we'll be sending the payloads
#define TOPIC_AUDIO "sensors/audio"
#define MQTT_MAX_PACKET_SIZE 600                                                                  // Max size of the payload that PubSub library can send

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif
TinyGsmClient client(modem);
PubSubClient mqtt(client);

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
	char colmeia;
	uint16_t audio[100];
};

#define E_DHT   0
#define E_TEN_C 1
#define E_TEN_R 2
#define E_RTC   3
#define E_PESO  4
#define E_SD    5

//GLOBAL VARIABLES
const char ArraySize = 12;                                                                        // Amount of payloads the central node is going to save to send to the webservice
payload_t ArrayPayloads[ArraySize];                                                               // Array to save the payloads
char msgBuffer[MQTT_MAX_PACKET_SIZE];                                                             // Buffer to send the payload
char fileNameBuffer[22];
char fileNameAudioBuffer[22];
char bufferErro[4];

char ArrayCount = 0;                                                                              // Used to store the next payload    
bool dataReceived;                                                                                // Used to know whether a payload was received or not
bool audioReceived;

payload_t payload;
payload_a payload_audio;

#define audioPacket_size 380
uint16_t audioPacket_count = 0;
char audio_timestamp[20];
RtcDateTime now_timestamp;

uint32_t strAddr = 0;

long previousMillis = 0;
long interval = 10000; //(ms)

uint8_t dia, mes, hora, min, seg;
uint16_t ano;

// PROTOTYPES OF FUNCTIONS - Force them to be after declarations
String payloadToString(payload_t* tmp_pp);
void saveToSD(payload_t* tmp_pp, RtcDateTime timestamp);
void writeAudioSD(payload_a* tmp_pp, int seqNumber, RtcDateTime timestamp);
RtcDateTime getTimestamp();

void setup() { 
	/* SIM800L, RTC configuration */
#ifdef DEBUG
	SerialMon.begin(57600);
	delay(10);
	
	SerialMon.println(F("Initializing SIM800L and Configuring..."));
	SerialAT.begin(57600);
	delay(3000);
	
	pinMode(DTR_PIN, OUTPUT);
	digitalWrite(DTR_PIN, LOW);

	SerialMon.println(F("Shutting SIM800L down"));
	sleepGSM();

	SerialMon.println(F("Initializing RTC and Configuring..."));

	RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
	
	Rtc.Begin();

	if (!Rtc.IsDateTimeValid()) 
	{
		// Common Cuases:
		//    1) first time you ran and the device wasn't running yet
		//    2) the battery on the device is low or even missing

		Serial.println("RTC lost confidence in the DateTime!");

		// following line sets the RTC to the date & time this sketch was compiled
		// it will also reset the valid flag internally unless the Rtc device is
		// having an issue

		Rtc.SetDateTime(compiled);
	}

	if (!Rtc.GetIsRunning())
	{
		Serial.println("RTC was not actively running, starting now");
		Rtc.SetIsRunning(true);
	}

	RtcDateTime now = Rtc.GetDateTime();
	
	if (now < compiled) 
	{
		Serial.println("RTC is older than compile time!  (Updating DateTime)");
		Rtc.SetDateTime(compiled);
	}
	else if (now > compiled) 
	{
		Serial.println("RTC is newer than compile time. (this is expected)");
	}
	else if (now == compiled) 
	{
		Serial.println("RTC is the same as compile time! (not expected but all is fine)");
	}

	// never assume the Rtc was last configured by you, so
	// just clear them to your needed state
	Rtc.Enable32kHzPin(false);
	Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
	
	SerialMon.flush();
	SerialMon.end();
#else

	/* SIM800L, RTC configuration*/
	SerialAT.begin(57600);                                                                        // Starts serial communication
	delay(3000);                                                                                  // Waits to communication be established
	
	sleepGSM();                                                                                   // Puts gsm to sleeps
	
	Rtc.Begin();
	
	RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__); 
	
	if (!Rtc.IsDateTimeValid()) {
		Rtc.SetDateTime(compiled);
	}

	if (!Rtc.GetIsRunning()){
		Rtc.SetIsRunning(true);
	}

	RtcDateTime now = Rtc.GetDateTime();
	if (now < compiled) {
		Rtc.SetDateTime(compiled);
	}

	Rtc.Enable32kHzPin(false);
	Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
#endif


	/* SD configuration*/
	
#ifdef DEBUG
	SerialMon.begin(57600);
	SerialMon.println(F("Initializing SD..."));
	
	if (!SD.begin(SD_CHIP_SELECT_PIN)){
		SerialMon.println("Initialization failed!");
	}else{
		SerialMon.println("Initialization done.");
	}     
	
	/* Get ArrayCount from SD (if there is one) */
	if (SD.exists("ArrayCount.txt")) {
		SerialMon.print("\nGetting ArrayCount from SD card... ");

		if (file.open("ArrayCount.txt", FILE_READ)) {
			char count_str[4];
			
			byte i = 0;
			while (file.available()) {
				char c = file.read();
				
				if (c >= 48 && c <= 57) {
					count_str[i++] = c;
				} else {
					count_str[i] = '\0';
					break;
				}
			}
			
			sscanf(count_str, "%u", &ArrayCount);
			file.close();
			
			SerialMon.println("Done!");
		} else {
			SerialMon.println("FAIL! Couldn't open the file ArrayCount.txt");
		}
	} else {
		SerialMon.print("\nCreating ArrayCount.txt...");
		
		if (file.open("ArrayCount.txt", FILE_WRITE)) {
			file.println((int) ArrayCount);
			file.close();
			SerialMon.println("Done!");
			
		} else {
			SerialMon.println("FAIL! Couldn't open the file ArrayCount.txt");
		}
	}
	
	SerialMon.flush();
	SerialMon.end();
	
#else
	SD.begin(SD_CHIP_SELECT_PIN);
	
	/* Get ArrayCount from SD (if there is one) */
	if (file.open("ArrayCount.txt", FILE_READ)) {
		char count_str[4];
		
		byte i = 0;
		
		while (file.available()) {
			char c = file.read();
			
			if (c >= 48 && c <= 57) {
				count_str[i++] = c;
				
			} else {
				count_str[i] = '\0';
				break;
			}
		}
		
		sscanf(count_str, "%u", &ArrayCount);
		file.close();
	}
#endif
	
	/* nRF24L01 configuration*/
#ifdef DEBUG
	SerialMon.begin(57600);                                                         
	SerialMon.println(F("Initializing nRF24L01..."));
#endif
	
	SPI.begin();                                                                                    // Starts SPI protocol
	radio.begin();                                                                                  // Starts nRF24L01
	radio.maskIRQ(1, 1, 0);                                                                         // Create a interruption mask to only generate interruptions when receive payloads
	radio.setPayloadSize(32);                                                                       // Set payload Size
	radio.setPALevel(RF24_PA_LOW);                                                                  // Set Power Amplifier level
	radio.setDataRate(RF24_250KBPS);                                                                // Set transmission rate

	/* SD configuration*/
#ifdef DEBUG
	SerialMon.println(F("Initializing SD..."));

	if (!SD.begin(SD_CHIP_SELECT_PIN)){
		SerialMon.println("Initialization failed!");
	}else{
		SerialMon.println("Initialization done.");
	}
	
	SerialMon.println("LOOP: ");
	SerialMon.flush();
	SerialMon.end();
#else
	SD.begin(SD_CHIP_SELECT_PIN);
#endif

	network.begin(/*channel*/ 120, /*node address*/ id_origem);                                     // Starts the network

}

void loop() {
	network.update();                                                                               // Check the network regularly

	receiveData();

	if (dataReceived) {
		#ifdef DEBUG
		SerialMon.begin(57600);
		#endif
		
		dataReceived = false;

    /* Adds timestamp to payload received */
		RtcDateTime now = getTimestamp();
         
    snprintf_P(ArrayPayloads[ArrayCount - 1].timestamp,
    countof(ArrayPayloads[ArrayCount - 1].timestamp),
    PSTR("%04u%02u%02u%02u%02u%02u"),
    now.Year(),
    now.Month(),
    now.Day(),
    now.Hour(),
    now.Minute(),
    now.Second());

		/* Save the payload in the microSD card */
		#ifdef DEBUG
		//SerialMon.println("Writing payload into SD... ");
		#endif
		
		saveToSD(&ArrayPayloads[ArrayCount - 1], now);
		
		#ifdef DEBUG
		SerialMon.println("Done!");
		#endif
		
		/* Check if the array is full, if it is, sends all the payloads to the webservice */
		if(ArrayCount >= ArraySize){
			wakeGSM();                                                                                 // Wakes the gsm                                                            
			delay(1000);                                                                               // Waits for the gsm's startup
			
			#ifdef DEBUG
			//SerialMon.println(F("GSM woke up"));
			
			SerialMon.println(F("\nConnecting to the server..."));
			connection();
			
			int signalQlt = modem.getSignalQuality();
			SerialMon.println("GSM Signal Quality: " + String(signalQlt));
			
			SerialMon.println(F("Publishing payload..."));
			publish();
      publish_audio();
			delay(1000);

			SerialMon.println(F("\nShutting GSM down"));
			
			#else
			connection();
			publish();
      publish_audio(); 
			delay(1000);
			#endif

			/* Puts gsm to sleep again */
			sleepGSM();

			/* Starts to save the payloads again whether the payloads were sent or not */
			ArrayCount = 0;
			updateArrayCountFile(ArrayCount);
			
		}

		previousMillis = millis();

		#ifdef DEBUG
		SerialMon.flush();
		SerialMon.end();
		#endif
		
	}else if(audioReceived){
    #ifdef DEBUG
    SerialMon.begin(57600);
    #endif

    if(audioPacket_count == 0){
        now_timestamp = getTimestamp();
    }
  
		audioReceived = false;
    memory.put(strAddr, payload_audio.audio);
    strAddr += 200;
    
    if(audioPacket_count == audioPacket_size-1){
       audioPacket_count = 1;
       strAddr = 0;

       for(uint32_t i = 0; i < 76000; i += 200){
          memory.get(i, payload_audio.audio);
          writeAudioSD(&payload_audio, audioPacket_count, now_timestamp);
          ++audioPacket_count;
       }

       audioPacket_count = 0;
       
    }else{
      ++audioPacket_count;
    }

    #ifdef DEBUG
    SerialMon.flush();
    SerialMon.end();
    #endif
    
	}

}

RtcDateTime getTimestamp(){
  /* Gets the timestamp from RTC */
    RtcDateTime timestamp;
    
    if (!Rtc.IsDateTimeValid()) {
       //tmp_pp->erro_vec |= (1<<E_RTC);
      //tmp_pp->erro_vec |= (1<<E_RTC);

      wakeGSM();                                                                                 // Wakes the gsm                                                            
      delay(1000);

      if (modem.testAT(5000L)) {
        String gsmTime = modem.getGSMDateTime(DATE_FULL);
        
        gsmTime.remove(2,1);
        gsmTime.remove(4,1);
        gsmTime.remove(6,1);
        gsmTime.remove(8,1);
        gsmTime.remove(10,1);
        gsmTime.remove(12);
        gsmTime = "20" + gsmTime;
              
        ano = gsmTime.substring(0,4).toInt();
        mes = gsmTime.substring(4,6).toInt();
        dia = gsmTime.substring(6,8).toInt();
        hora = gsmTime.substring(8,10).toInt();
        min = gsmTime.substring(10,12).toInt();
        seg = gsmTime.substring(12,14).toInt();

        timestamp = RtcDateTime(ano, mes, dia, hora, min, seg);
        
      }else{
        timestamp = RtcDateTime(2000, 01, 01, 0, 0, 0);              
      }
      
      sleepGSM();

    }else{
      timestamp = Rtc.GetDateTime();
    }

    return timestamp;

}

void connection(){
#ifdef DEBUG
	SerialMon.print(F("Inicializando GSM..."));
	
	if(!modem.restart()){
		SerialMon.println(F("Failed"));
		return;
	}

	SerialMon.println(F("Aguardando rede..."));
	modem.waitForNetwork();
	
	SerialMon.print(F("Conectando a "));
	SerialMon.print(apn);
	SerialMon.println("...");
	
	modem.gprsConnect(apn, user, pass);
	mqtt.setServer(broker, 1883);
	
	SerialMon.println(F("Conectando ao broker..."));
	mqtt.connect("CentralNode", user_MQTT, pass_MQTT);
	
#else
	if(!modem.restart()){
		return;
	}

	modem.waitForNetwork();
	
	modem.gprsConnect(apn, user, pass);
	mqtt.setServer(broker, 1883);
	mqtt.connect("CentralNode", user_MQTT, pass_MQTT);
#endif
}

void receiveData() {
	RF24NetworkHeader header;
  
  network.update();

	if(network.available()) {

		RF24NetworkHeader header;                            // If so, take a look at it
		network.peek(header);

		switch (header.type){                              // Dispatch the message to the correct handler.
		case 'D': handle_data(header); break;
		case 'A': handle_audio(header); break;
		
		}


	} 
}

void handle_audio(RF24NetworkHeader& header){

	network.read(header, &payload_audio, sizeof(payload_audio));                                   // Reads the payload received
	 
	#ifdef DEBUG
	//SerialMon.begin(57600);
	#endif 
	
	audioReceived = true;
	
	#ifdef DEBUG    
	//SerialMon.print(F("\nReceived data from sensor: "));
	//SerialMon.println(payload.colmeia);

	//SerialMon.println(F("The audio: "));
	//SerialMon.println(payload_audio.audio[9]);

	//SerialMon.flush();
	//SerialMon.end();
	#endif 
}

void handle_data(RF24NetworkHeader& header){

	network.read(header, &payload, sizeof(payload));                                   // Reads the payload received
 
	ArrayPayloads[ArrayCount] = payload;                                               // Saves the payload received
	
//	#ifdef DEBUG
//	SerialMon.begin(57600);
//	#endif 

  updateArrayCountFile(++ArrayCount);
	
	dataReceived = true;
	
//	#ifdef DEBUG    
//	SerialMon.print(F("\nReceived data from sensor: "));
//	SerialMon.println(payload.colmeia);
//
//	SerialMon.println(F("The data: "));
//	SerialMon.print(payload.colmeia);
//	SerialMon.print(F(" "));
//	SerialMon.print(payload.temperatura);
//	SerialMon.print(F(" "));
//	SerialMon.print(payload.umidade);
//	SerialMon.print(F(" "));
//	SerialMon.print(payload.tensao_c);
//	SerialMon.print(F(" "));
//	SerialMon.print(payload.tensao_r);
//	SerialMon.print(F(" "));
//	SerialMon.print(payload.peso);
//	SerialMon.print(F(" "));
//	SerialMon.println(payload.erro_vec - '\0');
//
//	SerialMon.flush();
//	SerialMon.end();
//
//	#endif 
}

void updateArrayCountFile(char val) {
#ifdef DEBUG
	//SerialMon.println("\nUpdating ArrayCount.txt...");
	
	if (file.open("ArrayCount.txt", O_CREAT | O_WRITE)) {
		file.println((int) val);
		file.close();
		//SerialMon.println("Done!");
	} else {
		if(ArrayCount == 0){
			ArrayPayloads[ArrayCount].erro_vec |= (1<<E_SD);
		}else{
			ArrayPayloads[ArrayCount - 1].erro_vec |= (1<<E_SD);
		}
		
		SerialMon.println("FAIL! Couldn't open the file ArrayCount.txt");
	}
#else
	if (file.open("ArrayCount.txt", O_CREAT | O_WRITE)) {
		file.println((int) val);
		file.close();
	}else{
		if(ArrayCount == 0){
			ArrayPayloads[ArrayCount].erro_vec |= (1<<E_SD);
		}else{
			ArrayPayloads[ArrayCount - 1].erro_vec |= (1<<E_SD);
		}
	}
#endif
}

void publish(){

	/* Sends to the webservice all the payloads saved */
#ifdef DEBUG
	if (!file.open("buffer.txt", FILE_READ)){
		SerialMon.print("FAIL TO OPEN FILE: ");
		SerialMon.println("buffer.txt");

		/* Sends to the webservice all the payloads saved */
		String msg = "";
		int length;
		
		for(int i = 0; i < ArraySize; i++){
			if(i > 0){
				msg += "/";
			}
			msg += payloadToString(&ArrayPayloads[i]); 
		}
		
		length = msg.length();
		msg.toCharArray(msgBuffer,length+1);
		
		mqtt.publish(TOPIC, msgBuffer);
		delay(1000);

	}else{
		//SerialMon.print("OPENED WITH SUCCESS: ");
		//SerialMon.println("buffer.txt");

		int i;
		
		for (i = 0; i < (MQTT_MAX_PACKET_SIZE + 7 + strlen(TOPIC)); i++) {
			char c;
			
			if (!file.available() || (c = file.read()) == '\n'){
				break;
			}
			
			msgBuffer[i] = c;
		}
		
		msgBuffer[i] = '\0';

		if (!file2.open("tmp_buffer.txt", FILE_WRITE)){
			SerialMon.print("FAIL TO OPEN FILE: ");
			SerialMon.println("tmp_buffer.txt");
		}else{
			//SerialMon.print("OPENED WITH SUCCESS: ");
			//SerialMon.println("tmp_buffer.txt");

			/* If the packet couldn't be sent, save in a new buffer */
			if (!mqtt.publish(TOPIC, msgBuffer)){
				file2.print(msgBuffer);
				file2.print('\n');
			}

			++i;
			/* Save the rest of the buffer.txt in the new buffer */
			while (file.available()) {
				file.read(msgBuffer, i);
				msgBuffer[i] = '\0';
				file2.print(msgBuffer);
			}
			
			/* Turns new buffer (tmp_buffer.txt) into THE buffer (buffer.txt) */
			file.close();
			file2.close();
			
			SD.remove("buffer.txt");
			
			if (!file2.open("tmp_buffer.txt", FILE_WRITE)){
				SerialMon.print("FAIL TO OPEN FILE: ");
				SerialMon.println("tmp_buffer.txt");
			}else{
				//SerialMon.print("OPENED WITH SUCCESS: ");
				//SerialMon.println("tmp_buffer.txt");

				file2.rename(SD.vwd(), "buffer.txt");
				file2.close();
				
				delay(1000);
			}
			
		}
		
	}
#else
	if(file.open("buffer.txt", FILE_READ)){
		int i;
		
		for (i = 0; i < (MQTT_MAX_PACKET_SIZE + 7 + strlen(TOPIC)); i++) {
			char c;
			
			if (!file.available() || (c = file.read()) == '\n'){
				break;
			}
			
			msgBuffer[i] = c;
		}
		
		msgBuffer[i] = '\0';
		
		if(file2.open("tmp_buffer.txt", FILE_WRITE)){
			/* If the packet couldn't be sent, save in a new buffer */
			if (!mqtt.publish(TOPIC, msgBuffer)){
				file2.print(msgBuffer);
				file2.print('\n');
			}

			++i;
			/* Save the rest of the buffer.txt in the new buffer */
			while (file.available()) {
				file.read(msgBuffer, i);
				msgBuffer[i] = '\0';
				file2.print(msgBuffer);
			}
			
			/* Turns new buffer (tmp_buffer.txt) into THE buffer (buffer.txt) */
			file.close();
			file2.close();
			
			SD.remove("buffer.txt");
			
			if (file2.open("tmp_buffer.txt", FILE_WRITE)){
				file2.rename(SD.vwd(), "buffer.txt");
				file2.close();
				
				delay(1000);
			}       
		}
	}else{
		/* Sends to the webservice all the payloads saved */
		String msg = "";
		int length;
		
		for(int i = 0; i < ArraySize; i++){
			if(i > 0){
				msg += "/";
			}
			msg += payloadToString(&ArrayPayloads[i]); 
		}
		
		length = msg.length();
		msg.toCharArray(msgBuffer,length+1);
		
		mqtt.publish(TOPIC, msgBuffer);
		delay(1000);
		
	}
#endif
	
}

void publish_audio(){

  /* Sends to the webservice all the payloads saved */
#ifdef DEBUG
  if (!file.open("buffer_a.txt", FILE_READ)){
    SerialMon.print("FAIL TO OPEN FILE: ");
    SerialMon.println("buffer.txt");
  }else{
    int i;
    
    for (i = 0; i < (MQTT_MAX_PACKET_SIZE + 7 + strlen(TOPIC_AUDIO)); i++) {
      char c;
      
      if (!file.available() || (c = file.read()) == '\n'){
        break;
      }
      
      msgBuffer[i] = c;
    }
    
    msgBuffer[i] = '\0';

    if (!file2.open("tmp_buffer_a.txt", FILE_WRITE)){
      SerialMon.print("FAIL TO OPEN FILE: ");
      SerialMon.println("tmp_buffer_a.txt");
    }else{
      /* If the packet couldn't be sent, save in a new buffer */
      if (!mqtt.publish(TOPIC_AUDIO, msgBuffer)){
        file2.print(msgBuffer);
        file2.print('\n');
      }

      ++i;
      /* Save the rest of the buffer.txt in the new buffer */
      while (file.available()) {
        file.read(msgBuffer, i);
        msgBuffer[i] = '\0';
        file2.print(msgBuffer);
      }
      
      /* Turns new buffer (tmp_buffer_a.txt) into THE buffer (buffer_a.txt) */
      file.close();
      file2.close();
      
      SD.remove("buffer_a.txt");
      
      if (!file2.open("tmp_buffer_a.txt", FILE_WRITE)){
        SerialMon.print("FAIL TO OPEN FILE: ");
        SerialMon.println("tmp_buffer_a.txt");
      }else{
        file2.rename(SD.vwd(), "buffer_a.txt");
        file2.close();
        
        delay(1000);
      }
      
    }
    
  }
#else
  if(file.open("buffer_a.txt", FILE_READ)){
    int i;
    
    for (i = 0; i < (MQTT_MAX_PACKET_SIZE + 7 + strlen(TOPIC_AUDIO)); i++) {
      char c;
      
      if (!file.available() || (c = file.read()) == '\n'){
        break;
      }
      
      msgBuffer[i] = c;
    }
    
    msgBuffer[i] = '\0';
    
    if(file2.open("tmp_buffer_a.txt", FILE_WRITE)){
      /* If the packet couldn't be sent, save in a new buffer */
      if (!mqtt.publish(TOPIC_AUDIO, msgBuffer)){
        file2.print(msgBuffer);
        file2.print('\n');
      }

      ++i;
      /* Save the rest of the buffer.txt in the new buffer */
      while (file.available()) {
        file.read(msgBuffer, i);
        msgBuffer[i] = '\0';
        file2.print(msgBuffer);
      }
      
      /* Turns new buffer (tmp_buffer.txt) into THE buffer (buffer.txt) */
      file.close();
      file2.close();
      
      SD.remove("buffer_a.txt");
      
      if (file2.open("tmp_buffer_a.txt", FILE_WRITE)){
        file2.rename(SD.vwd(), "buffer_a.txt");
        file2.close();
        
        delay(1000);
      }       
    }
  }
#endif
  
}

void sleepGSM() {
	modem.radioOff();
	digitalWrite(DTR_PIN, HIGH);                                                          // Puts DTR pin in HIGH mode so we can enter in sleep mode
	modem.sleepEnable(true);

}

void wakeGSM() {  
	digitalWrite(DTR_PIN, LOW);                                                          // Puts DTR pin in LOW mode so we can exit the sleep mode
	modem.sleepEnable(false);
}


void saveToSD(payload_t* tmp_pp, RtcDateTime timestamp) {
	byte i = 0;
	
	snprintf_P(fileNameBuffer,
	countof(fileNameBuffer),
	PSTR("%02u_%02u_%02u_D.txt"),
	timestamp.Day(),
	timestamp.Month(),
	timestamp.Year());

#ifdef DEBUG
	//SerialMon.println("Nome do arquivo criado!");
	
	/* Writes the payload data into the SD backup file */
	String msg = payloadToString(tmp_pp);
	
	//SerialMon.println("Payload Convertido!");
	
	if(!file.open(fileNameBuffer, FILE_WRITE)){
		tmp_pp->erro_vec |= (1<<E_SD);
		SerialMon.print("FAIL TO OPEN FILE: ");
		SerialMon.println(fileNameBuffer);
	}else{
		//SerialMon.print("OPENED WITH SUCCESS: ");
		SerialMon.println(fileNameBuffer);
	}
	
	file.println(msg);
	file.close();
	
	//SerialMon.println("Payload salvo no backup");
	
	/* Writes the payload data into the SD buffer file */
	if (!file.open("buffer.txt", FILE_WRITE)){
		tmp_pp->erro_vec |= (1<<E_SD);
		SerialMon.print("FAIL TO OPEN FILE: ");
		SerialMon.println("buffer.txt");
	}else{
		//SerialMon.print("OPENED WITH SUCCESS: ");
		//SerialMon.println("buffer.txt");
	}
#else
	
	/* Writes the payload data into the SD backup file */
	String msg = payloadToString(tmp_pp);
	
	if (!file.open(fileNameBuffer, FILE_WRITE)){
		tmp_pp->erro_vec |= (1<<E_SD);
	}
	
	file.println(msg);
	file.close();
	
	/* Writes the payload data into the SD buffer file */
	if (!file.open("buffer.txt", FILE_WRITE)){
		tmp_pp->erro_vec |= (1<<E_SD);
	}
	
#endif 
	
	if (ArrayCount - 1 > 0) {
		file.print("/");                                                                   // Add a slash between payloads
	}

	file.print(msg);

	if (ArrayCount == ArraySize) {
		file.print('\n');                                                                    // Add a new line after 12 payloads
	}

	file.close();

#ifdef DEBUG
	//SerialMon.println("Payload salvo no buffer");
#endif 
}

void writeAudioSD(payload_a* tmp_pp, int seqNumber, RtcDateTime timestamp) {

#ifdef DEBUG
  //SerialMon.println("Nome do arquivo criado!");
  
  /* Writes the payload data into the SD backup file */
  snprintf_P(fileNameAudioBuffer,
    countof(fileNameAudioBuffer),
    PSTR("%02u_%02u_%02u_A.txt"),
    timestamp.Day(),
    timestamp.Month(),
    timestamp.Year());

  snprintf_P(audio_timestamp,
    countof(audio_timestamp),
    PSTR("%04u%02u%02u%02u%02u%02u"),
    timestamp.Year(),
    timestamp.Month(),
    timestamp.Day(),
    timestamp.Hour(),
    timestamp.Minute(),
    timestamp.Second());

  file.open(fileNameAudioBuffer, FILE_WRITE);
  
  if(!file.isOpen()){     
    SerialMon.print("FAIL TO OPEN FILE: ");
    SerialMon.println(fileNameAudioBuffer); 
    return; 
  }
  
  String msg = String((int)tmp_pp->colmeia) + "/" + String(seqNumber) + "/" + String(audioPacket_size) + "/";

  for(int i = 0; i < 100; ++i){
    if(i != 0){
      msg += ",";
    }
    msg += String((int)tmp_pp->audio[i]);
  }

  msg += "/" + String(audio_timestamp) + "\n";
  file.print(msg);
  file.close();

  //SerialMon.println("Payload salvo no backup");
  
  /* Writes the payload data into the SD buffer file */
  file.open("buffer_a.txt", FILE_WRITE);

  if(!file.isOpen()){     
    SerialMon.print("FAIL TO OPEN FILE: ");
    SerialMon.println("buffer_a.txt"); 
    return;
  }

  file.print(msg);
  file.close();
  
#else

/* Writes the payload data into the SD backup file */
  snprintf_P(fileNameAudioBuffer,
    countof(fileNameAudioBuffer),
    PSTR("%02u_%02u_%02u_A.txt"),
    timestamp.Day(),
    timestamp.Month(),
    timestamp.Year());

  snprintf_P(audio_timestamp,
    countof(audio_timestamp),
    PSTR("%04u%02u%02u%02u%02u%02u"),
    timestamp.Year(),
    timestamp.Month(),
    timestamp.Day(),
    timestamp.Hour(),
    timestamp.Minute(),
    timestamp.Second());

  file.open(fileNameAudioBuffer, FILE_WRITE);
  
  if(!file.isOpen()){     
    return; 
  }
  
  String msg = String((int)tmp_pp->colmeia) + "/" + String(seqNumber) + "/" + String(audioPacket_size) + "/";

  for(int i = 0; i < 100; ++i){
    if(i != 0){
      msg += ",";
    }
    msg += String((int)tmp_pp->audio[i]);
  }

  msg += "/" + String(audio_timestamp) + "\n";
  
  file.print(msg);
  file.close();
  
  /* Writes the payload data into the SD buffer file */
  file.open("buffer_a.txt", FILE_WRITE);

  if(!file.isOpen()){     
    return; 
  }

  file.print(msg);
  file.close();
  
#endif 

}
 
String payloadToString(payload_t* tmp_pp) {
	int erros = tmp_pp->erro_vec - '\0';
	
	snprintf(bufferErro,
	countof(bufferErro),
	"%03d",
	erros);
	
	return String(tmp_pp->colmeia) + "," + String(tmp_pp->temperatura) + "," + String(tmp_pp->umidade) + "," + String(tmp_pp->tensao_c) + "," + String(tmp_pp->tensao_r) + "," + String(tmp_pp->peso) + "," + String(tmp_pp->timestamp) + "," + String(bufferErro);
}
