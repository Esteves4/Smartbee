#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
 
RF24 radio(8, 9);
 
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
 
typedef enum { role_ping_out = 1, role_pong_back } role_e;
 
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};
 
role_e role = role_pong_back;
 
void setup(void)
{
 
    Serial.begin(57600);
    printf_begin();
    
    radio.begin();
    radio.setRetries(15,15);
    
    radio.openReadingPipe(1,pipes[1]);
    
    radio.startListening();
    
}
 
void loop(void)
{
      radio.printDetails();
      delay(5000);

}
