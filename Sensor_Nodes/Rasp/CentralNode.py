import GsmClient as gsm
import PubSubClient as MQTT

SerialAT = gsm.GsmClient('/dev/ttyAMA0', 57600)
mqtt = MQTT.PubSubClient(SerialAT)

SerialAT.restart()
SerialAT.waitForNetwork()
SerialAT.gprsConnect("claro.com.br","claro","claro")
print("Starting MQTT")
mqtt.setServer("200.129.43.208",1883)
mqtt.connect("CentralNode", "teste@teste", "123456")
mqtt.publish("sensors/coleta_data", "1, 25.0, 25.0, 12.0, 12.0, 10.0, 20181213102500")



