import GsmClient as gsm
import PubSubClient as mqtt

SerialAT = gsm.GsmClient('/dev/ttyAMA0', 57600)
SerialAT.restart()
SerialAT.waitForNetwork()
SerialAT.gprsConnect("claro.com.br","claro","claro")
SerialAT.sendAT("+CSQ")
SerialAT.waitResponse()
SerialAT.sendAT("+HTTPINIT")
SerialAT.waitResponse()
SerialAT.sendAT("+HTTPPARA=\"CID\",1")
SerialAT.waitResponse()
SerialAT.sendAT("+SAPBR=2,1")
SerialAT.waitResponse()
SerialAT.sendAT("+SAPBR=4,1")
SerialAT.waitResponse()
SerialAT.sendAT("+CGATT?")
SerialAT.waitResponse()
SerialAT.sendAT("+HTTPPARA=\"URL\",\"api.thingspeak.com/update?api_key=X1H7B6RD67MHVGIZ&field1=99773\"")
SerialAT.waitResponse()
SerialAT.sendAT("+HTTPACTION=0")
SerialAT.waitResponse()
SerialAT.sendAT("+HTTPREAD")
SerialAT.waitResponse()
SerialAT.sendAT("+HTTPTERM")
SerialAT.waitResponse()
SerialAT.sendAT("+SAPBR=0,1")
SerialAT.waitResponse()



