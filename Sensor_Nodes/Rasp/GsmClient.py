import time 
import serial 
import TinyGsmFifo


class GsmClient:
	SIM_ERROR = 0
  	SIM_READY = 1
  	SIM_LOCKED = 2

	REG_UNREGISTERED = 0
	REG_SEARCHING = 2
	REG_DENIED = 3
	REG_OK_HOME = 1
	REG_OK_ROAMING = 5
	REG_UNKNOWN = 4

	TINY_GSM_MUX_COUNT = 5
	TINY_GSM_RX_BUFFER = 64


	sockets = [None]*TINY_GSM_MUX_COUNT

	def __init__(self, port, baudrate, mux = 1):
		self.ser = serial.Serial('/dev/ttyAMA0', 57600, timeout=5)
		self.mux = 1
		self.sock_available = 0
		self.prev_check = 0
		self.sock_connected = False
		self.got_data = False
		self.sockets[mux] = self
		self.rx = TinyGsmFifo.TinyGsmFifo(self.TINY_GSM_RX_BUFFER)

		time.sleep(1)
		
	def begin(self):
		return self.init()
				
	def init(self):
		if(not self.testAT()):
			return False
		
		self.sendAT("&FZ") #Factory + Reset
		self.waitResponse()
		
		#self.sendAT("E0") #Echo off
		self.sendAT("E1") #Echo on
		if(self.waitResponse() != 1):
			return False
		
		self.getSimStatus()
		return True
		
	def setBaud(self,baud):
		self.sendAT("+IPR=", str(baud))

	def getSimStatus(self,timeout = 10000):
		start = self.millis()
		while self.millis() - start < timeout:
			self.sendAT("+CPIN?")
			if(self.waitResponse(r1="\r\n+CPIN:") != 1):
				time.time(1)
				continue
			status = self.waitResponse(r1="READY", r2="SIM PIN", r3="SIM PUK", r4="NOT INSERTED")
			self.waitResponse()
			if(status == 2 or status == 3):
				return self.SIM_LOCKED
			elif(status == 1):
				return self.SIM_READY
			else:
				return self.SIM_ERROR

		return self.SIM_ERROR
	
	def testAT(self,timeout = 10000):
		start = self.millis()
		while self.millis() - start < timeout:
			self.sendAT("")
			
			if(self.waitResponse(timeout=200) == 1):
				time.sleep(0.1)
				return True
			
			time.sleep(0.1)
			
		return False
	
	def factoryDefault(self):
		self.sendAT("&FZE1&W")  # Factory + Reset + Echo Off + Write
		self.waitResponse()
		self.sendAT("+IPR=0")   # Auto-baud
		self.waitResponse()
		self.sendAT("+IFC=0,0") # No Flow Control
		self.waitResponse()
		self.sendAT("+IFC=3,3") # 8 data 0 parity 1 stop
		self.waitResponse()
		self.sendAT("+CSCLK=0") # Disable Slow Clock
		self.waitResponse()
		self.sendAT("&W")       # Write configuration
		return self.waitResponse() == 1
	
	def restart(self):
		if(not self.testAT()):
			return False
		
		self.sendAT("+CLTS=1")
		if self.waitResponse(timeout=10000) != 1:
			return False
		
		self.sendAT("&W") 
		self.waitResponse()
		self.sendAT("+CFUN=0")
		if self.waitResponse(timeout=10000) != 1:
			return False
		self.sendAT("+CFUN=1,1")
		if self.waitResponse(timeout=10000) != 1:
			return False
		
		time.sleep(3)
		
		return self.init()
	
	def getRegistrationStatus(self):
		self.sendAT("+CREG?")
		if(self.waitResponse(r1="\r\n+CREG:") != 1):
			return self.REG_UNKNOWN 

		self.streamSkipUntil(',') #Skip format (0)

		status = int(self.ser.read_until('\n'))

		self.waitResponse()
		
		return status
	
	def getSignalQuality(self):
		self.sendAT("+CSQ")
		if self.waitResponse(r1="\r\n+CSQ:") != 1:
			return 99
		res = int(self.ser.read_until(','))
		self.waitResponse()

		return res

	def isNetworkConnected(self):
		s = self.getRegistrationStatus()
		return (s == self.REG_OK_HOME or s == self.REG_OK_ROAMING)
			 
	def waitForNetwork(self,timeout = 60000):
		start = self.millis()
		end = self.millis()
		while self.millis() - start < timeout:   
			if(self.isNetworkConnected()):
				return True
			time.sleep(0.25)
		return False
	
	def gprsConnect(self,apn, user = None, pwd = None):
		self.gprsDisconnect()
		
		# Set the Bearer for the IP 
		self.sendAT("+SAPBR=3,1,\"Contype\",\"GPRS\"")  # Set the connection type to GPRS
		self.waitResponse()
		
		self.sendAT("+SAPBR=3,1,\"APN\",\"", apn, '"') # Set the APN
		self.waitResponse()
		
		if(user and len(user) > 0):
			self.sendAT("+SAPBR=3,1,\"USER\",\"", user, '"') # Set the user name
			self.waitResponse()
			
		if(pwd and len(pwd) > 0):
			self.sendAT("+SAPBR=3,1,\"PWD\",\"", pwd, '"') # Set the password
			self.waitResponse()
			
		# Define the PDP context
		self.sendAT("+CGACT=1,1")
		self.waitResponse(timeout=60000)
		
		# Open the defined GPRS bearer context
		self.sendAT("+SAPBR=1,1")
		self.waitResponse(timeout=85000)
		
		# Query the GPRS bearer context status
		self.sendAT("+SAPBR=2,1")
		if(self.waitResponse(timeout=30000) != 1):
			return False
		
		# Attach to GPRS
		self.sendAT("+CGATT=1")
		if(self.waitResponse(timeout=60000) != 1):
			return False
		
		# Set to multi-IP
		self.sendAT("+CIPMUX=1")
		if(self.waitResponse() != 1):
			return False
		
		# Put in "quick send" mode (thus no extra "Send OK")
		self.sendAT("+CIPQSEND=1")
		if(self.waitResponse() != 1):
			return False
		
		# Set to get data manually
		self.sendAT("+CIPRXGET=1")
		if(self.waitResponse() != 1):
			return False
		
		# Start Task and Set APN, USER NAME, PASSWORD
		self.sendAT("+CSTT=\"", apn, "\",\"", user, "\",\"", pwd, "\"")
		if(self.waitResponse(timeout=60000) != 1):
			return False
		
		# Bring Up Wireless Connection with GPRS or CSD
		self.sendAT("+CIICR")
		if(self.waitResponse(timeout=60000) != 1):
			return False
		
		# Get Local IP Address, only assigned after connection
		self.sendAT("+CIFSR;")
		if(self.waitResponse(timeout=10000) != 1):
			return False
		
		# Configure Domain name Server (DNS)
		self.sendAT("+CDNSCFG=\"8.8.8.8\",\"8.8.4.4\"")
		if(self.waitResponse() != 1):
			return False
		
		return True
	
	def gprsDisconnect(self):
		# Shut the TCP/IP connection
		self.sendAT("+CIPSHUT")
		if(self.waitResponse(timeout=60000) != 1):
			return False
		
		self.sendAT("+CGATT=0")  # Deactivate the bearer context
		if(self.waitResponse(timeout=60000) != 1):
			return False
		
		return True
	

	def sendAT(self, *argv):
		self.streamWrite("AT", argv, "\r\n")
				
	def streamWrite(self, head, *tail):
		for i in tail:
			head += ''.join(i)

		self.ser.write(head)
		time.sleep(1)

	def waitResponse(self, timeout = 1000, data = "", r1 = 'OK\r\n', r2 = 'ERROR\r\n', r3 = None, r4 = None, r5=None):
		start = self.millis()
		end = self.millis()
		index = 0

		while end - start < timeout:
			while self.ser.in_waiting > 0:
				a = self.ser.read()

				if a <= 0:
					continue

				data += a

				if r1 and data.endswith(r1):
					index = 1
					break;
				elif r2 and data.endswith(r2):
					index = 3
					break;
				elif r3 and data.endswith(r3):
					index = 3
					break;
				elif r4 and data.endswith(r4):
					index = 4
					break;
				elif r5 and data.endswith(r5):
					index = 5
					break;
				elif data.endswith("\r\n+CIPRXGET:"):
					mode = self.ser.read_until(',')
					if(int(mode) == 1):
						mux = int(self.ser.read_until('\n'))
						if(mux >= 0 and mux < self.TINY_GSM_MUX_COUNT and self.sockets[mux]):
							self.sockets[mux].got_data = True

						data = ""
					else:
						data += mode
				elif data.endswith("CLOSED\r\n"):
					n1 = data.rfind("\r\n", len(data) - 8)
					coma = data.find(',', n1+2)

					mux = int(data[n1+2:coma])
					if(mux >= 0 and mux < self.TINY_GSM_MUX_COUNT and self.sockets[mux]):
						self.sockets[mux].sock_connected = False 

					data = ""
					print("### Closed: ", mux)

			end = self.millis()
			if index: break
	
		if not index:
			data = data.lstrip()
			if(len(data)):
				print("### Unhandled:", data)
			data = ""
		print(data)
		return index



	def streamSkipUntil(self, c, timeout = 3000):
		start = self.millis()
		while self.millis() - start < timeout:   
			if(self.ser.read() == c):
				return True
			
		return False
		
	def millis(self):
		return time.time() * 1000

	def connect(self,host, port):
		self.stop()
		self.rx.clear()
		self.sock_connected = self.modemConnect(host, port, self.mux)
		return self.sock_connected

	def stop(self):
		self.sendAT("\r\n+CIPCLOSE=", str(self.mux))
		self.sock_connected = False
		self.waitResponse()
		self.rx.clear()

	def write(self,buf, size):
		if buf == None: return 0

		self.maintain()

		return self.modemSend(buf, size, self.mux)

	def available(self):
		if (not self.rx.size() and self.sock_connected):
			# Workaround: sometimes SIM800 forgets to notify about data arrival.
			# TODO: Currently we ping the module periodically,
			# but maybe there's a better indicator that we need to poll

			if (self.millis() - self.prev_check > 500):
				self.got_data = True
				self.prev_check = self.millis()

			self.maintain()

		return self.rx.size() + self.sock_available

	def read(buf = [], size = 1):
		self.maintain()
		cnt = 0

		while (cnt < size and self.sock_connected):
			chunk = min(size-cnt, self.rx.size())

			if(chunk > 0):
				self.rx.get(buf, chunk)
				buf.append(chunk)
				cnt += chunk
				continue

			# TODO: Read directly into user buffer?
			self.maintain()

			if(sock_available > 0):
				self.modemRead(self.rx.free(), self.mux)
			else:
				break

		return cnt

	def flush(self):
		self.ser.flush()

	def connected(self):
		if(self.available()):
			return True

		return self.sock_connected

	def maintain(self):
		for mux in range(0, self.TINY_GSM_MUX_COUNT):
			sock = self.sockets[mux]

			if sock and sock.got_data:
				sock.got_data = False
				sock.sock_available = self.modemGetAvailable(mux)

		while self.ser.in_waiting:
			self.waitResponse(timeout=10,r1=None, r2=None)

	def modemGetConnected(self, mux):
		self.sendAT("+CIPSTATUS=", str(mux))
		res = self.waitResponse(r1=",\"CONNECTED\"", r2="\"CLOSED\"",r3=",\"CLOSING\"", r4=",\"INITIAL\"")
		self.waitResponse()
		return 1 == res

	def modemSend(self,buff, lenh, mux):
		self.sendAT("+CIPSEND=",str(mux),",", str(lenh))
		if self.waitResponse(r1=">") != 1:
			return  0

		self.ser.write(buff[0:lenh])
		self.ser.flush()

		if self.waitResponse(r1="\r\nDATA ACCEPT:") != 1:
			return 0

		self.streamSkipUntil(',') #Skip mux
		return int(self.streamSkipUntil('\n'))

	def modemRead(size, mux):
		self.sendAT("+CIPRXGET=2", str(mux), ',', str(size))

		if(self.waitResponse(r1="+CIPRXGET:") != 1):
			return 0

		self.streamSkipUntil(',') #Skip mode 2/3
		self.streamSkipUntil(',') #Skip mux
		lenh = int(self.ser.read_until(','))
		self.sockets[mux].sock_available = int(self.ser.read_until('\n'))

		for i in range(0, lenh):
			while (not self.ser.in_waiting ):
				pass

			c = self.ser.read()
			self.sockets[mux].rx.put(c)

		self.waitResponse()
		return len

	def modemGetAvailable(self, mux):
		self.sendAT("+CIPRXGET=4,", str(mux))
		result = 0

		if self.waitResponse(r1="+CIPRXGET:") == 1:
			self.streamSkipUntil(',') #Skip mode 4
			self.streamSkipUntil(',') #Skip mux]
			result = int(self.ser.read_until('\n'))
			self.waitResponse()

		if(not result):
			self.sockets[mux].sock_connected = self.modemGetConnected(mux)

		return result

	def modemConnect(self, host, port, mux, ssl = False):
		rsp = None
		self.sendAT("+CPSTART=", str(mux), ',', "\"TCP", "\",\"", str(host), "\",", str(port))
		rsp = self.waitResponse(75000,
									"CONNECT OK\r\n",
									"CONNECT FAIL\r\n",
									"ALREADY CONNECT\r\n",
									"ERROR\r\n",
									"CLOSE OK\r\n") # Happens when HTTPS handshake fails
		return (1 == rsp)





