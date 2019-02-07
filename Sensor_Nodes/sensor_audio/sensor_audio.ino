#define SerialMon Serial                                                                          // Serial communication with the computer
#define DEBUG                                                                                     // Comment this if you don't need to debug the arduino commands         
#define TEMPO_ENTRE_CADA_LEITURA 300                              // Time between each reading in seconds  

#include <Wire.h>           // Biblioteca para manipulação do protocolo I2C
#include <RtcDS3231.h>      // Biblioteca para manipulação do DS3231

#include <LowPower.h>

#include <MicrochipSRAM.h> 

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

//INITIAL CONFIGURATION OF RTC 
RtcDS3231<TwoWire> Rtc(Wire);                                                                                       // Criação do objeto do tipo DS3231
#define countof(a) (sizeof(a) / sizeof(a[0]))

//INITIAL CONFIGURATION OF 23LC1024 - SRAM
#define SRAM_SS_PIN 6

static MicrochipSRAM memory(SRAM_SS_PIN);

char fileNameBuffer[22]; 

/* Audio Variables */
uint32_t strAddr = 0;
uint16_t bufferADC;
volatile uint8_t bufferADC_H;
volatile uint8_t bufferADC_L;

#define audio_size 100
#define sampleFragments 380
uint16_t audioSamples[audio_size];

volatile bool interrupted = false;                           // Variable to know if a interruption ocurred or not

uint16_t ADCSRA_BKP = 0;
uint16_t ADCSRB_BKP = 0;

/* Timestamp variables */
RtcDateTime now_timestamp;
char audio_timestamp[20];

uint8_t dia, mes, hora, min, seg;
uint16_t ano;

/* Control Variables */
bool goSleep = false;

// PROTOTYPES OF FUNCTIONS - Force them to be after declarations
RtcDateTime getTimestamp();


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

ISR(ADC_vect){
	bufferADC_L = ADCL;
	bufferADC_H = ADCH;  // read 8 bit value from ADC

	interrupted = true;
}

void setup() { 

#ifdef DEBUG
	SerialMon.begin(57600);
	delay(10);

  /* SRAM configuration*/
	if(memory.SRAMBytes==0){
		SerialMon.println("ERRO SRAM");
	}
	
	memory.clearMemory();

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
  	
	SerialMon.flush();
	SerialMon.end();
	
#else
	SD.begin(SD_CHIP_SELECT_PIN);
#endif

  /* ADC configuration*/ 
	analogRead_freeRunnig(3);

}

void loop() {       
	if(interrupted){

		bufferADC = (bufferADC_H << 8)|bufferADC_L;
		memory.put(strAddr, bufferADC);
		strAddr += 2;
		interrupted = false;

	}else if(strAddr >= 76000){
		ADCSRA = 0;             
		ADCSRB = 0;

		ADCSRA = ADCSRA_BKP;
		ADCSRB = ADCSRB_BKP;

		RtcDateTime now = getTimestamp();

		uint16_t i = 0;
    uint16_t countFragments = 0;
    SerialMon.begin(57600);
    SerialMon.println("Saving samples to SD");
    		
		for(uint32_t j = 0; j < 76000; j = j + 2){
			memory.get(j, bufferADC);

      audioSamples[i] = bufferADC;
			++i;
      
      if(i == audio_size){
        ++countFragments;
        
        if( countFragments == sampleFragments){
          saveSamples(true, now);   
        }else{
          saveSamples(false, now);
        }
				        
        i = 0;      
			}
			

		}
		goSleep = true;
    strAddr = 0;
    SerialMon.println("Done saving!");
    Serial.flush();
    Serial.end();
	}else if(goSleep){
		for(int i = 0; i < TEMPO_ENTRE_CADA_LEITURA; ++i){
		  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);          // Function to put the arduino in sleep mode
	  }

  	goSleep = false;
    
  	analogRead_freeRunnig(3);
  }

}

String audioToString(){
	String strResult = "";

	for(int i = 0; i < countof(audioSamples); i++){
		strResult += String(audioSamples[i]) + ",";
	}

	return strResult;
}

void saveSamples(bool isLast, RtcDateTime timestamp){

	snprintf_P(fileNameBuffer,
		countof(fileNameBuffer),
		PSTR("%02u_%02u_%02u_A.txt"),
		timestamp.Day(),
		timestamp.Month(),
		timestamp.Year());

  /* Writes the payload data into the SD backup file */
	String audioStr = audioToString();

	if (!file.open(fileNameBuffer, FILE_WRITE)){
		return;
	} 

	if(isLast){

		snprintf_P(audio_timestamp,
			countof(audio_timestamp),
			PSTR("%04u%02u%02u%02u%02u%02u"),
			timestamp.Year(),
			timestamp.Month(),
			timestamp.Day(),
			timestamp.Hour(),
			timestamp.Minute(),
			timestamp.Second());

		audioStr += String(audio_timestamp) + "\n";

		file.print(audioStr);
	}else{
		file.print(audioStr);
	}

	file.close();  
}

RtcDateTime getTimestamp(){
  /* Gets the timestamp from RTC */
	RtcDateTime timestamp;

	if (!Rtc.IsDateTimeValid()) {
		timestamp = RtcDateTime(2000, 01, 01, 0, 0, 0);              

	}else{
		timestamp = Rtc.GetDateTime();

	}

	return timestamp;
}
