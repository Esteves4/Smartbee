## Getting Started

These instructions will get you a copy of the project up and running on your arduino for development and testing purposes.

### Prerequisites

```
The Arduino IDE

Microcontroller: ........... Arduino nano
Sensors: ................... DHT22, Microfone, Load Sensors, HX711
RF transceiver/receiver: ... nRF24L01
```

### Libraries

You can find them [here](libraries) or download the updated version:

- [RF24Network](https://github.com/nRF24/RF24Network)
- [RF24](https://github.com/nRF24/RF24)
- [DigitalIO](https://github.com/greiman/DigitalIO)
- [DHT-sensor](https://github.com/adafruit/DHT-sensor-library)
- [HX711](https://github.com/bogde/HX711)
- [Low Power](https://github.com/rocketscream/Low-Power)
- [SRAM](https://github.com/SV-Zanshin/MicrochipSRAM)

### Installing

The steps to get the project up and running:

#### Step 1: Install the libraries 

   1. Download the libraries from github
   2. Follow this tutorial from Arduino website and go to section "Importing a .zip Library". [Here](https://www.arduino.cc/en/Guide/Libraries)

#### Step 2: Install the hardware on arduino

   1. Connecting DHT22 on sensor node
   
      | DHT22 PIN  | Arduino PIN | Notes |
      | :-------------: | :-------------: | :-------------: |
      | 1. VCC | 5V |
      | 2. DADOS  | A1  |
      | 3. N.C  | - |
      | 4. GND | GND  |
      
   2. Connecting the microfone on sensor node
   
      | Microfone PIN  | Arduino PIN | Notes |
      | :-------------: | :-------------: | :-------------: |
      | 1. AUDIO | A3 |
      | 2. ENVELOPE  | -  |
      | 3. GATE  | - |
      | 4. GND | + Battery  |
      | 5. GND | - Battery  |
      
   3. Connection the load sensors on HX711
   
      | Load Sensor | HX711 PIN | Notes |
      | :-------------: | :-------------: | :-------------: |
      | 1. AUDIO | A3 |
      | 2. ENVELOPE  | -  |
      | 3. GATE  | - |
      | 4. VCC | + Battery  |
      | 5. GND | - Battery  |
      
   4. Connecting the HX711 on sensor node
   
      | HX711 PIN  | Arduino PIN | Notes |
      | :-------------: | :-------------: | :-------------: |
      | 1. GND | GND |
      | 2. DT/DOUT  | D3  |
      | 3. SCK/CLK  | D4 |
      | 4. VCC | 5V  |
   
   5. Connecting nRF24L01 on sensor node
   
      | nRF24L01 PIN  | Arduino PIN | Notes |
      | :-------------: | :-------------: | :-------------: |
      | 1. GND | GND |
      | 2. VCC  | 3.3V  |
      | 3. CE  | D8 |
      | 4. CSN | D9  |
      | 5. SCK | D5  |
      | 6. MOSI | D10  |
      | 7. MISO | D2  |
      | 8. IRQ | -  |

## Authors

* **Lucas Esteves Rocha** - [Esteves4](https://github.com/Esteves4)
* **Alef Carneiro de Sousa** - [AlefCS](https://github.com/AlefCS)
* **Alisson Lima Silva** - *Initial work* - [alissonlimasilva](https://github.com/alissonlimasilva)

## License

This project is licensed under the MIT License - see the [LICENSE.md](../LICENSE) file for details
