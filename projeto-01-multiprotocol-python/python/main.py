from MQTT.mqtt_client import MQTTClient
from MQTT.readMqttConf import ReadMqttConf
from ConnectionManager import ConnectionManager
import json
import time
import datetime

def main():

    # Read MQTT configuration file
    mqtt_conf = ReadMqttConf(config_file="MQTT/mqttConf.txt")

    # Start MQTT client with the parameters read from the configuration file
    mqtt_client = MQTTClient(host=mqtt_conf.host, port=mqtt_conf.port)
    mqtt_client.connect()

    connection = ConnectionManager()
    connection.connect_all()


    while True:
        message = input("'exit' to quit: ")
        if message.lower() == 'exit':
            break

        variables = connection.read_variables_all()
        for var in variables:
            payload = json.dumps({"name": var["Name"],
                                  "value": var["Value"],
                                  "timestamp": datetime.datetime.now(datetime.timezone.utc).isoformat()})
            mqtt_client.publish(topic=var["Topic"], message=payload)

    mqtt_client.disconnect()
    connection.disconnect_all()

if __name__ == "__main__":
    main()