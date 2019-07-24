## Getting Started

These instructions will get you a copy of the project up and running on your Raspberry Pi for development and testing purposes.

### Prerequisites

```
Raspberry Pi + Raspbian (version: 9 stretch were used)
Sensors: ................... DHT22
RF transceiver/receiver: ... nRF24L01
```

### Libraries

You can find them [here](libraries) or download the updated version:

- [RF24Network](https://github.com/nRF24/RF24Network)
- [RF24](https://github.com/nRF24/RF24)
- [Adafruit_DHT](https://github.com/adafruit/Adafruit_Python_DHT)
- [RPi.GPIO](https://pypi.org/project/RPi.GPIO/)

### Installing

The steps to get the project up and running:

#### Step 1: Install the libraries 

   1. Download the libraries from github
   2. Follow this tutorial from Arduino website and go to section "Importing a .zip Library". [Here](https://www.arduino.cc/en/Guide/Libraries)

#### Step 2: Install the hardware on arduino

   1. Connecting DHT22 on sensor node
   2. Connecting the microfone on sensor node
   3. Connection the load sensors on HX711
   4. Connecting the HX711 on sensor node
   5. Connecting nRF24L01 on sensor node

## Authors

* **Lucas Esteves Rocha** - [Esteves4](https://github.com/Esteves4)
* **Alef Carneiro de Sousa** - [AlefCS](https://github.com/AlefCS)
* **Alisson Lima Silva** - *Initial work* - [alissonlimasilva](https://github.com/alissonlimasilva)

## License

This project is licensed under the MIT License - see the [LICENSE.md](../LICENSE) file for details
