import time

class PubSubClient:
	MQTT_VERSION_3_1 = 3
	MQTT_VERSION_3_1_1 = 4

	# MQTT_VERSION: Pick the version
	MQTT_VERSION = MQTT_VERSION_3_1_1

	#MQTT_MAX_PACKET_SIZE: maximm packet size
	MQTT_MAX_PACKET_SIZE = 700

	#MQTT_KEEPALIVE: keepAlive interval in Seconds
	MQTT_KEEPALIVE = 15

	#MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds
	MQTT_SOCKET_TIMEOUT = 15

	# Possible values for client.state()
	MQTT_CONNECTION_TIMEOUT = -4
	MQTT_CONNECTION_LOST = -3
	MQTT_CONNECT_FAILED = -2
	MQTT_DISCONNECTED = -1
	MQTT_CONNECTED =  0
	MQTT_CONNECT_BAD_PROTOCOL = 1
	MQTT_CONNECT_BAD_CLIENT_ID = 2
	MQTT_CONNECT_UNAVAILABLE = 3
	MQTT_CONNECT_BAD_CREDENTIALS = 4
	MQTT_CONNECT_UNAUTHORIZED = 5

	MQTTCONNECT = 1 << 4
	MQTTCONNACK = 2 << 4
	MQTTPUBLISH = 3 << 4
	MQTTPUBREC = 5 << 4
	MQTTPUBREL = 6 << 4
	MQTTPUBCOMP = 7 << 4
	MQTTSUBSCRIBE = 8 << 4
	MQTTSUBACK = 9 << 4
	MQTTUNSUBSCRIBE = 10 << 4
	MQTTUNSUBACK = 11 << 4
	MQTTPINGREQ = 12 << 4
	MQTTPINGRESP = 13 << 4
	MQTTDISCONNECT = 14 << 4
	MQTTReserved = 15 << 4

	MQTTQOS0 = 0 << 1
	MQTTQOS1 = 1 << 1
	MQTTQOS2 = 2 << 1

	def __init__(self, client):
		self.state = self.MQTT_DISCONNECTED
		self.client = client
		self.stream = None
		self.ip = None
		self.domain = None
		self.port = None
		self.buffer = [0]*self.MQTT_MAX_PACKET_SIZE
		self.nextMsgId = 0
		self.lastOutActivity = None
		self.lastInActivity = None
		self.pingOutstanding = None

	def millis(self):
		return time.time() * 1000

	def connect(self, id, user = None, password = None, willTopic = 0, willQos = 0, willRetain = 0, willMessage = 0):
		if (not self.connected()):
			result = 0

			if (self.domain != None):
				result = self.client.connect(self.domain, self.port)
			else:
				result = self.client.connect(self.ip, self.port)

			if result == 1:
				# Leave rooom in the buffer for header and variable length field
				self.nextMsgId = 1
				length = 5
				j = 0
				
				if self.MQTT_VERSION == self.MQTT_VERSION_3_1:
					d = [0x00,0x06, 'M', 'Q','I','s','d', 'p', self.MQTT_VERSION]
					self.MQTT_HEADER_VERSION_LENGTH = 9
				elif self.MQTT_VERSION == self.MQTT_VERSION_3_1_1:
					d = [0x00, 0x04, 'M', 'Q', 'T', 'T', self.MQTT_VERSION]
					self.MQTT_HEADER_VERSION_LENGTH = 7

				for i in range(0,self.MQTT_HEADER_VERSION_LENGTH):
					self.buffer[length] = d[i]
					length += 1				
				v = 0
				if willTopic:
					v = 0x06|(willQos<<3)|(willRetain<<5)
				else:
					v = 0x02

				if user != None:
					v = v|0x80

					if password != None:
						v = v|(0x80>>1)

				self.buffer[length] = v
				length+=1 

				self.buffer[length] = ((self.MQTT_KEEPALIVE) >> 8)
				length += 1

				self.buffer[length] = ((self.MQTT_KEEPALIVE) & 0xFF)
				length += 1

				length = self.writeString(id, self.buffer, length)
				if willTopic:
					length = self.writeString(willTopic, self.buffer, length)
					length = self.writeString(willMessage, self.buffer, length)

				if user != None:
					length = self.writeString(user, self.buffer, length)

					if password != None:
						length = self.writeString(password, self.buffer, length)

				self.write(self.MQTTCONNECT, self.buffer, length-5)
								
				self.lastInActivity = self.lastOutActivity = self.millis()

				while( not self.client.available()):
					t = self.millis()

					if t - self.lastOutActivity >= self.MQTT_SOCKET_TIMEOUT*1000:
						self.state = self.MQTT_CONNECTION_TIMEOUT
						self.client.stop()
						return False
				
				llen = 0
				lenh = self.readPacket(llen)
				
				
 				if(lenh == 4):
					if(self.buffer[3] == 0):
						self.lastInActivity = self.millis()
						self.pingOutstanding = False;

						self.state = self.MQTT_CONNECTED
						return True
					else:
						self.state = self.buffer[3]
				self.client.stop()
			else:
				self.state = self.MQTT_CONNECT_FAILED

			return False

		return True

	def writeString(self, string, buf, pos):
		i = len(string)
		
		pos += 2
		for j in range(0, i):
			buf[pos] = string[j]
			pos += 1

		buf[pos-i-2] = (i >> 8)
		buf[pos-i-1] = (i & 0xFF)

		return pos

	def write(self, header, buf, length):
		lenBuf = [0]*4
		llen = 0
		digit = 0
		pos = 0
		rc = 0
		lenh = length
		firstPass = True
		while lenh > 0 or firstPass:
			firstPass = False
			digit = lenh%128
			lenh = lenh/128

			if(lenh > 0):
				digit |= 0x80

			lenBuf[pos] = digit
			pos += 1
			llen += 1
		buf[4-llen] = header
		for i in range(0, llen):
			buf[5-llen+i] = lenBuf[i]
		
		print("buffer:",buf[4-llen:length+5])
		c = self.client.write(buf[4-llen:], length+1+llen)
		self.lastOutActivity = self.millis()
		return (rc == 1+llen+length)

	def millis(self):
		return time.time() * 1000

	def readPacket(self, lengthLength):
		lenh = 0

		(success, result) = self.readByte()

		if not success: return 0
		self.buffer[lenh] = result
		lenh += 1

		isPublish = (self.buffer[0]&0xF0) == self.MQTTPUBLISH
		multiplier = 1
		length = 0
		digit = 0
		skip = 0
		start = 0
		firstPass = True
		while ((digit & 128) != 0 or firstPass):
			firstPass = False
			if(lenh == 6):
				# Invalid remaining length encoding - kill the connection
				self.state = self.MQTT_DISCONNECTED
				self.client.stop()
				return 0

			(success, result) = self.readByte()

			if not success:  return 0
			digit = result

			self.buffer[lenh] = digit
			lenh += 1

			length += (digit & 127) * multiplier
			multiplier *= 128

		lengthLength = lenh - 1

		if isPublish:
			(success, result) = self.readByte()

			if not success: return 0
			self.buffer[lenh] = result
			lenh += 1

			(success, result) = self.readByte()
			if not success: return 0
			self.buffer[lenh] = result
			lenh += 1

			skip = (self.buffer[lengthLength+1]<<8) + self.buffer[lengthLength+2]
			start = 2

			if(self.buffer[0]&self.MQTTQOS1):
				skip += 2


		for i in range(start, length):
			(success, result) = self.readByte()
			if not success: return 0
			digit = result

			if(self.stream != None):
				if(isPublish and lenh - lengthLength - 2 > skip):
					self.stream.write(digit)

			if (lenh < self.MQTT_MAX_PACKET_SIZE):
				self.buffer[lenh] = digit

			lenh += 1

		if (not self.stream and lenh > self.MQTT_MAX_PACKET_SIZE):
			lenh = 0 # THis will cause the packet to be ignored
		

		return lenh

	def connected(self):
		rc = None

		if(self.client == None):
			rc = False
		else:
			rc = int(self.client.connected())

			if (not rc):
				if(self.state == self.MQTT_CONNECTED):
					self.state = self.MQTT_CONNECTION_LOST
					self.client.flush()
					self.client.stop()

		return rc

	#Reads a byte into result
	def readByte(self):
		previousMillis = self.millis()

		while(not self.client.available()):
			currentMillis = self.millis()
			if currentMillis - previousMillis >= self.MQTT_SOCKET_TIMEOUT * 1000:
				return (False, None)


		cnt, result = self.client.read()
		
		return (True, result)

	def publish(self,topic, payload, retained = False):
		plength = len(payload)
		if(self.connected()):
			if (self.MQTT_MAX_PACKET_SIZE < 5 + 2 + len(topic) + plength):
				# Too  long
				return False

			length = 5
			length = self.writeString(topic, self.buffer, length)
			length += 1
			

			for i in range(0, plength):
				self.buffer[length] = payload[i]
				length += 1

			header = self.MQTTPUBLISH

			if retained:
				header |= 1

			return self.write(header, self.buffer, length-5)

		return False

	def subscribe(self, topic, qos =0):
		if(qos > 1):
			return False

		if (self.MQTT_MAX_PACKET_SIZE < 9 + len(topic)):
			# Too long
			return False

		if (self.connected()):
			# Leave room in the buffer for header and variable length field
			length = 5

			self.nextMsgId += 1

			if(nextMsgId == 0):
				nextMsgId = 1

			length += 1
			self.buffer[length] = (self.nextMsgId >> 8)
			length += 1
			self.buffer[length] = (self.nextMsgId & 0XFF)

			length = self.writeString(topic, self.buffer, length)

			length += 1
			buffer[length] = qos
			return self.write(self.MQTTSUBSCRIBE|MQTTQOS1, self.buffer, length-5)

		return False

	def disconnect(self):
		self.buffer[0] = self.MQTTDISCONNECT
		self.buffer[1] = 0

		self.client.write(self.buffer, 2)
		self.state = self.MQTT_DISCONNECTED

		self.client.stop()

		self.lastInActivity = self.lastOutActivity = millis()

	def setServer(self, domain, port):
		self.domain = domain
		self.port = port

		return self





