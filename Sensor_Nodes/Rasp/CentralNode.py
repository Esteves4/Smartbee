import GsmClient as gsm 
import PubSubClient as MQTT 
import time 
import datetime 
import os
from struct import * 
from RF24 import * 
from RF24Network import *


# SIM800L Configuration
SerialAT = gsm.GsmClient('/dev/ttyAMA0', 57600)

apn = "claro.com.br"
user = "claro"
password = "claro"

# MQTT Configuration
mqtt = MQTT.PubSubClient(SerialAT)

broker = "200.129.43.208"
user_MQTT = "teste@teste"
pass_MQTT = "123456"


# NRF24L01 Configuration
octlit = lambda n:int(n,8)

radio = RF24(RPI_GPIO_P1_22, RPI_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ)
network = RF24Network(radio)
this_node = octlit("00")

radio.begin()
time.sleep(0.1)
radio.setPALevel(RF24_PA_HIGH);                                # Set Power Amplifier level
radio.setDataRate(RF24_250KBPS);                              # Set transmission rate
radio.enableDynamicPayloads()
network.begin(120, this_node)    # channel 120
radio.printDetails()

# Control Variables
counter = 0
audio_count = 0

MAX_COUNTER = 5
MAX_AUDIO_COUNT = 380

bufferAudio = []

if not os.path.exists("data_collect/"):
	os.makedirs("data_collect/")
if not os.path.exists("data_to_send/"):
	os.makedirs("data_to_send/")

if not os.path.exists("audio_collect/"):
	os.makedirs("audio_collect/")
if not os.path.exists("audio_to_send"):
	os.makedirs("audio_to_send")

def receiveData():
	network.update()

	if(network.available()):
		header, payload = network.read(201)
		print(header.type)
		if header.type == 68:
			bufferData = list(unpack('<b5fb',bytes(payload)))
			return True, False, bufferData
		elif header.type == 65:
			bufferData = list(unpack('<b10H', bytes(payload)))
			return False, True, bufferData
		
	return False, False, [0]

def getTimeStamp():

	return datetime.datetime.now()

def toString(buffer):
	return ",".join(str(e) for e in buffer)	

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
	
	with open("audio_to_send/"+"buffer_audio.txt", "a") as file:
		file.write(msg[0:100]+'\n')
	
		

def publish_GET(SerialAT):
	with open("toSend/"+"buffer.txt", "r") as file:
		for line in file:
			SerialAT.sendAT("+CSQ")
			SerialAT.waitResponse()
			SerialAT.sendAT("+HTTPINIT")
			SerialAT.waitResponse()
			SerialAT.sendAT("+HTTPPARA=\"CID\",1")
			SerialAT.waitResponse()
			SerialAT.sendAT("+SAPBR=2,1")
			SerialAT.waitResponse()
			SerialAT.sendAT("+SAPBR=4,1")
			SerialAT.waitResponse()
			SerialAT.sendAT("+CGATT?")
			SerialAT.waitResponse()
			SerialAT.sendAT("+HTTPPARA=\"URL\",\"api.thingspeak.com/update?api_key=X1H7B6RD67MHVGIZ&field1=oi\"")
			SerialAT.waitResponse()
			SerialAT.sendAT("+HTTPACTION=0")
			SerialAT.waitResponse()
			SerialAT.sendAT("+HTTPREAD")
			SerialAT.waitResponse()
			SerialAT.sendAT("+HTTPTERM")
			SerialAT.waitResponse()
			SerialAT.sendAT("+SAPBR=0,1")
			SerialAT.waitResponse()	

while(1):
	network.update()

	dataReceived, audioReceived, bufferData = receiveData()

	if(dataReceived):
		timestamp = getTimeStamp()

		if counter == MAX_COUNTER - 1:
			saveDataToSD(bufferData, timestamp, True)

			SerialAT.restart()
			SerialAT.waitForNetwork()
			SerialAT.gprsConnect("claro.com.br","claro","claro")

			publish_GET(SerialAT)
			counter = 0
		else:
			saveDataToSD(bufferData, timestamp, False)
			counter += 1

	elif(audioReceived):
		if(audio_count == 0):
			timestamp = getTimeStamp()
			bufferAudio.append(timestamp)
			bufferAudio = bufferAudio + bufferData
		else:
			bufferAudio = bufferAudio + bufferData[1:]
		
		if(audio_count == MAX_AUDIO_COUNT - 1):
			saveAudioToSD(bufferAudio[1:], bufferAudio[0])		
			del bufferAudio[:]
			audio_count = 0
		else:
			audio_count += 1

			
		
		


