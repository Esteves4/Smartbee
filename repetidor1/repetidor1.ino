#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>


RF24 radio(8,9);                // nRF24L01(+) radio attached using Getting Started board 

RF24Network network(radio);      // Network uses that radio
const uint16_t id_origem = 001;    // Address of our node in Octal format ( 04,031, etc)
const uint16_t id_destino = 00;   // Address of the other node in Octal format

struct payload_t {                  // Structure of our payload
  int colmeia;
  float temperatura;
  float umidade;
  float tensao_c;
  float tensao_r;
};

float tensao_lida = 0;
int SENSORTENSAO=1;
char *para_enviar;

void lerTensao(){
  float valor_lido_tensao = analogRead(SENSORTENSAO);
  tensao_lida=((valor_lido_tensao*0.004887586)*4.65);
}

void setup(void)
{
  Serial.begin(57600);
  Serial.println("Repetidor 1");
 
  SPI.begin();
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  network.begin(/*channel*/ 120, /*node address*/ id_origem);
}

void loop(void){
  
  network.update();                  // Check the network regularly
  
  while ( network.available() ) {     // Is there anything ready for us?

    //recebe o que foi enviado do repetidor 2
    RF24NetworkHeader header;        // If so, grab it and print it out
    payload_t recebido;
    network.read(header,&recebido,sizeof(recebido));
    //ENVIAR PARA GATEWAY
    Serial.print("Colmeia: ");
    Serial.print(recebido.colmeia);
    Serial.print(" Temperatura: ");
    Serial.print(recebido.temperatura);
    Serial.print(" Umidade:");
    Serial.print(recebido.umidade);
    Serial.print(" Tensao Sensores: ");
    Serial.print(recebido.tensao_c);
    Serial.print(" Tensao Repetidor 2: ");
    Serial.println(recebido.tensao_r);

    
    RF24NetworkHeader header_para_enviar(id_destino);
    bool ok = network.write(header_para_enviar,&recebido,sizeof(recebido));
    if(ok){
      Serial.println("Enviado gw");
    }else{
      Serial.println("Falha ao enviar");
    }   
  }
}


