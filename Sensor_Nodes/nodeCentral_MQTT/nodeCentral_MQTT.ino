#include <GSM_MQTT.h>

#include <SoftwareSerial.h>
#include <LowPower.h>

#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>

#define PORTADTR 7
#define DEBUG

//INITIAL CONFIGURATION OF MQTT / THINGSPEAK
char MQTT_HOST[] = "mqtt.thingspeak.com";
char MQTT_PORT[] = "1883";

char MQTT_USER[] = "esteves";  // Can be any name.
char MQTT_PASS[] = "FWZ6OTXMYGXC5GKW";        // Change this your MQTT API Key from Account > MyProfile.
char WRITE_API_KEY[] = "X1H7B6RD67MHVGIZ";    // Change to your channel Write API Key.
long channelID = 419812;

char APN[] = "timbrasil.br";
char USER[] = "tim";
char PASS[] = "tim";

//INITIAL CONFIGURATION OF SIM800L
SoftwareSerial SIM800L(4, 5);                                   // Serial Port configuration (RX, TX)

//INITIAL CONFIGURATION OF NRF
//const int pinCE = 8;                                            // This pin is used to set the nRF24 to standby (0) or active mode (1)
//const int pinCSN = 9;                                           // This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out

RF24 radio(8, 9);                                      // Declare object from nRF24 library (Create your wireless SPI)
RF24Network network(radio);                                     // Network uses that radio

const uint16_t id_origem = 00;                                  // Address of this node

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

void GSM_MQTT::AutoConnect(void)
{
  /*
     Use this function if you want to use autoconnect(and auto reconnect) facility
     This function is called whenever TCP connection is established (or re-established).
     put your connect codes here.
  */
  connect("Node", 1, 1, MQTT_USER , MQTT_PASS, 1, 0, 0, 0, "", "");
  /*    void connect(char *ClientIdentifier, char UserNameFlag, char PasswordFlag, char *UserName, char *Password, char CleanSession, char WillFlag, char WillQoS, char WillRetain, char *WillTopic, char *WillMessage);
          ClientIdentifier  :Is a string that uniquely identifies the client to the server.
                            :It must be unique across all clients connecting to a single server.(So it will be better for you to change that).
                            :It's length must be greater than 0 and less than 24
                            :Example "qwerty"
          UserNameFlag      :Indicates whether UserName is present
                            :Possible values (0,1)
                            :Default value 0 (Disabled)
          PasswordFlag      :Valid only when  UserNameFlag is 1, otherwise its value is disregarded.
                            :Indicates whether Password is present
                            :Possible values (0,1)
                            :Default value 0 (Disabled)
          UserName          :Mandatory when UserNameFlag is 1, otherwise its value is disregarded.
                            :The UserName corresponding to the user who is connecting, which can be used for authentication.
          Password          :alid only when  UserNameFlag and PasswordFlag are 1 , otherwise its value is disregarded.
                            :The password corresponding to the user who is connecting, which can be used for authentication.
          CleanSession      :If not set (0), then the server must store the subscriptions of the client after it disconnects.
                            :If set (1), then the server must discard any previously maintained information about the client and treat the connection as "clean".
                            :Possible values (0,1)
                            :Default value 1
          WillFlag          :This flag determines whether a WillMessage published on behalf of the client when client is disconnected involuntarily.
                            :If the WillFlag is set, the WillQoS, WillRetain, WillTopic, WilMessage fields are valid.
                            :Possible values (0,1)
                            :Default value 0 (Disables will Message)
          WillQoS           :Valid only when  WillFlag is 1, otherwise its value is disregarded.
                            :Determines the QoS level of WillMessage
                            :Possible values (0,1,2)
                            :Default value 0 (QoS 0)
          WillRetain        :Valid only when  WillFlag is 1, otherwise its value is disregarded.
                            :Determines whether the server should retain the Will message.
                            :Possible values (0,1)
                            :Default value 0
          WillTopic         :Mandatory when  WillFlag is 1, otherwise its value is disregarded.
                            :The Will Message will published to this topic (WillTopic) in case of involuntary client disconnection.
          WillMessage       :Mandatory when  WillFlag is 1, otherwise its value is disregarded.
                            :This message (WillMessage) will published to WillTopic in case of involuntary client disconnection.
  */
  
}
void GSM_MQTT::OnConnect(void){
  /*
     This function is called when mqqt connection is established.
     put your subscription/publish codes here.
  
  subscribe(0, _generateMessageID(), "SampleTopic", 1);
      void subscribe(char DUP, unsigned int MessageID, char *SubTopic, char SubQoS);
          DUP       :This flag is set when the client or server attempts to re-deliver a SUBSCRIBE message
                    :This applies to messages where the value of QoS is greater than zero (0)
                    :Possible values (0,1)
                    :Default value 0
          Message ID:The Message Identifier (Message ID) field
                    :Used only in messages where the QoS levels greater than 0 (SUBSCRIBE message is at QoS =1)
          SubTopic  :Topic names to which  subscription is needed
          SubQoS    :QoS level at which the client wants to receive messages
                    :Possible values (0,1,2)
                    :Default value 0
  */

  String publishingMsg = "field1=" + String(payload.colmeia) + "&field2=" + String(payload.temperatura) + "&field3=" + String(payload.umidade) + "&field4=" + String(payload.tensao_c) + "&field5=" + String(payload.tensao_r);
  String topicString = "channels/" + String( channelID ) + "/publish/"+String(WRITE_API_KEY);

  int length = publishingMsg.length();
  char msgBuffer[length];
  publishingMsg.toCharArray(msgBuffer,length+1);

  length = topicString.length();
  char topicBuffer[length];
  topicString.toCharArray(topicBuffer,length+1);
  
  publish(0, 0, 0, _generateMessageID(), topicBuffer, msgBuffer);

  /*  void publish(char DUP, char Qos, char RETAIN, unsigned int MessageID, char *Topic, char *Message);
      DUP       :This flag is set when the client or server attempts to re-deliver a PUBLISH message
                :This applies to messages where the value of QoS is greater than zero (0)
                :Possible values (0,1)
                :Default value 0
      QoS       :Quality of Service
                :This flag indicates the level of assurance for delivery of a PUBLISH message
                :Possible values (0,1,2)
                :Default value 0
      RETAIN    :if the Retain flag is set (1), the server should hold on to the message after it has been delivered to the current subscribers.
                :When a new subscription is established on a topic, the last retained message on that topic is sent to the subscriber
                :Possible values (0,1)
                :Default value 0
      Message ID:The Message Identifier (Message ID) field
                :Used only in messages where the QoS levels greater than 0
      Topic     :Publishing topic
      Message   :Publishing Message
  */
}
void GSM_MQTT::OnMessage(char *Topic, int TopicLength, char *Message, int MessageLength)
{
  /*
    This function is called whenever a message received from subscribed topics
    put your subscription publish codes here.
  */

  /*
     Topic        :Name of the topic from which message is coming
     TopicLength  :Number of characters in topic name
     Message      :The containing array
     MessageLength:Number of characters in message
  */
  Serial.println(TopicLength);
  Serial.println(Topic);
  Serial.println(MessageLength);
  Serial.println(Message);

}


GSM_MQTT MQTT(20);
/*
   20 is the keepalive duration in seconds
*/

void setup() {  
  /* MQTT/SIM800L configuration*/
  MQTT.begin();
  
  /* nRF24L01 configuration*/
  Serial.println(F("Initializing nRF24L01..."));
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

void loop() {
  network.update();                                            // Check the network regularly

  Serial.begin(57600);

  if (radio.rxFifoFull()) {                                     // If the RX FIFO is full, the RX FIFO is cleared
    radio.flush_rx();
  } else if (radio.txFifoFull()) {                              // If the TX FIFO is full, the TX FIFO is cleared
    radio.flush_tx();
  }

  Serial.println(F("Shutting Arduino down"));
  Serial.end();

  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);                  // Function to put the arduino in sleep mode
  
  attachInterrupt(0, receberDados, FALLING);

  Serial.begin(57600);
  Serial.println(F("Arduino woke up"));
  
  if (dataReceived) {
    //Envia dados pelo GSM
    dataReceived = false;
    MQTT.processing();
  }
  
  Serial.flush();
  Serial.end();

}

void receberDados() {
  RF24NetworkHeader header;
  Serial.begin(57600);

  while (!network.available()) {                              // Keeps busy-waiting until the transmission of the payload is completed
    network.update();
  }

  while (network.available()) {                               // Reads the payload received
    network.read(header, &payload, sizeof(payload));

#ifdef DEBUG
    Serial.print(F("Received data from sensor: "));
    Serial.println(payload.colmeia);

    Serial.println(F("The data: "));
    //Serial.print("Colmeia: ");
    Serial.print(payload.colmeia);
    Serial.print(F(" "));
    //Serial.print("Temperatura: ");
    Serial.print(payload.temperatura);
    Serial.print(F(" "));
    //Serial.print("Umidade: ");
    Serial.print(payload.umidade);
    Serial.print(F(" "));
    //Serial.print("Tensao sensor: ");
    Serial.print(payload.tensao_c);
    Serial.print(F(" "));
    //Serial.print("Tensao repetidor: ");
    Serial.println(payload.tensao_r);
#endif

  Serial.flush();
  Serial.end();

    dataReceived = true;
  }

}
