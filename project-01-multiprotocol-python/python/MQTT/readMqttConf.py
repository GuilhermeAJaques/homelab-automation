import configparser

class ReadMqttConf:
    def __init__(self, config_file):
        config = configparser.ConfigParser()
        self.host = None
        self.port = None
        self.username = None
        self.password = None

        # Read the configuration file
        try:
            file_read = config.read(config_file)
            if not file_read:
                raise FileNotFoundError("Configuration file not found: {}".format(config_file))
        except Exception as e:
            print("Error reading configuration file: {}".format(e))

        # Extract MQTT broker parameters from the configuration file
        # These variables are acessible from the class instance
        try:
            self.host = config["broker"]["host"]
            self.port = int(config["broker"]["port"])
            self.username = config["broker"]["username"]
            self.password = config["broker"]["password"]
        except Exception as e:
            print("Error extracting MQTT broker parameters: {}".format(e))