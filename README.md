# Smartbee
The main objective of this project is to describe the behavior of bees in healthy hives and ready for pollination. Through real-time and minimally invasive monitoring, data from hives and colonies will be collected through heterogeneous sensors, connected to sensing devices with embedded radios. The data will be stored in a computational cloud and accessed remotely via the Web.

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

### Libraries(Bibliotecas)

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

1. Install the libraries 
    - Download the .zip libraries. [Here](Bibliotecas)
    - Follow this tutorial from Arduino website and go to section "Importing a .zip Library". [Here](https://www.arduino.cc/en/Guide/Libraries)
    - ```P.S.: The SPI library is already installed with the arduino IDE```

2. Install the hardware on arduino
    2.1 Connecting DHT22 on sensor node
    2.2 Connecting nRF24L01 on sensor node/gateway
    2.3 Connecting gsm module on gateway
    
## Running the tests

Explain how to run the automated tests for this system

## Authors

* **Lucas Esteves Rocha** - [Esteves4](https://github.com/Esteves4)
* **Alisson Lima Silva** - *Initial work* - [alissonlimasilva](https://github.com/alissonlimasilva)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE) file for details
