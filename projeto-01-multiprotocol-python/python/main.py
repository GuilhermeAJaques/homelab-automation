from MQTT.mqtt_client import MQTTClient
from MQTT.readMqttConf import ReadMqttConf

def main():

    # Read MQTT configuration file
    mqtt_conf = ReadMqttConf(config_file="MQTT/mqttConf.txt")

    # Start MQTT client with the parameters read from the configuration file
    mqtt_client = MQTTClient(host=mqtt_conf.host, port=mqtt_conf.port)
    mqtt_client.connect()

    while True:
        message = input("Enter message to publish (or 'exit' to quit): ")
        if message.lower() == 'exit':
            break
        mqtt_client.publish(topic="test/topic", message=message)
    
    mqtt_client.disconnect()


if __name__ == "__main__":
    main()