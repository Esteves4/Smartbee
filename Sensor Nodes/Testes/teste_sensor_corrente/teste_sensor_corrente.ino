/*
Henry's Bench
MAX471 Tutorial
*/
const int max471In = A0;

int RawValue= 0;
float Current = 0;

void setup(){  
  pinMode(max471In, INPUT);
  Serial.begin(9600);
}

void loop(){  
  RawValue = analogRead(max471In); 
  Current = (RawValue * 5.0 )/ 1024.0; // scale the ADC  
       
  Serial.print("Current = "); // shows the voltage measured     
  Serial.print(Current,3); //3 digits after decimal point
  Serial.println(" amps DC"); //3 digits after decimal point  
  delay(1500);  
}
