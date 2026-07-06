import paho.mqtt.client as mqtt
import time

class MQTTClient:
    def __init__(self, host, port):
        # Initialize MQTT broker parameters
        self.host = host
        self.port = port
        self.connected = False

        # Create MQTT client instance
        self.client = mqtt.Client()

        # Callback for connection event
        self.client.on_connect = self._on_connect

    def connect(self):
        try:
            # Connect to MQTT broker
            self.client.connect(self.host, self.port)
            # Start loop for reconection
            self.client.loop_start()
        except Exception as e:
            print("Error connecting to MQTT broker: {}".format(e))

        # Delay to allow connection to establish
        time.sleep(0.5)

        # Check if the client is connected to the broker
        if not self.connected:
            print("Failed to connect to MQTT broker")

    def disconnect(self):
        try:
            # Disconnect from MQTT broker
            self.client.loop_stop()
            self.client.disconnect()
            self.connected = False
            print("Disconnected from MQTT broker")
        except Exception as e:
            print("Error disconnecting from MQTT broker: {}".format(e))

    def publish(self, topic, message):
        try:
            # Check if the client is connected to the broker before publishing
            if self.connected:
                # Send MQTT message
                result = self.client.publish(topic, message)
                if result.rc != mqtt.MQTT_ERR_SUCCESS:
                    print("Failed to publish message: {}".format(result.rc))
            else:
                print("Failed to publish message: Not connected to MQTT broker")
        except Exception as e:
            print("Error publishing message: {}".format(e))

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.connected = True
            print("Connected to MQTT broker")
        else:
            print("Failed to connect to MQTT broker, return code: {}".format(rc))