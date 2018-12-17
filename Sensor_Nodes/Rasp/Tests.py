import GsmClient as gsm
import PubSubClient as MQTT

SerialAT = gsm.GsmClient('/dev/ttyAMA0', 57600)
mqtt = MQTT.PubSubClient(SerialAT)

print("Restart:",SerialAT.restart())
print("Wait Network:",SerialAT.waitForNetwork())
print("GPRS Connect:",SerialAT.gprsConnect("timbrasil.br","tim","tim"))
print("Starting MQTT")
print("Set Server:",mqtt.setServer(domain="200.129.43.208",port="1883"))
print("Connect:",mqtt.connect(id="CentralNode", user="teste@teste", password="123456"))
print("Publish:",mqtt.publish(topic="sensors/coleta_data", payload="1,25.0,25.0,12.0,12.0,10.0,20181213102500"))



