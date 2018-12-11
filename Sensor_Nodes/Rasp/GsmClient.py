import time
import serial

class GsmClient:
  def __init__(self, port, baudrate):
    self.ser = serial.Serial('/dev/ttyAMA0', 57600, timeout=5)
    time.sleep(1)
    
  def begin(self):
    return self.init()
  
  def millis(self):
    return time.time() * 1000
      
  def init(self):
    if(not self.testeAT()):
      return False
    
    self.sendAT("&FZ") #Factory + Reset
    self.waitResponse()
    
    self.sendAT("&E0") #Echo off
    if(self.waitResponse() != 1):
      return False
    
    self.getSimStatus()
    return True
    
  def setBaud(self,baud):
    self.sendAT("+IPR=", baud)
  
  def testAT(self,timeout = 10000):
    start = self.millis()
    while self.millis() - start < timeout:
      self.sendAT("")
      
      if(self.waitResponse(2000) == True):
        time.sleep(0.1)
        return True
      
      time.sleep(0.1)
      
    return False
  
  def factoryDefault(self):
    self.sendAT("&FZE0&W")  # Factory + Reset + Echo Off + Write
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
    if(not testAT()):
      return False
    
    self.sendAT("+CLTS=1")
    if self.waitResponse(10000) != 1:
      return False
    
    self.sendAT("&W") 
    self.waitResponse()
    self.sendAT("+CFUN=0")
    if self.waitResponse(10000) != 1:
      return False
    self.sendAT("+CFUN=1,1")
    if self.waitResponse(10000) != 1:
      return False
    
    time.sleep(3)
    
    return self.init()
  
  # def getRegistrationStatus():
  #   #TO_DO 
  #   self.sendAT("+CREG?")
  #   if(waitResponse("\r\n+CREG:") != 1):
  #     return REG_UNKOWN 
  #   streamskipUntil(',') #Skip format (0)
  #   status = stream.readStringUntil('\n').toInt()
  #   self.waitResponse()
    
  #   return (RegStatus)status
        
  def isNetworkConnected(self):
    #s = getRegistrationStatus()
    #return (s == REG_OK_HOME || s == REG_OK_ROAMING)
    self.sendAT("+CREG?")
    return self.waitResponse("\r\n+CREG:")
    
  def waitForNetwork(self,timeout = 60000):
    start = self.millis()
    while self.millis() - start < timeout:   
      if(self.isNetworkConnected()):
        return True
      time.sleep(0.25)
    return False
  
  def gprsConnect(self,apn, user = None, pwd = None):
    self.gprsDisconnect()
    
    # Set the Bearer for the IP 
<<<<<<< HEAD
    self.sendAT("AT+SAPBR=3,1,\"Contype\",\"GPRS\"")  # Set the connection type to GPRS
=======
    self.sendAT("+SAPBR=3,1,\"Contype\",\"GPRS\"")  # Set the connection type to GPRS
>>>>>>> 133db9293bb0ce7681c73ef3063c66ab0d2fd353
    self.self.waitResponse()
    
    self.sendAT("+SAPBR=3,1,\"APN\",\"", apn, '"') # Set the APN
    self.self.waitResponse()
    
    if(user and len(user) > 0):
      self.sendAT("+SAPBR=3,1,\"USER\",\"", user, '"') # Set the user name
      self.waitResponse()
      
    if(pwd and len(pwd) > 0):
      self.sendAT("+SAPBR=3,1,\"PWD\",\"", pwd, '"') # Set the password
      self.waitResponse()
      
    # Define the PDP context
    self.sendAT("+CGACT=1,1")
    self.waitResponse(60000)
    
    # Open the defined GPRS bearer context
    self.sendAT("+SAPBR=1,1")
    self.waitResponse(85000)
    
    # Query the GPRS bearer context status
    self.sendAT("+SAPBR=2,1")
    if(self.waitResponse(30000) != 1):
      return False
    
    # Attach to GPRS
    self.sendAT("+CGATT=1")
    if(self.waitResponse(60000) != 1):
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
    if(self.waitResponse(60000) != 1):
      return False
    
    # Bring Up Wireless Connection with GPRS or CSD
    self.sendAT("+CIICR")
    if(self.waitResponse(60000) != 1):
      return False
    
    # Get Local IP Address, only assigned after connection
    self.sendAT("+CIFSR;E0")
    if(self.waitResponse(10000) != 1):
      return False
    
    # Configure Domain name Server (DNS)
    self.sendAT("+CDNSCFG=\"8.8.8.8\",\"8.8.4.4\"")
    if(self.waitResponse() != 1):
      return False
    
    return True
  
  def gprsDisconnect(self):
    # Shut the TCP/IP connection
    self.sendAT("+CIPSHUT")
    if(self.waitResponse(60000) != 1):
      return False
    
    self.sendAT("+CGATT=0")  # Deactivate the bearer context
    if(self.waitResponse(60000) != 1):
      return False
    
    return True
  
<<<<<<< HEAD
  def sendAT(self,*argv):    
=======
  def sendAT(self, *argv):
>>>>>>> 133db9293bb0ce7681c73ef3063c66ab0d2fd353
    self.streamWrite("AT", argv, "\r\n")
        
  def streamWrite(self, head, *tail):
    for i in tail:
      head += ''.join(i)
<<<<<<< HEAD
=======
    print head
>>>>>>> 133db9293bb0ce7681c73ef3063c66ab0d2fd353
    self.ser.write(head)
    time.sleep(1)
    
  def waitResponse(self, timeout, r1 = 'OK\r\n', r2 = 'ERROR\r\n', r3 = None, r4 = None, r5=None):
    start = self.millis()
    data = ''

    while self.millis() - start < timeout:
      while self.ser.in_waiting > 0:
        a = self.ser.read(self.ser.in_waiting)

        if a <= 0:
          continue

        data += a

<<<<<<< HEAD
        if r1 and data.endswith(r1):
          print(data)
          return True
        elif r2 and data.endswith(r2):
          print(data)
          return True
        elif r3 and data.endswith(r3):
          print(data)
          return True
        elif r4 and data.endswith(r4):
          print(data)
          return True
        elif r5 and data.endswith(r5):
=======
        if r1 and data.endswith(r1) :
          print(data)
          return True
        elif r2 and data.endswith(r2) :
          print(data)
          return True
        elif r3 and data.endswith(r3) :
          print(data)
          return True
        elif r4 and data.endswith(r4) :
          print(data)
          return True
        elif r5 and data.endswith(r5) :
>>>>>>> 133db9293bb0ce7681c73ef3063c66ab0d2fd353
          print(data)
          return True

        #falta colocar o resto
        print data
        return False

    def modemSend(self,buff, len, mux):
      self.sendAT("+CIPSEND=",mux,",", len)
      if self.waitResponse(">") != 1:
        return  0

      self.ser.write(buff[0:len])
      self.ser.flush()

      if self.waitResponse("\r\nDATA ACCEPT:") != 1:
        return 0

      self.streamSkipUntil(',') #Skip mux
      return int(self.streamSkipUntil('\n'))
      
  def streamSkipUntil(self, c):
    start = self.millis()
    while self.millis() - start < timeout:   
      while self.millis() - start < timeout and self.ser.in_waiting <= 0:
        if(self.ser.read() == c):
          return True
      
    return False
    
