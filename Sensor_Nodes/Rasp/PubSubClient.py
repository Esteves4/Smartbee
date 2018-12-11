class PubSubClient
  MQTT_VERSION_3_1 = 3
  MQTT_VERSION_3_1_1 = 4

  # MQTT_VRSION: Pick the version
  MQTT_VERSION = MQTT_VERSION_3_1_1

  #MQTT_MAX_PACKET_SIZE: maximm packet size
  MQTT_MAX_PACKET_SIZE = 700

  #MQTT_KEEPALIVE: keepAlive interval in Seconds
  MQTT_KEEPALIVE = 15

  #MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds
  MQTT_SOCKET_TIMEOUT = 15

  # Possible values for client.state()
  MQTT_CONNECTION_TIMEOUT = -4
  MQTT_CONNECTION_LOST = -3
  MQTT_CONNECT_FAILED = -2
  MQTT_DISCONNECTED = -1
  MQTT_CONNECTED =  0
  MQTT_CONNECT_BAD_PROTOCOL = 1
  MQTT_CONNECT_BAD_CLIENT_ID = 2
  MQTT_CONNECT_UNAVAILABLE = 3
  MQTT_CONNECT_BAD_CREDENTIALS = 4
  MQTT_CONNECT_UNAUTHORIZED = 5

  MQTTCONNECT = 1 << 4
  MQTTCONNACK = 2 << 4
  MQTTPUBLISH = 3 << 4
  MQTTPUBREC = 5 << 4
  MQTTPUBREL = 6 << 4
  MQTTPUBCOMP = 7 << 4
  MQTTSUBSCRIBE = 8 << 4
  MQTTSUBACK = 9 << 4
  MQTTUNSUBSCRIBE = 10 << 4
  MQTTUNSUBACK = 11 << 4
  MQTTPINGREQ = 12 << 4
  MQTTPINGRESP = 13 << 4
  MQTTDISCONNECT = 14 << 4
  MQTTReserved = 15 << 4

  MQTTQOS0 = 0 << 1
  MQTTQOS1 = 1 << 1
  MQTTQOS2 = 2 << 1

  def __init__(self, client):
    self.state = MQTT_DISCONNECTED
    self.setClient(client)
    self.stream = None
    self.ip = None
    self.port = None

  def connect(self,id):
    return self.connect(id, None, None, 0, 0, 0, 0)

  def connect(self, id, user, pass):
    return connect(id, user, pass, 0, 0, 0, 0)

  def connect(id, willTopic, willQos, willRetain, willMessage):
    return connect(id, None, None, willTopic, willQos, willRetain, willMessage) 
    
  def connect(id, user, user, pass, willTopic, willQos, willRetain, willMessage)
    if (not connected()):
      result = 0

      self.client.connect()