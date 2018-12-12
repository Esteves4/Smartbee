import GsmClient as gsm
import PubSubClient as mqtt

SerialAT = gsm.GsmClient('/dev/ttyAMA0', 57600)
SerialAT.begin()