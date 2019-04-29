#!/usr/bin/python
# coding=utf-8
"""
This script receives, saves, processes the data coming
from the sensor nodes and sends to the webservice

Attributes:
    GSM_CONFIG      (dictionary): It holds the network configuration of the gsm module
    TOPICS          (dictionary): It holds the MQTT Topics
    MQTT_CONFIG     (dictionary): It holds the MQTT network configuration
    SENSOR          (Adafruit_DHT): The type of the sensor used
    SENSOR_PIN      (int): The pin where the sensor is connected
    MAX_COUNTER     (int): How much data packets are being buffered before sending to the webservice
    MAX_AUDIO_COUNT (int): How much audio packets are being buffered before saving to a file
"""
import time
import datetime
import os
import sys
import traceback
import logging
import logging.handlers

from struct import unpack

import GsmClient as gsm
import PubSubClient as MQTT
import Adafruit_DHT
import RPi.GPIO as GPIO

from RF24 import *
from RF24Network import *

LOG_FILENAME = './log/CentralNode.log'

# SIM800L Configuration

#APN = "claro.com.br"
#USER = "claro"
#PASSWORD = "claro"

#APN = "timbrasil.br"
#USER = "tim"
#PASSWORD = "tim"

GSM_CONFIG = {"apn":"zap.vivo.com.br", "user":"vivo", "pass":"vivo", "pin_rst":11}

# MQTT Configuration

TOPICS = {"audio":"sensors/coleta_audio", "data": "sensors/coleta_data"}
MQTT_CONFIG = {"broker":"200.129.43.208", "user":"teste@teste", "pass":"123456", "topics": TOPICS}

# Sensor temperatura Externa
SENSOR = Adafruit_DHT.DHT22
SENSOR_PIN = 23

MAX_COUNTER = 12
MAX_AUDIO_COUNT = 760


def receive_data(network_obj):
    """
    This function checks if any message was received through nRF24L01

    Args:
        network_obj (RF24Network): An object of the RF24Network class

    Returns:
        tuple:
            False, False, True, False, buffer_return, when data from sensors is received
            False, False, False, True, buffer_return, when audio from sensors is received
            True, False, False, False, [0], when a start byte is received
            False, True, False, False, [0], when a stop byte is received
            False, False, False, False, [0], when nothing happens

    """
    if network_obj.available():
        header, payload = network_obj.read(201)

        if header.type == 68:
            buffer_return = list(unpack('<b5fb', bytes(payload)))
            return False, False, True, False, buffer_return

        if header.type == 65:
            buffer_return = list(unpack('<b50H', bytes(payload)))
            return False, False, False, True, buffer_return

        if header.type == 83:
            print("START")
            return True, False, False, False, [0]

        if header.type == 115:
            print("STOP")
            return False, True, False, False, [0]

    return False, False, False, False, [0]

def set_rasp_timestamp(gsm_client, network_apn, network_user, network_password):
    """
    This function gets timestamp from gsm network and updates the Raspbian timestamp

    Args:
        gsm_client         (GsmClient): An object of the GsmClient class
        network_apn        (string): The network apn
        network_user       (string): The network user
        network_password   (string): The network password

    Returns:
        bool: True if successful, False otherwise.

    """


    if not connection_gsm(gsm_client, network_apn, network_user, network_password):
        return False

    time_now = gsm_client.getGsmTime()
    time_now = time_now.split(',')

    if int(time_now[0]) != 0:
        return False
    os.system("sudo date +%Y/%m/%d -s "+time_now[1])
    os.system("sudo date +%T -s " + time_now[2] + " -u")
    os.system("sudo date")
    print(time_now)

    return True

def get_timestamp():
    """
    This function gets timestamp from the Raspbian timestamp

    Returns:
        datetime: The updated timestamp

    """
    timestamp_now = datetime.datetime.now()

    return timestamp_now

def to_string(buffer):
    """
    This function makes a string of the data received, separating them with commas.

    Args:
        buffer         (list): A list with the data received

    Returns:
        string: The string created

    """
    for (pos, value) in enumerate(buffer):
        if isinstance(value, float):
            buffer[pos] = "%3.2f" %buffer[pos]
        else:
            buffer[pos] = str(buffer[pos])

    return ",".join(buffer)

def save_data_to_sd(buffer, data_timestamp, is_last):
    """
    This function saves the data received in two txt file, the first as a backup and the second
    as a buffer to send to the webservice

    Args:
        buffer          (list): A list with the data received
        data_timestamp  (datetime): A datetime object with the updated timestamp
        is_last         (bool): A flag informing if this is the last data to be saved in
                                the same line

    Returns:
        No returns

    """

    buffer.append(data_timestamp.strftime("%Y%m%d%H%M%S"))
    msg = to_string(buffer)


    with open("data_collect/"+data_timestamp.strftime("%d_%m_%y") + ".txt", "a") as data_file:
        data_file.write(msg + '\n')

    with open("data_to_send/buffer_data.txt", "a") as data_file:

        if is_last:
            msg += '\n'
        else:
            msg += '/'

        data_file.write(msg)

def save_audio_to_sd(buffer, audio_timestamp, is_last):
    buffer.append(audio_timestamp.strftime("%Y%m%d%H%M%S"))

    msg = to_string(buffer)

    with open("audio_collect/"+audio_timestamp.strftime("%d_%m_%y") + ".txt", "a") as audio_file:
        audio_file.write(msg+'\n')

    #Selecionando 100 amostras do audio
    if len(buffer) > 100:
        msg = to_string(buffer[0:101])
    else:
        msg = to_string(buffer)

    msg = msg + "," + audio_timestamp.strftime("%Y%m%d%H%M%S")

    with open("audio_to_send/buffer_audio.txt", "a") as audio_file:
        if is_last:
            audio_file.write(msg + '\n')
        else:
            audio_file.write(msg + '/')

def connection_gsm(gsm_client, network_apn, network_user, network_password):
    if not gsm_client.waitForNetwork():
        return False
    return gsm_client.gprsConnect(network_apn, network_user, network_password)

def connection_mqtt(mqtt_client, mqtt_user, mqtt_password, mqtt_broker):

    mqtt_client.setServer(mqtt_broker, 1883)

    return mqtt_client.connect("CentralNode", mqtt_user, mqtt_password)

def publish_mqtt(mqtt_client, mqtt_topic, file_source, file_temp):
    all_sent = True

    with open(file_temp, "a") as backup_file:
        with open(file_source, "r") as send_file:
            count = 1
            for send_line in send_file:
                if count > 12:
                    backup_file.write(send_line)
                else:
                    if not mqtt_client.publish(mqtt_topic, send_line.rstrip('\n')):
                        backup_file.write(send_line)
                        all_sent = False
                count += 1
                time.sleep(1)

    os.remove(file_source)
    os.rename(file_temp, file_source)

    return all_sent

def update_counter(new_d_counter, new_a_counter):
    with open("counter.txt", "w") as counter_file:
        counter_file.write(str(new_d_counter) + "," + str(new_a_counter)+ '\n')

def configure_log(logger_name):
    # Create logger
    logger = logging.getLogger(logger_name)
    logger.setLevel(logging.DEBUG)

    # Create console handler and set level to debug
    handler = logging.handlers.RotatingFileHandler(LOG_FILENAME, maxBytes=1024000, backupCount=5)
    handler.setLevel(logging.DEBUG)

    #Create formatter
    formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")

    # Add formatter to handler
    handler.setFormatter(formatter)

    # Add ch to logger
    logger.addHandler(handler)

    return logger

def configure_sim800(port, baudrate, reset_pin):
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(reset_pin, GPIO.OUT, initial=GPIO.HIGH)

    return gsm.GsmClient(port, baudrate)

def configure_radio_rf24(ce_pin=RPI_GPIO_P1_22, csn_pin=RPI_GPIO_P1_24):
    radio = RF24(ce_pin, csn_pin, BCM2835_SPI_SPEED_8MHZ)

    radio.begin()
    time.sleep(0.1)
    radio.setPALevel(RF24_PA_HIGH)                                # Set Power Amplifier level
    radio.setDataRate(RF24_1MBPS)                              # Set transmission rate
    radio.enableDynamicPayloads()

    return radio

def configure_network_rf24(radio_client, channel=120):
    # NRF24L01 Configuration
    octlit = lambda n: int(n, 8)
    this_node = octlit("00")

    network = RF24Network(radio_client)
    network.begin(channel, this_node)    # channel 120

    return network

def check_start_stop_received(start_stop_flags, state, payload):

    start_received, stop_received = start_stop_flags

    audio_buffer = payload["audio_buffer"]

    if start_received:
        state["reading_enable"] = False

        if state["previous_start"]:
            if len(audio_buffer) > 2:
                if state["a_counter"] == 2:
                    save_audio_to_sd(audio_buffer[1:], audio_buffer[0], True)

                    state["a_counter"] = 0
                    update_counter(state["d_counter"], state["a_counter"])

                else:
                    save_audio_to_sd(audio_buffer[1:], audio_buffer[0], False)

                    state["a_counter"] += 1
                    update_counter(state["d_counter"], state["a_counter"])

                del audio_buffer[:]
                state["audio_count"] = 0

                if state["dataReady"]:
                    state["audioReady"] = True

            else:
                state["previous_start"] = True

    elif stop_received:
        state["reading_enable"] = True

        if len(audio_buffer) > 2:
            if state["a_counter"] == 2:
                save_audio_to_sd(audio_buffer[1:], audio_buffer[0], True)

                state["a_counter"] = 0
                update_counter(state["d_counter"], state["a_counter"])

            else:
                save_audio_to_sd(audio_buffer[1:], audio_buffer[0], False)

                state["a_counter"] += 1
                update_counter(state["d_counter"], state["a_counter"])

            del audio_buffer[:]
            state["audio_count"] = 0

            if state["dataReady"]:
                state["audioReady"] = True

        state["previous_start"] = False

def check_payload_received(data_audio_flags, buffer_payload, state, payload):
    data_received, audio_received = data_audio_flags

    if data_received:
        timestamp = get_timestamp()

        buffer_payload.append(payload["temp_ext"])
        buffer_payload.append(payload["umid_ext"])

        if state["d_counter"] == MAX_COUNTER - 1:
            save_data_to_sd(buffer_payload, timestamp, True)

            state["data_ready"] = True

        else:
            save_data_to_sd(buffer_payload, timestamp, False)
            state["d_counter"] += 1
            update_counter(state["d_counter"], state["a_counter"])

    elif audio_received:
        audio_buffer = payload["audio_buffer"]

        if state["audio_count"] == 0:
            timestamp = get_timestamp()

            audio_buffer.append(timestamp)
            audio_buffer.extend(buffer_payload)

            state["reading_enable"] = False

        else:
            audio_buffer.extend(buffer_payload[1:])

        if state["audio_count"] == MAX_AUDIO_COUNT - 1:
            if state["a_counter"] == 2:
                save_audio_to_sd(audio_buffer[1:], audio_buffer[0], True)
                state["a_counter"] = 0

            else:
                save_audio_to_sd(audio_buffer[1:], audio_buffer[0], False)
                state["a_counter"] += 1

            update_counter(state["d_counter"], state["a_counter"])

            del audio_buffer[:]

            if state["data_ready"]:
                state["audio_ready"] = True

            state["audio_count"] = 0
            state["reading_enable"] = True

        else:
            state["audio_count"] += 1

def check_dirs():
    if not os.path.exists("data_collect/"):
        os.makedirs("data_collect/")
    if not os.path.exists("data_to_send/"):
        os.makedirs("data_to_send/")

    if not os.path.exists("audio_collect/"):
        os.makedirs("audio_collect/")
    if not os.path.exists("audio_to_send/"):
        os.makedirs("audio_to_send/")

    if not os.path.exists("counter.txt"):
        with open("counter.txt", "w") as file:
            file.write(str(0) + ","+str(0) + '\n')

def get_counter(state):
    with open("counter.txt", "r") as file:
        line = file.readline()
        line = line.split(",")
        state["d_counter"] = int(line[0])
        state["a_counter"] = int(line[1])

def update_external_readings(state, payload, sensor, pin):
    end_delay = time.time()

    if(end_delay - state["start_delay"] > 10 and state["reading_enable"]):

        umid, temp = Adafruit_DHT.read(sensor, pin)

        if umid is not None and temp is not None:
            payload["umid_ext"] = umid
            payload["temp_ext"] = temp

        state["start_delay"] = time.time()

def send_to_webserice(gsm_client, mqtt, gsm_config, mqtt_config, logger):
    send_ok = True

    data_buffer_path = "data_to_send/buffer_data.txt"
    data_temp_path = "data_to_send/temp.txt"

    audio_buffer_path = "audio_to_send/buffer_audio.txt"
    audio_temp_path = "audio_to_send/temp.txt"

    if not connection_gsm(gsm_client, gsm_config["apn"], gsm_config["user"], gsm_config["pass"]):
        logger.error("Erro na conexão com rede gsm")
        send_ok = False
    elif not connection_mqtt(mqtt, mqtt_config["user"], mqtt_config["pass"], mqtt_config["broker"]):
        logger.error("Erro na conexão com servidor MQTT")
        send_ok = False
    elif not publish_mqtt(mqtt, mqtt_config["topics"]["data"], data_buffer_path, data_temp_path):
        logger.error("Erro no envio dos dados via MQTT")
        send_ok = False

    if not connection_mqtt(mqtt, mqtt_config["user"], mqtt_config["pass"], mqtt_config["broker"]):
        logger.error("Erro na conexao com servidor MQTT")
        send_ok = False
    elif not publish_mqtt(mqtt, mqtt_config["topics"]["audio"], audio_buffer_path, audio_temp_path):
        logger.error("Erro no envio dos audios via MQTT")
        send_ok = False

    return send_ok

def reset_gsm(gsm_client, config):
    #Hardware reset
    GPIO.output(config["pin_rst"], GPIO.LOW)
    time.sleep(1)
    GPIO.output(config["pin_rst"], GPIO.HIGH)
    time.sleep(1)

    gsm_client.restart()

def update_state_variables(state):
    state["d_counter"] = 0
    state["a_counter"] = 0
    state["audio_count"] = 0
    state["audio_ready"] = False
    state["data_ready"] = False
    state["reading_enable"] = True


def main():
    #Descomentar linha abaixo para atualizar data e hora pelo SIM800L
    #set_rasp_timestamp(serial_at, GSM_CONFIG["apn"], GSM_CONFIG["user"], GSM_CONFIG["pass"])

    logger = configure_log("CentralNode-logger")

    serial_at = configure_sim800('/dev/ttyAMA0', 57600, GSM_CONFIG["pin_rst"])

    mqtt_client = MQTT.PubSubClient(serial_at)

    radio = configure_radio_rf24()

    network = configure_network_rf24(radio)

    radio.printDetails()

    # Control Variables
    state_variables = {"d_counter": 0, "a_counter": 0, "audio_count": 0,
                       "start_delay": 0, "reading_enable": True,
                       "previous_start": False, "data_ready": False, "audio_ready": False}

    payload_variables = {"temp_ext": 0.0, "umid_ext": 0.0, "buffer_audio":[]}

    check_dirs()

    get_counter(state_variables)

    try:
        while True:
            network.update()

            package = receive_data(network)

            check_start_stop_received(package[0:2], state_variables, payload_variables)

            check_payload_received(package[2:4], package[4], state_variables, payload_variables)

            update_external_readings(state_variables, payload_variables, SENSOR, SENSOR_PIN)

            if state_variables["audio_ready"] and state_variables["data_ready"]:

                send_ok = send_to_webserice(serial_at, mqtt_client, GSM_CONFIG, MQTT_CONFIG, logger)

                if not send_ok:
                    logger.warning("GSM reiniciado por Software e Hardware!")
                    reset_gsm(serial_at, GSM_CONFIG)

                update_state_variables(state_variables)

                update_counter(state_variables["a_counter"], state_variables["d_counter"])

                # NRF24L01 Reset
                del network

                network = configure_network_rf24(radio)
                radio.printDetails()

    except KeyboardInterrupt:
        traceback.print_exc(file=sys.stdout)
        GPIO.cleanup()
        print("\nGoodbye!\n")
        sys.exit()
    except:
        GPIO.cleanup()
        logger.error(traceback.print_exc(file=sys.stdout))
        sys.exit()

if __name__ == '__main__':
    main()
