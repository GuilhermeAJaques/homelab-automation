from MQTT.mqtt_client import MQTTClient
from MQTT.readMqttConf import ReadMqttConf
from readGeneralConf import ReadGeneralConf
from ConnectionManager import ConnectionManager
import json
import time
import datetime
import signal
from restAPI import create_app
import threading

def main():
    # Define software constants:
    mqttConfFilePath = "MQTT/mqttConf.txt"
    generalConfFilePath = "generalConf.txt"

    # Read MQTT configuration file
    mqtt_conf = ReadMqttConf(config_file=mqttConfFilePath)
    general_conf = ReadGeneralConf(config_file=generalConfFilePath)

    # Start MQTT client with the parameters read from the configuration file
    mqtt_client = MQTTClient(host=mqtt_conf.host, 
                             port=mqtt_conf.port, 
                             username=mqtt_conf.username, 
                             password=mqtt_conf.password)
    mqtt_client.connect()

    # Start all drivers
    connection = ConnectionManager()
    connection.connect_all()

    # Update subscribe topics
    for topic in connection.subTopics:
        mqtt_client.subscribe(topic)

    # Rest API
    app = create_app(connection)
    flask_thread = threading.Thread(
        target=lambda: app.run(host="0.0.0.0", port=5000, debug=False)
    )
    flask_thread.daemon = True
    flask_thread.start()

    # Call back from MQTT message
    def on_message(topic, payload):
        try:
            try:
                value = json.loads(payload)
            except json.JSONDecodeError:
                if payload.lower() == 'true':
                    value = True
                elif payload.lower() == 'false':
                    value = False
                else:
                    value = payload
            connection.write_variable(topic, value)
        except Exception as e:
            print(f"Error processing message: {e}")

    mqtt_client.on_message_callback = on_message

    # Logic to keep/break the loop
    running = True
    def handle_stop(signum, frame):
        nonlocal running
        running = False
        print("Stopping collector...")

    signal.signal(signal.SIGTERM, handle_stop)
    signal.signal(signal.SIGINT, handle_stop)  # Ctrl+C

    while running:
        variables = connection.read_variables_all()
        for var in variables:
            payload = json.dumps({"name": var["Name"],
                                  "value": var["Value"],
                                  "timestamp": datetime.datetime.now(datetime.timezone.utc).isoformat()})
            
            # Check if has a value from driver
            if var["Value"] is not None:
                mqtt_client.publish(topic=var["Topic"], message=payload)
        
        time.sleep(general_conf.cycle)

    mqtt_client.disconnect()
    connection.disconnect_all()

if __name__ == "__main__":
    main()
