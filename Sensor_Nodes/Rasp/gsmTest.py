import serial
import time
import RPi.GPIO as GPIO

i = 0

GPIO.setmode(GPIO.BOARD)
GPIO.setup(11, GPIO.OUT, initial=GPIO.HIGH)
time.sleep(10)

gsm = serial.Serial('/dev/ttyAMA0', 57600, timeout=5)
gsm.write("AT\r\n")
time.sleep(5)
print(gsm.read(gsm.in_waiting))


while i < 100:
	GPIO.output(11, GPIO.LOW)
	time.sleep(3)
	#GPIO.output(29, GPIO.HIGH)

	#gsm = serial.Serial('/dev/ttyAMA0', 57600, timeout=5)
	#time.sleep(5)

	gsm.write("AT\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+CFUN=1\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+CPIN?\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+SAPBR=3,1,\"CONTYPE\", \"GPRS\"\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+SAPBR=3,1,\"APN\",\"zap.vivo.com.br\"\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+SAPBR=3,1,\"USER\",\"vivo\"\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+SAPBR=3,1,\"PWD\", \"vivo\"\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+SAPBR=1,1\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+SAPBR=2,1\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+HTTPINIT\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+HTTPPARA=\"CID\",1\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+HTTPPARA=\"URL\",\"http://vsh.pp.ua/TinyGSM/logo.txt\"\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+HTTPACTION=0\r\n")
	time.sleep(10)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+HTTPREAD\r\n")
	time.sleep(10)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+HTTPTERM\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))

	gsm.write("AT+SAPBR=0,1\r\n")
	time.sleep(5)
	print(gsm.read(gsm.in_waiting))
	
	i = i + 1
