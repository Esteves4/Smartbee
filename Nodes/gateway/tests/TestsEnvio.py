# coding=utf-8
import GsmClient as gsm 
import PubSubClient as MQTT 
import time 
import datetime 
import os
import logging
import logging.handlers
from struct import * 
from RF24 import * 
from RF24Network import *


LOG_FILENAME = './log/CentralNode.log'

# Create logger
logger = logging.getLogger("CentralNode-logger")
logger.setLevel(logging.DEBUG)

# Create console handler and set level to debug
handler = logging.handlers.RotatingFileHandler(LOG_FILENAME, maxBytes=1024000, backupCount = 5)
handler.setLevel(logging.DEBUG)

#Create formatter
formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")

# Add formatter to handler
handler.setFormatter(formatter)

# Add ch to logger
logger.addHandler(handler)

# SIM800L Configuration
SerialAT = gsm.GsmClient('/dev/ttyAMA0', 57600)
mqtt = MQTT.PubSubClient(SerialAT)

apn = "claro.com.br"
user = "claro"
password = "claro"

#apn = "timbrasil.br"
#user = "tim"
#password = "tim"

# MQTT Configuration
mqtt = MQTT.PubSubClient(SerialAT)

broker = "200.129.43.208"
user_MQTT = "teste@teste"
pass_MQTT = "123456"

topic_audio = "sensors/coleta_audio"
topic_data = "sensors/coleta_data"

# NRF24L01 Configuration
octlit = lambda n:int(n,8)

radio = RF24(RPI_GPIO_P1_22, RPI_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ)
network = RF24Network(radio)
this_node = octlit("00")

radio.begin()
time.sleep(0.1)
radio.setPALevel(RF24_PA_HIGH);                                # Set Power Amplifier level
radio.setDataRate(RF24_1MBPS);                              # Set transmission rate
radio.enableDynamicPayloads()
network.begin(120, this_node)    # channel 120
radio.printDetails()

# Control Variables
counter = 0
audio_count = 0

MAX_COUNTER = 5
MAX_AUDIO_COUNT = 760

bufferAudio = []

dataReady = False
audioReady = False

if not os.path.exists("data_collect/"):
	os.makedirs("data_collect/")
if not os.path.exists("data_to_send/"):
	os.makedirs("data_to_send/")

if not os.path.exists("audio_collect/"):
	os.makedirs("audio_collect/")
if not os.path.exists("audio_to_send"):
	os.makedirs("audio_to_send")

if not os.path.exists("counter.txt"):
	with open("counter.txt","w") as file:
		file.write(str(0) + '\n')

with open("counter.txt","r") as file:
	counter = int(file.readline())


def receiveData():
	network.update()

	if(network.available()):
		header, payload = network.read(201)
		
		if header.type == 68:
			bufferData = list(unpack('<b5fb',bytes(payload)))
			return True, False, bufferData
		elif header.type == 65:
			bufferData = list(unpack('<b50H', bytes(payload)))
			return False, True, bufferData
		
	return False, False, [0]
def setRaspTimestamp(gsm, apn, user, password):
	if not connection_gsm(gsm,apn, user, password):
		return False
	
	time = gsm.getGsmTime()
	time = time.split(',')
	
	if int(time[0]) != 0:
		return False
	os.system("sudo date +%Y/%m/%d -s "+time[1])
	os.system("sudo date +%T -s " + time[2] + " -u")
	os.system("sudo date")
	print(time)
	
	return True

def getTimeStamp():
	timestamp = datetime.datetime.now()
	
	return timestamp

def toString(buffer):
	for i in range(0, len(buffer)):
		if type(buffer[i]) == float:
			buffer[i] = "%3.2f" %buffer[i]
		else:
			buffer[i] = str(buffer[i])
		
	return ",".join(buffer)	

def saveDataToSD(buffer, timestamp, isLast):

	buffer.append(timestamp.strftime("%Y%m%d%H%M%S"))
	msg = toString(buffer)


	with open("data_collect/"+timestamp.strftime("%d_%m_%y") + ".txt", "a") as file:
		file.write(msg + '\n')

	with open("data_to_send/"+"buffer_data.txt", "a") as file:

		if isLast:
			msg += '\n'
		else:
			msg += '/'

		file.write(msg)

def saveAudioToSD(buffer, timestamp):
	buffer.append(timestamp.strftime("%Y%m%d%H%M%S"))
	
	msg = toString(buffer)

	with open("audio_collect/"+timestamp.strftime("%d_%m_%y") + ".txt", "a") as file:
		file.write(msg+'\n')

	#Selecionando 100 amostras do audio
	msg = toString(buffer[0:101]) 
	
	with open("audio_to_send/buffer_audio.txt", "a") as file:
		file.write(msg+'\n')

def connection_gsm(gsmClient, apn, user, password):
	if not gsmClient.restart():
		return False
	if not gsmClient.waitForNetwork():
		return False
	return gsmClient.gprsConnect(apn,user,password)
	
def connection_mqtt(mqttClient, user, password, broker):
	
	mqttClient.setServer(broker, 1883)

	return mqttClient.connect("CentralNode", user, password) 
	
def publish_MQTT(mqttClient, topic, file_source, file_temp):
	
	with open(file_temp, "a") as file2:
		with open(file_source, "r") as file:
			count = 1
			for line in file:
				if count > 5:
					file2.write(line)
				else:	
					if not mqttClient.publish(topic,line.rstrip('\n')):
						file2.write(line)		
				count += 1
				time.sleep(1)
	os.remove(file_source)
	os.rename(file_temp,file_source)

def updateCounter(newCounter):
	with open("counter.txt","w") as file:
		file.write(str(newCounter) + '\n')

#Descomentar linha abaixo para atualizar data e hora pelo SIM800L
#setRaspTimestamp(SerialAT, apn, user, password)


if not connection_gsm(SerialAT,apn, user, password):
	logger.error("Erro na conexão com rede gsm")			
elif not connection_mqtt(mqtt, user_MQTT, pass_MQTT, broker):
	logger.error("Erro na conexão com servidor MQTT")			
else:
	publish_MQTT(mqtt, topic_data, "data_to_send/buffer_data.txt", "data_to_send/temp.txt")

if not connection_mqtt(mqtt, user_MQTT, pass_MQTT, broker):
	logger.error("Erro na conexao com servidor MQTT")
else:
	publish_MQTT(mqtt, topic_audio, "audio_to_send/buffer_audio.txt", "audio_to_send/temp.txt")
		

		
		


