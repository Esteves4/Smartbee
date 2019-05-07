## Getting Started

These instructions will get you a copy of the project up and running on your arduino for development and testing purposes.

### Prerequisites

```
The Arduino IDE
Thingspeak account

Microcontroller: ........... Arduino nano
Sensor: .................... DHT22
RF transceiver/receiver: ... nRF24L01
GSM Module: ................ SIM800L
```

### Libraries

You can find them [here](Bibliotecas).
```
- RF24Network
- RF24
- DHT
- JeeLib
- SPI
```

### Installing

The steps to get the project up and running:

#### Step 1: Install the libraries 

   1. Download the .zip libraries. [Here](Bibliotecas)
   2. Follow this tutorial from Arduino website and go to section "Importing a .zip Library". [Here](https://www.arduino.cc/en/Guide/Libraries)
   3. ```P.S.: The SPI library is already installed with the arduino IDE```

#### Step 2: Install the hardware on arduino

   1. Connecting DHT22 on sensor node
   2. Connecting nRF24L01 on sensor node/gateway
   3. Connecting GSM module on gateway

## Authors

* **Lucas Esteves Rocha** - [Esteves4](https://github.com/Esteves4)
* **Alisson Lima Silva** - *Initial work* - [alissonlimasilva](https://github.com/alissonlimasilva)

## License

This project is licensed under the MIT License - see the [LICENSE.md](../LICENSE) file for details
