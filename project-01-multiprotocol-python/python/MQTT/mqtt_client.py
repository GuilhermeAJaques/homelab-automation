import paho.mqtt.client as mqtt
import time
from Logger import Logger, Criticality, Class

class MQTTClient:
    def __init__(self, host, port, username=None, password=None):
        # Initialize MQTT broker parameters
        self.host = host
        self.port = port
        self.connected = False

        # Create MQTT client instance
        self.client = mqtt.Client()

        # Login with credentialsif username and password:
        if username and password:
            self.client.username_pw_set(username, password)

        # Callback for connection event
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect

        # Call back from mqtt subscribe
        self.client.on_message = self._on_message
        self.on_message_callback = None

    def connect(self):
        try:
            # Connect to MQTT broker
            self.client.connect(self.host, self.port)
            # Start loop for reconection
            self.client.loop_start()
        except Exception as e:
            Logger.log(Class.MQTT, Criticality.ERROR ,"Error connecting to MQTT broker: {}".format(e))

        # Delay to allow connection to establish
        time.sleep(0.5)

        # Check if the client is connected to the broker
        if not self.connected:
            Logger.log(Class.MQTT, Criticality.ERROR ,"Failed to connect to MQTT broker")

    def disconnect(self):
        try:
            # Disconnect from MQTT broker
            self.client.loop_stop()
            self.client.disconnect()
            self.connected = False
            Logger.log(Class.MQTT, Criticality.INFO ,"Disconnected from MQTT broker")
        except Exception as e:
            Logger.log(Class.MQTT, Criticality.ERROR ,"Error disconnecting from MQTT broker: {}".format(e))

    def publish(self, topic, message):
        try:
            # Check if the client is connected to the broker before publishing
            if self.connected:
                # Send MQTT message
                result = self.client.publish(topic, message)
                if result.rc != mqtt.MQTT_ERR_SUCCESS:
                    Logger.log(Class.MQTT, Criticality.WARNING ,"Failed to publish message: {}".format(result.rc))
            else:
                Logger.log(Class.MQTT, Criticality.WARNING ,"Failed to publish message: Not connected to MQTT broker")
        except Exception as e:
            Logger.log(Class.MQTT, Criticality.WARNING ,"Error publishing message: {}".format(e))

    def subscribe(self, topic):
        try:
            # Check if the client is connected to the broker before publishing
            if self.connected:
                # Send MQTT message
                self.client.subscribe(topic)
            else:
                Logger.log(Class.MQTT, Criticality.ERROR ,"Failed to subscribe message: Not connected to MQTT broker")
        except Exception as e:
            Logger.log(Class.MQTT, Criticality.ERROR ,"Error subscribe message: {}".format(e))


    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.connected = True
            Logger.log(Class.MQTT, Criticality.INFO ,"Connected to MQTT broker")
        else:
            Logger.log(Class.MQTT, Criticality.ERROR ,"Failed to connect to MQTT broker, return code: {}".format(rc))

    def _on_disconnect(self, client, userdata, rc):
        self.connected = False
        Logger.log(Class.MQTT, Criticality.INFO ,f"Disconnected from MQTT broker, rc={rc}")

    def _on_message(self, client, userdata, message):
        if self.on_message_callback:
            topic = message.topic
            payload = message.payload.decode()
            self.on_message_callback(topic, payload)