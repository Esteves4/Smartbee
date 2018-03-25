#include <SoftwareSerial.h>

#include <JeeLib.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>

#define TEMPOENTRECADALEITURA 10000                             // Time between each reading in milliseconds 
#define PORTADTR 7
#define DEBUG

//INITIAL CONFIGURATION OF NRF
const int pinCE = 8;                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 9;                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(pinCE, pinCSN);                                      // Declare object from nRF24 library (Create your wireless SPI)
RF24Network network(radio);                                     // Network uses that radio

const uint16_t id_origem = 00;                                  // Address of this node
const uint16_t ids_destino[3] = {01, 02, 03};                   // Addresses of the others nodes

//INITIAL CONFIGURATION OF SIM800L
SoftwareSerial SIM800L(4, 5);                                   // Configuração da Porta Serial

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

ISR(WDT_vect) {
  Sleepy::watchdogEvent();
}

void setup() {
  Serial.begin(9600);                                           // Start Serial communication
  delay(3000);

  /* SIM800L (GSM) configuration */
  Serial.println("Initializing modem...");
  SIM800L.begin(9600);                                          // Start SoftwareSerial for communication with the GSM
  delay(3000);                                                  // Delays of three seconds
  sendAT();                                                     // Send AT command to check if the GSM is responding

  pinMode(PORTADTR, OUTPUT);                                    // Configure DTR pin to control sleep mode of the GSM

#ifdef DEBUG
  SIM800L.println("AT+CMEE=2");                                 // Send AT command to activate the verbose format of error messages
  delay(2000);
  gsmAnswer();
#endif

  /* nRF24L01 configuration*/
  Serial.println("Initializing nRF24L01...");
  SPI.begin();                                                  // Start SPI protocol
  radio.begin();                                                // Start nRF24L01
  radio.maskIRQ(1, 1, 0);                                       // Create a interruption mask to only generate interruptions when receive payloads
  radio.setPayloadSize(32);                                     // Set payload Size
  radio.setPALevel(RF24_PA_LOW);                                // Set Power Amplifier level
  radio.setDataRate(RF24_250KBPS);                              // Set transmission rate
  attachInterrupt(0, receberDados, FALLING);                    // Attach the pin where the interruption is going to happen
  network.begin(/*channel*/ 120, /*node address*/ id_origem);   // Start the network

  Serial.flush();
  Serial.end();
}

void sendAT() {
  SIM800L.println("AT");
  delay (2000);
  gsmAnswer();
}

void loop() {
  network.update();                                            // Check the network regularly

  Serial.begin(9600);

  SIM800L.println("ATZ");                                      // Sends AT command to reset the GSM
  delay (9000);
  gsmAnswer();


  if (radio.rxFifoFull()) {                                    // If the RX FIFO is full, the RX FIFO is cleared
    radio.flush_rx();
  } else if (radio.txFifoFull()) {                              // If the TX FIFO is full, the TX FIFO is cleared
    radio.flush_tx();
  }

  /* Starts the process of waking up the sensors, receive the data and send to the web service */
  for (int i = 2; i < 3; ++i) {
    wakeSensors(i);                                            // Calls the function which sends a message to the sensor to wake up
    if (dataReceived) {                                        // Check if a payload was received. If true, sends the data to the web service.
      bool isOK;
      Serial.println("Configuring GSM Network");
      isOK = configureBearerProfile("claro.com.br", "claro", "claro");// Configure the GSM network
      if (isOK) {
        Serial.println("OK");
        Serial.println("Sending the GET Requisition");
        isOK = sendGET_Requisition();                                   // Sends a GET requisition to the web service with the data received from the sensor node
        if (isOK) {
          Serial.println("OK");
        } else {
          Serial.println("Error sending the GET Requisition");
        }
      } else {
        Serial.println("Error configuring GSM Network");
      }
    }
  }

  Serial.println("\nShuting GSM down...");
  sleepGSM();                                                  // Calls the function to put GSM in sleep mode

  Serial.println("Powering down the nRF24L01");
  radio.powerDown();                                           // Calls the function to power down the nRF24L01

  Serial.println("Shutting Arduino down");
  Serial.end();

  int ok;                                                      // Local variable to know if the arduino slept the time we wanted
  for (int i = 0; i < 1; ++i) {
    do {
      ok = Sleepy::loseSomeTime(TEMPOENTRECADALEITURA);          // Function to put te arduino in sleep mode
    } while (!ok);
  }

  attachInterrupt(0, receberDados, FALLING);

  Serial.begin(9600);

  Serial.println("Arduino woke up");

  Serial.println("Powering up the nRF24L01");
  radio.powerUp();                                              // Calls the function to power up the nRF24L01

  Serial.println("Waking modem up...");
  wakeGSM();                                                    // Calls the function to wake up the GSM

  Serial.flush();
  Serial.end();

}

void sleepGSM() {
  digitalWrite(PORTADTR, HIGH);                                // Puts DTR pin in HIGH mode so we can enter in sleep mode

  SIM800L.println("AT+CSCLK=1");                               // Sends AT command to activate sleep mode
  delay(2000);
  gsmAnswer();

}

void wakeGSM() {
  digitalWrite(PORTADTR, LOW);                                // Puts DTR pin in LOW mode so we can exit the sleep mode
  delay(500);                                                 // Waits 500ms so the GSM starts properly

  sendAT();

  SIM800L.println("AT+CSCLK=0");                              // Sends AT command to deactivate sleep mode
  delay (2000);
  gsmAnswer();


}

void wakeSensors(int i) {
  dataReceived = false;
  RF24NetworkHeader header(ids_destino[i]);                   // Set the header of the payload
  char msg = 'H';                                             // Set the message to wake up the sensor node

  Serial.println("Say HI");

  //If the write fails let the user know over serial monitor
  if (!network.write(header, &msg, sizeof(msg))) {
    radio.flush_tx();                                         // Clean up the TX queue
    Serial.println("Command delivery failed");
  } else {
    radio.flush_tx();
    Serial.println("Success sending command");
    delay(2000);                                              // Waits for one second to receive the payload from the sensor
  }
}

void receberDados() {
  RF24NetworkHeader header;


  while (!network.available()) {                              // Keeps busy-waiting until the transmission of the payload is completed
    network.update();
  }

  while (network.available()) {                               // Reads the payload received
    network.read(header, &payload, sizeof(payload));

#ifdef DEBUG
    Serial.print("Received data from sensor: ");
    Serial.println(payload.colmeia);

    Serial.println("The data: ");
    //Serial.print("Colmeia: ");
    Serial.print(payload.colmeia);
    Serial.print(" ");
    //Serial.print("Temperatura: ");
    Serial.print(payload.temperatura);
    Serial.print(" ");
    //Serial.print("Umidade: ");
    Serial.print(payload.umidade);
    Serial.print(" ");
    //Serial.print("Tensao sensor: ");
    Serial.print(payload.tensao_c);
    Serial.print(" ");
    //Serial.print("Tensao repetidor: ");
    Serial.println(payload.tensao_r);
#endif

    dataReceived = true;
  }

}

bool sendGET_Requisition() {
  bool isOK;
  uint8_t counter = 0;

  do {
    SIM800L.println("AT+HTTPINIT");
    delay (2000);
    isOK = waitFor("OK");
    ++counter;
    if (counter >= 3) {
      Serial.println("ERROR: AT+HTTPINIT");
      return false;
    }
  } while (!isOK);

  counter = 0;
  do {
    SIM800L.println("AT+HTTPPARA=\"CID\",1");
    delay (2000);
    isOK = waitFor("OK");
    ++counter;

    if (counter >= 3) {
      Serial.println("ERROR: AT+HTTPPARA");
      return false;
    }
  } while (!isOK);

  SIM800L.println("AT+HTTPPARA=\"URL\",\"api.thingspeak.com/update?api_key=X1H7B6RD67MHVGIZ&field1=" + String(payload.colmeia) + "&field2=" + String(payload.temperatura) + "&field3=" + String(payload.umidade) + "&field4=" + String(payload.tensao_c) + "&field5=" + String(payload.tensao_r) + "\"");
  delay (2000);
  gsmAnswer();

  counter = 0;
  do {
    SIM800L.println("AT+HTTPACTION=0");
    delay (9000);
    isOK = waitFor("OK");
    ++counter;

    if (counter >= 3) {
      Serial.println("ERROR: AT+HTTPACTION");
      return false;
    }
  } while (!isOK);

  counter = 0;
  do {
    SIM800L.println("AT+HTTPREAD");
    delay (2000);
    isOK = waitFor("OK");
    ++counter;

    if (counter >= 3) {
      Serial.println("ERROR: AT+HTTPREAD");
      return false;
    }
  } while (!isOK);

  counter = 0;
  do {
    SIM800L.println("AT+HTTPTERM");
    delay (2000);
    isOK = waitFor("OK");
    ++counter;

    if (counter >= 3) {
      Serial.println("ERROR: AT+HTTPTERM");
      break;
    }
  } while (!isOK);

  counter = 0;
  do {
    SIM800L.println("AT+SAPBR= 0,1");
    delay (2000);
    isOK = waitFor("OK");
    ++counter;

    if (counter >= 3) {
      Serial.println("ERROR: AT+SAPBR");
      break;
    }
  } while (!isOK);

  return true;
}

bool configureBearerProfile(const char* APN, const char* USER, const char* PASS) {
  bool isOK;
  uint8_t counter = 0;

  do {
    SIM800L.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
    delay (1000); //Tempo de espera
    isOK = waitFor("OK");

    ++counter;
    if (counter >= 3) {
      break;
      //return false;
    }
  } while (!isOK);

  counter = 0;
  do {
    SIM800L.println("AT+SAPBR=3,1,\"APN\",\"" + String(APN) + "\"");
    delay (3000); //Tempo de espera
    isOK = waitFor("OK");

    ++counter;
    if (counter >= 3) {
      break;
      //return false;
    }

  } while (!isOK);

  counter = 0;
  do {
    SIM800L.println("AT+SAPBR=3,1,\"USER\",\"" + String(USER) + "\"");
    isOK = waitFor("OK");

    ++counter;
    if (counter >= 3) {
      break;
      //return false;
    }
  } while (!isOK);

  counter = 0;
  do {
    SIM800L.println("AT+SAPBR=3,1,\"PWD\",\"" + String(PASS) + "\"");
    delay (3000); //Tempo de espera
    isOK = waitFor("OK");

    ++counter;
    if (counter >= 3) {
      break;
      //return false;
    }
  } while (!isOK);

  counter = 0;
  do {
    SIM800L.println("AT+SAPBR=1,1");
    delay (2000);
    isOK = waitFor("OK");

    ++counter;
    if (counter >= 3) {
      break;
      //return false;
    }
  } while (!isOK);

  counter = 0;
  do {
    SIM800L.println("AT+SAPBR=2,1");
    delay (2000);
    isOK = waitFor("OK");

    ++counter;
    if (counter >= 3) {
      break;
      //return false;
    }
  } while (!isOK);

  return true;
}


void gsmAnswer() {
  while (SIM800L.available())
  {
    if (SIM800L.available() > 0)                      // If receives data
    {
      Serial.write(SIM800L.read());                   // Prints out the data

    }
  }
}

void cleanBuffer()
{
  delay( 250 );
  while ( SIM800L.available() > 0)
  {
    SIM800L.read();    // Clean the input buffer
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
    if (SIM800L.available() > 0) {
      response[x] = SIM800L.read();
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

