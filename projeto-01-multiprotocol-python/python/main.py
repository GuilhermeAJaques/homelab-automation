from MQTT.mqtt_client import MQTTClient
from MQTT.readMqttConf import ReadMqttConf
from Field_protocols.s7_nonOptimized import S7_NonOptimized
from Field_protocols.OPC_UA import OPC_UA
from Field_protocols.ModbusTCP import ModbusTCP
from Field_protocols.EthernetIP import EthernetIP

def main():

    # Read MQTT configuration file
    mqtt_conf = ReadMqttConf(config_file="MQTT/mqttConf.txt")

    # Start MQTT client with the parameters read from the configuration file
    mqtt_client = MQTTClient(host=mqtt_conf.host, port=mqtt_conf.port)
    mqtt_client.connect()

    ethernetIP_client = EthernetIP("192.168.15.201")
    ethernetIP_client.connect()


    while True:
        message = input("'exit' to quit: ")
        if message.lower() == 'exit':
            break

        print(f"iVar: {ethernetIP_client.read_variable("iVar")}")
        print(f"xVar: {ethernetIP_client.read_variable("xVar")}")
        print(f"flVar: {ethernetIP_client.read_variable("flVar")}")
        print(f"sVar: {ethernetIP_client.read_variable("sVar")}")
    
    mqtt_client.disconnect()


if __name__ == "__main__":
    main()