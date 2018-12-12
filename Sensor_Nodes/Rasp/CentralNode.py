import GsmClient as gsm
import PubSubClient as mqtt

SerialAT = gsm.GsmClient('/dev/ttyAMA0', 57600)
print(SerialAT.sendAT())
print(SerialAT.waitResponse())
