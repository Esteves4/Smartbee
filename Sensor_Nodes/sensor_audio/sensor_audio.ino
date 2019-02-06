#define SerialMon Serial                                                                          // Serial communication with the computer
#define DEBUG                                                                                     // Comment this if you don't need to debug the arduino commands         

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
SdFile file2;

//INITIAL CONFIGURATION OF RTC 
RtcDS3231<TwoWire> Rtc(Wire);                                                                                       // Criação do objeto do tipo DS3231
#define countof(a) (sizeof(a) / sizeof(a[0]))

//INITIAL CONFIGURATION OF 23LC1024 - SRAM
#define SRAM_SS_PIN 6

static MicrochipSRAM memory(SRAM_SS_PIN);

struct payload_a {
	char colmeia;
	uint16_t audio[100];
};

char fileNameBuffer[22]; 
char fileNameAudioBuffer[22];
char formatBuffer[5];

char audio_timestamp[20];
RtcDateTime now_timestamp;

uint32_t strAddr = 0;

long previousMillis = 0;
long interval = 10000; //(ms)

uint8_t dia, mes, hora, min, seg;
uint16_t ano;

// PROTOTYPES OF FUNCTIONS - Force them to be after declarations
void writeAudioSD(payload_a* tmp_pp, int seqNumber, RtcDateTime timestamp);
RtcDateTime getTimestamp();

void setup() { 
	/* SIM800L, RTC configuration */
#ifdef DEBUG
	SerialMon.begin(57600);
	delay(10);
	
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

}

void loop() {

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

 
String payloadToString(payload_t* tmp_pp) {
	int erros = tmp_pp->erro_vec - '\0';
	
	snprintf(formatBuffer,
	countof(formatBuffer),
	"%03d",
	erros);
	
	return String(tmp_pp->colmeia) + "," + String(tmp_pp->temperatura) + "," + String(tmp_pp->umidade) + "," + String(tmp_pp->tensao_c) + "," + String(tmp_pp->tensao_r) + "," + String(tmp_pp->peso) + "," + String(tmp_pp->timestamp) + "," + String(formatBuffer);
}
