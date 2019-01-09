#define SerialMon Serial
#include <SdFat.h>
#include <SPI.h>

#define SD_CS_PIN SS

SdFat SD;

// File
File file;

int count = 0;

void setup() { 
  
  /* SD configuration*/
 SerialMon.begin(57600);
 SerialMon.println(F("Initializing SD..."));
  
  if (!SD.begin(SD_CS_PIN)){
    SerialMon.println("Initialization failed!");
  }else{
    SerialMon.println("Initialization done.");
  }     

  pinMode(3, OUTPUT);
  file = SD.open("TimeSD.txt", FILE_WRITE);
  if (!file){
    SerialMon.println("Open file failed!");
  }else{
    SerialMon.println("Open file done.");
  }   
}

void loop() {

    digitalWrite(3, LOW);
    
    file.print(String(10) + '\n');
    
    digitalWrite(3, HIGH);
    
    ++count;
  
    if(count == 100){
      file.close();
      while(true);
    }

   
}
