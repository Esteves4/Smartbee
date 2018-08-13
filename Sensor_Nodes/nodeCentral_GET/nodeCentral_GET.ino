#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialAT Serial1

#include <StreamDebugger.h>
#include <TinyGsmClient.h>
#include <SoftwareSerial.h>

#include <LowPower.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>

#define TEMPOENTRECADALEITURA 60000                             // Time between each reading in milliseconds 
#define PORTADTR 7
#define DEBUG


//INITIAL CONFIGURATION OF SIM800
const char apn[]  = "claro.com.br";
const char user[] = "claro";
const char pass[] = "claro";
SoftwareSerial SerialAT(4, 5);                                   // Serial Port configuration -(RX, TX) pins of SerialAT
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
TinyGsmClient client(modem);

//INITIAL CONFIGURATION OF NRF
const int pinCE = 8;                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 9;                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);                                      // Declare object from nRF24 library (Create your wireless SPI)
RF24Network network(radio);                                     // Network uses that radio

const uint16_t id_origem = 00;                                  // Address of this node
const uint16_t ids_destino[3] = {01, 02, 03};                   // Addresses of the others nodes

//STRUCTURE OF OUR PAYLOAD
struct payload_t {
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
};

//GLOBAL VARIABLES
payload_t payload;                                              // Used to store the payload from the sensor node
bool dataReceived;                                              // Used to know whether a payload was received or not

void setup() {
  SerialMon.begin(57600);                                           // Start Serial communication
  delay(10);


  /* SerialAT (GSM) configuration */
  SerialMon.println("Initializing modem...");
  SerialAT.begin(57600);
  delay(3000);                                                 // Delays of three seconds
  sendAT();                                                     // Send AT command to check if the GSM is responding

  pinMode(PORTADTR, OUTPUT);                                    // Configure DTR pin to control sleep mode of the GSM

#ifdef DEBUG
  SerialAT.println("AT+CMEE=2");                                 // Send AT command to activate the verbose format of error messages
  delay(2000);
  gsmAnswer();
#endif

  /* nRF24L01 configuration*/
  SerialMon.println("Initializing nRF24L01...");
  SPI.begin();                                                  // Start SPI protocol
  radio.begin();                                                // Start nRF24L01
  radio.maskIRQ(1, 1, 0);                                       // Create a interruption mask to only generate interruptions when receive payloads
  radio.setPayloadSize(32);                                     // Set payload Size
  radio.setPALevel(RF24_PA_LOW);                                // Set Power Amplifier level
  radio.setDataRate(RF24_250KBPS);                              // Set transmission rate
  attachInterrupt(0, receberDados, FALLING);                    // Attach the pin where the interruption is going to happen
  network.begin(/*channel*/ 120, /*node address*/ id_origem);   // Start the network
}

void sendAT() {
  SerialAT.println("AT");
  delay (2000);
  gsmAnswer();
}

void loop() {
  network.update();                                            // Check the network regularly

  SerialAT.println("ATZ");                                       // Sends AT command to reset the GSM
  delay (9000);
  gsmAnswer();

  if (radio.rxFifoFull()) {                                     // If the RX FIFO is full, the RX FIFO is cleared
    radio.flush_rx();
  } else if (radio.txFifoFull()) {                              // If the TX FIFO is full, the TX FIFO is cleared
    radio.flush_tx();
  }
 
  SerialMon.println("\nShuting GSM down...");
  sleepGSM();                                                  // Calls the function to put GSM in sleep mode

  SerialMon.println("Shutting Arduino down");
  SerialMon.flush();

  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);                  // Function to put the arduino in sleep mode
  
  attachInterrupt(0, receberDados, FALLING);


  SerialMon.println("Arduino woke up");

  SerialMon.println("Waking modem up...");
  wakeGSM();                                                    // Calls the function to wake up the GSM

                                     
  if (dataReceived) {                                        // Check if a payload was received. If true, sends the data to the web service.

    //configureBearerProfile("claro.com.br", "claro", "claro");// Configure the GSM network
    configureBearerProfile("timbrasil.br", "tim", "tim");
    sendGET_Requisition();                                   // Sends a GET requisition to the web service with the data received from the sensor node
  
    dataReceived = false;
  }


}

void sleepGSM() {
  digitalWrite(PORTADTR, HIGH);                                // Puts DTR pin in HIGH mode so we can enter in sleep mode

  SerialAT.println("AT+CFUN=0");                               // Sends AT command to activate sleep mode
  delay(1000);
  gsmAnswer();

  SerialAT.println("AT+CSCLK=1");                               // Sends AT command to activate sleep mode
  delay(2000);
  gsmAnswer();

}

void wakeGSM() {
  digitalWrite(PORTADTR, LOW);                                // Puts DTR pin in LOW mode so we can exit the sleep mode
  delay(500);                                                 // Waits 500ms so the GSM starts properly

  sendAT();

  SerialAT.println("AT+CSCLK=0");                              // Sends AT command to deactivate sleep mode
  delay (2000);
  gsmAnswer();

  SerialAT.println("AT+CFUN=1");                               // Sends AT command to activate sleep mode
  delay(1000);
  gsmAnswer();


}

void receberDados() {
  RF24NetworkHeader header;


  while (!network.available()) {                              // Keeps busy-waiting until the transmission of the payload is completed
    network.update();
  }

  while (network.available()) {                               // Reads the payload received
    network.read(header, &payload, sizeof(payload));

#ifdef DEBUG
    SerialMon.print("Received data from sensor: ");
    SerialMon.println(payload.colmeia);

    SerialMon.println("The data: ");
    //Serial.print("Colmeia: ");
    SerialMon.print(payload.colmeia);
    SerialMon.print(" ");
    //Serial.print("Temperatura: ");
    SerialMon.print(payload.temperatura);
    SerialMon.print(" ");
    //Serial.print("Umidade: ");
    SerialMon.print(payload.umidade);
    SerialMon.print(" ");
    //Serial.print("Tensao sensor: ");
    SerialMon.print(payload.tensao_c);
    SerialMon.print(" ");
    //Serial.print("Tensao repetidor: ");
    SerialMon.println(payload.tensao_r);
#endif

    dataReceived = true;
  }

}

bool sendGET_Requisition() {
  SerialAT.println("AT+CSQ");
  delay (1000); //Tempo de espera
  gsmAnswer();

  SerialAT.println("AT+HTTPINIT");
  delay (2000);
  gsmAnswer();

  SerialAT.println("AT+HTTPPARA=\"CID\",1");
  delay (2000);
  gsmAnswer();

  
  SerialAT.println("AT+SAPBR=2,1");
  delay (2000);
  gsmAnswer();

  SerialAT.println("AT+SAPBR=4,1");
  delay (2000);
  gsmAnswer();
  
  SerialAT.println("AT+CGATT?");
  delay (2000);
  gsmAnswer();

  SerialAT.println("AT+HTTPPARA=\"URL\",\"api.thingspeak.com/update?api_key=X1H7B6RD67MHVGIZ&field1=" + String(payload.colmeia) + "&field2=" + String(payload.temperatura) + "&field3=" + String(payload.umidade) + "&field4=" + String(payload.tensao_c) + "&field5=" + String(payload.tensao_r) + "\"");
  delay (2000);
  gsmAnswer();

  SerialAT.println("AT+HTTPACTION=0");
  delay (10000);
  gsmAnswer();

  SerialAT.println("AT+HTTPREAD");
  delay (2000);
  gsmAnswer();

  SerialAT.println("AT+HTTPTERM");
  delay (2000);
  gsmAnswer();

  SerialAT.println("AT+SAPBR=0,1");
  delay (2000);
  gsmAnswer();
}

bool configureBearerProfile(const char* APN, const char* USER, const char* PASS) {

  SerialAT.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  delay (1000); //Tempo de espera
  gsmAnswer();

  SerialAT.println("AT+SAPBR=3,1,\"APN\",\"" + String(APN) + "\"");
  delay (3000); //Tempo de espera
  gsmAnswer();

  SerialAT.println("AT+SAPBR=3,1,\"USER\",\"" + String(USER) + "\"");
  delay (3000);
  gsmAnswer();

  SerialAT.println("AT+SAPBR=3,1,\"PWD\",\"" + String(PASS) + "\"");
  delay (3000); //Tempo de espera
  gsmAnswer();

  SerialAT.println("AT+SAPBR=1,1");
  delay (2000);
  gsmAnswer();

  SerialAT.println("AT+SAPBR=2,1");
  delay (2000);
  gsmAnswer();

}


void gsmAnswer() {
  while (SerialAT.available())
  {
    if (SerialAT.available() > 0)                      // If receives data
    {
      Serial.write(SerialAT.read());                   // Prints out the data

    }
  }
}

void cleanBuffer()
{
  delay( 250 );
  while ( SerialAT.available() > 0)
  {
    SerialAT.read();    // Clean the input buffer
    delay(50);
  }
}

bool waitFor(const char* expected_answer1)
{
  uint8_t x = 0;
  bool answer = false;
  char response[100];
  unsigned long previous;

  memset(response, (char)0, 100);    // Initialize the string

  delay( 250 );

  x = 0;
  previous = millis();

  // this loop waits for the answer
  do {
    // if there are data in the UART input buffer, reads it and checks for the asnwer
    if (SerialAT.available() > 0) {
      response[x] = SerialAT.read();
      x++;
      // check if the desired answer 1  is in the response of the module
      if (strstr(response, expected_answer1) != NULL)
      {
        answer = true;
      }
    }
    delay(10);
  } while ((answer == false) && ((millis() - previous) < 10000 )); // Waits for the asnwer with time out

  Serial.write(response);
  return answer;
}
