import Adafruit_DHT
import time
import RPi.GPIO as GPIO

# Tipo do sensor
sensor = Adafruit_DHT.DHT22

GPIO.setmode(GPIO.BOARD)

# GPIO conectada
pino_sensor = 23

while(1):
	umid, temp = Adafruit_DHT.read_retry(sensor, pino_sensor)

	if umid is not None and temp is not None:
		print ("Temp = {0:0.1f} Umidade = {1:0.1f}\n").format(temp,umid)
	else:
		print("Falha ao ler dados do sensor !!!")
	time.sleep(3)
