import GsmClient as gsm
import PubSubClient as MQTT

SerialAT = gsm.GsmClient('/dev/ttyAMA0', 57600)
mqtt = MQTT.PubSubClient(SerialAT)

SerialAT.restart()
SerialAT.waitForNetwork()
SerialAT.gprsConnect("claro.com.br","claro","claro")
print("Starting MQTT")
mqtt.setServer(domain="200.129.43.208",port="1883")
mqtt.connect(id="CentralNode", user="teste@teste", password="123456")
mqtt.publish(topic="sensors/coleta_data", payload="1, 25.0, 25.0, 12.0, 12.0, 10.0, 20181213102500")



