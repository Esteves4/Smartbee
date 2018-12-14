import GsmClient as gsm
import PubSubClient as MQTT
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
radio = RF24(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ)
network = RF24Network(radio)
this_node = 00

radio.begin()
time.sleep(0.1)
radio.setPALevel(RF24_PA_HIGH);                                # Set Power Amplifier level
radio.setDataRate(RF24_250KBPS);                              # Set transmission rate
network.begin(120, this_node)    # channel 120

# Control Variables
counter = 0
MAX_COUNTER = 5

def receiveData():
	network.update()

	if(network.available()):
		header, payload = network.read(41)
		bufferData = list(struct.unpack('ifffffb', payload))
		return True, bufferData

	return False, [0]

def getTimeStamp():

	return datetime.datetime.now()

def toString(buffer):
	return ",".join(str(e) for in buffer)	

def saveToSD(buffer, timestamp, isLast):

	buffer.append(timestamp.strftime("%Y%m%d%H%M%S"))
	msg = toString(buffer)

	with open(timestamp.strftime("%d_%m_%y") + ".txt", "a") as file:
		file.write(msg + '\n')

	with open("buffer.txt", "a") as file:

		if isLast:
			msg += '\n'
		else:
			msg += '/'

		file.write(msg)

def publish_GET(SerialAT):
	with open("buffer.txt", "a") as file:
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
			SerialAT.sendAT("+HTTPPARA=\"URL\",\"api.thingspeak.com/update?api_key=X1H7B6RD67MHVGIZ&field1=" + line +"\"")
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

	dataReceived, bufferData = receiveData()

	if(dataReceived):
		timestamp = getTimeStamp()

		counter += 1
		print(counter)
		if counter == MAX_COUNTER - 1:
			saveToSD(bufferData, timestamp, True)

			SerialAT.restart()
			SerialAT.waitForNetwork()
			SerialAT.gprsConnect("claro.com.br","claro","claro")

			publish_GET(SerialAT)
			counter = 0
		else:
			saveToSD(bufferData, timestamp, False)



