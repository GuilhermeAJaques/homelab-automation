import paho.mqtt.client as mqtt
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS
import json
import os

class InfluxSubscriber:
    def __init__(self, 
                 mqtt_host, 
                 mqtt_port, 
                 influx_url, 
                 influx_token, 
                 influx_org, 
                 influx_bucket, 
                 mqtt_username=None, 
                 mqtt_password=None):

        # Influx parameters
        self.influx_client = InfluxDBClient(url=influx_url, 
                                            token=influx_token, 
                                            org=influx_org)
        self.write_api = self.influx_client.write_api(write_options=SYNCHRONOUS)
        self.bucket = influx_bucket

        # MQTT parameter
        self.mqtt_client = mqtt.Client()
        
        if mqtt_username and mqtt_password:
            self.mqtt_client.username_pw_set(mqtt_username, mqtt_password)

        self.mqtt_client.on_connect = self._on_connect
        self.mqtt_client.on_message = self._on_message
        self.mqtt_client.connect(mqtt_host, mqtt_port)

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT broker")
            self.mqtt_client.subscribe("#")
        else:
            print(f"Failed to connect, rc={rc}")

    def _on_message(self, client, userdata, message):
        try:
            data = json.loads(message.payload.decode())
            value = data["value"]
            name = data["name"]
            topic = message.topic

            if isinstance(value, str):
                try:
                    value = float(value)
                except:
                    pass

            if value is not None:
                point = Point(topic).field(name, value)
                self.write_api.write(bucket=self.bucket, record=point)
        except Exception as e:
            print(f"Error: {e}")

    def start(self):
        self.mqtt_client.loop_forever()

if __name__ == "__main__":
    subscriber = InfluxSubscriber(
        mqtt_host=os.getenv("MQTT_HOST", "mosquitto"),
        mqtt_port=int(os.getenv("MQTT_PORT", "1883")),
        mqtt_username=os.getenv("MQTT_USERNAME"),
        mqtt_password=os.getenv("MQTT_PASSWORD"),
        influx_url=os.getenv("INFLUX_URL", "http://influxdb:8086"),
        influx_token=os.getenv("INFLUX_TOKEN"),
        influx_org=os.getenv("INFLUX_ORG", "homelab"),
        influx_bucket=os.getenv("INFLUX_BUCKET", "automation")
    )
    subscriber.start()