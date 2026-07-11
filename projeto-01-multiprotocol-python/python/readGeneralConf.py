import configparser

class ReadGeneralConf:
    def __init__(self, config_file):
        config = configparser.ConfigParser()
        self.cycle = None

        # Read the configuration file
        try:
            file_read = config.read(config_file)
            if not file_read:
                raise FileNotFoundError("Configuration file not found: {}".format(config_file))
        except Exception as e:
            print("Error reading configuration file: {}".format(e))

        # Extract General parameters from the configuration file
        # These variables are acessible from the class instance
        try:
            self.cycle = (float(config["General parameters"]["scanCycle"]) / 1000)
        except Exception as e:
            print("Error extracting General parameters: {}".format(e))