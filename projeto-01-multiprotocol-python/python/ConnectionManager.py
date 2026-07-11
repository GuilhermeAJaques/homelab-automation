from Field_protocols.s7_nonOptimized import S7_NonOptimized
from Field_protocols.OPC_UA import OPC_UA
from Field_protocols.ModbusTCP import ModbusTCP
from Field_protocols.EthernetIP import EthernetIP
from gpio_driver import GPIO_driver
import configparser
import os
import csv
from enum import IntEnum

class DriverType(IntEnum):
    GPIO = 0
    S7 = 1
    ETHERNET_IP = 2
    MODBUS_TCP = 3
    OPC_UA = 4

class ConnectionManager:
    def __init__(self):

        base_path = "connections/"

        self.connections = []
        self.subTopics = set()

        for folder in sorted(os.listdir(base_path)):
            if folder.startswith("Connection_"):
                configFile = configparser.ConfigParser()
                variables = []
                parameters = {}
                connection_path = os.path.join(base_path, folder)
       
                # Read the configuration file
                try:
                    # Read config file
                    file_read = configFile.read(connection_path + "/driver.txt")
                    if not file_read:
                        raise FileNotFoundError("Configuration file not found: {}".format(connection_path))
                    
                except Exception as e:
                    print("Error reading configuration file: {}".format(e))

                driver = self.__getDriverType(configFile)

                if driver is None:
                    print(f"Skipping {folder}: invalid driver configuration")
                    continue

                match (driver):
                    case DriverType.GPIO: # GPIO driver
                        # Get parameters
                        parameters = {} # No parameters required
                        driver_instance = GPIO_driver()

                        # Get variables
                        try:
                            with open(connection_path + "/variables.csv") as file:
                                reader = csv.reader(file, delimiter=';')
                                for row in reader:
                                    variable = {
                                        "Name": str(row[0]),
                                        "GPIO": int(row[0]),
                                        "Topic": row[1],
                                        "Access": row[2]
                                    }
                                    variables.append(variable)

                                    if (variable["Access"] == "w"):
                                        self.subTopics.add(variable["Topic"])
                        except Exception as e:
                            print("Error reading variables file: {}".format(e))


                    case DriverType.S7: # S7
                        # Get parameters
                        parameters = {
                            "ip": configFile["Siemens S7"]["ip"],
                            "rack": configFile["Siemens S7"]["rack"],
                            "slot": configFile["Siemens S7"]["slot"]
                        }
                        driver_instance = S7_NonOptimized(ip=parameters["ip"], 
                                                          rack=int(parameters["rack"]),
                                                          slot=int(parameters["slot"]))

                        # Get variables
                        try:
                            with open(connection_path + "/variables.csv") as file:
                                reader = csv.reader(file, delimiter=';')
                                for row in reader:
                                    variable = {
                                        "DB": int(row[0]),
                                        "Name": row[1],
                                        "Type": row[2],
                                        "Offset": row[3],
                                        "Topic": row[4],
                                        "Access": row[5]
                                    }
                                    variables.append(variable)

                                    if (variable["Access"] == "w"):
                                        self.subTopics.add(variable["Topic"])
                        except Exception as e:
                            print("Error reading variables file: {}".format(e))


                    case DriverType.ETHERNET_IP: # Ethernet/IP
                        # Get parameters
                        parameters = {
                            "ip": configFile["Rockwell Ethernet/IP"]["ip"]
                        }
                        driver_instance = EthernetIP(ip=parameters["ip"])

                        # Get variables
                        try:
                            with open(connection_path + "/variables.csv") as file:
                                reader = csv.reader(file, delimiter=';')
                                for row in reader:
                                    variable = {
                                        "Name": row[0],
                                        "Topic": row[1],
                                        "Access": row[2]
                                    }
                                    variables.append(variable)

                                    if (variable["Access"] == "w"):
                                        self.subTopics.add(variable["Topic"])
                        except Exception as e:
                            print("Error reading variables file: {}".format(e))


                    case DriverType.MODBUS_TCP: # Modbus TCP
                        # Get parameters
                        parameters = {
                            "ip": configFile["Modbus TCP"]["ip"],
                            "port": configFile["Modbus TCP"]["port"]
                        }
                        driver_instance = ModbusTCP(host=parameters["ip"],
                                                    port=int(parameters["port"]))

                        # Get variables
                        try:
                            with open(connection_path + "/variables.csv") as file:
                                reader = csv.reader(file, delimiter=';')
                                for row in reader:
                                    variable = {
                                        "Name": row[0],
                                        "Address": row[1],
                                        "Type": row[2],
                                        "Topic": row[3],
                                        "Access": row[4]
                                    }
                                    variables.append(variable)

                                    if (variable["Access"] == "w"):
                                        self.subTopics.add(variable["Topic"])
                        except Exception as e:
                            print("Error reading variables file: {}".format(e))


                    case DriverType.OPC_UA: # OPC-UA
                        # Get parameters
                        parameters = {
                            "url": configFile["OPC-UA"]["url"]
                        }
                        driver_instance = OPC_UA(endpoint=parameters["url"])

                        # Get variables
                        try:
                            with open(connection_path + "/variables.csv") as file:
                                reader = csv.reader(file, delimiter=';')
                                for row in reader:
                                    variable = {
                                        "Name": row[0],
                                        "Topic": row[1],
                                        "Access": row[2]
                                    }
                                    variables.append(variable)

                                    if (variable["Access"] == "w"):
                                        self.subTopics.add(variable["Topic"])
                        except Exception as e:
                            print("Error reading variables file: {}".format(e))
                
                # Build connection object
                connection = {
                    "Driver": driver,
                    "driver_instance": driver_instance,
                    "Parameters": parameters,
                    "Variables": variables
                }

                self.connections.append(connection)

    def connect_all(self):
        for connection in self.connections:
            connection["driver_instance"].connect()

    def disconnect_all(self):
        for connection in self.connections:
            connection["driver_instance"].disconnect()

    def read_variables_all(self):
        variables = []

        # Pass by all connections
        for connection in self.connections:
            driver = connection["driver_instance"]
            value = 0
            match (connection["Driver"]):
                case DriverType.GPIO: # GPIO
                    for variable in connection["Variables"]:
                        if (variable["Access"] == "r"):
                            value = driver.read_variable(variable["GPIO"])
                            
                            # Build variable struct
                            result = {
                                "Name": variable["Name"],
                                "Value": value,
                                "Topic": variable["Topic"]
                            }
                            variables.append(result)
                case DriverType.S7: # S7
                    for variable in connection["Variables"]:
                        if (variable["Access"] == "r"):
                            value = driver.read_variable(variable["DB"], 
                                                        variable["Offset"], 
                                                        variable["Type"])
                            
                            # Build variable struct
                            result = {
                                "Name": variable["Name"],
                                "Value": value,
                                "Topic": variable["Topic"]
                            }
                            variables.append(result)
                        
                case DriverType.ETHERNET_IP: # Ethernet/IP
                    for variable in connection["Variables"]:
                        if (variable["Access"] == "r"):
                            value = driver.read_variable(variable["Name"])
                            
                            # Build variable struct
                            result = {
                                "Name": variable["Name"],
                                "Value": value,
                                "Topic": variable["Topic"]
                            }
                            variables.append(result)

                case DriverType.MODBUS_TCP: # Modbus TCP
                    for variable in connection["Variables"]:
                        if (variable["Access"] == "r"):
                            value = driver.read_variable(variable["Address"], 
                                                        variable["Type"])
                            
                            # Build variable struct
                            result = {
                                "Name": variable["Name"],
                                "Value": value,
                                "Topic": variable["Topic"]
                            }
                            variables.append(result)

                case DriverType.OPC_UA: # OPC-UA
                    for variable in connection["Variables"]:
                        if (variable["Access"] == "r"):
                            value = driver.read_variable(variable["Name"])
                            
                            # Build variable struct
                            result = {
                                "Name": variable["Name"],
                                "Value": value,
                                "Topic": variable["Topic"]
                            }
                            variables.append(result)
        
        return variables
    
    def write_variable(self, topic, value):
        try:
            for connection in self.connections:
                driver = connection["driver_instance"]
                for variable in connection["Variables"]:
                    if (variable["Topic"] == topic):
                        match (connection["Driver"]):
                            case DriverType.GPIO: # GPIO
                                driver.write_variable(variable["GPIO"],
                                                      value)
                            case DriverType.S7: # S7
                                driver.write_variable(variable["DB"], 
                                                      variable["Offset"], 
                                                      variable["Type"],
                                                      value)
                            case DriverType.ETHERNET_IP: # Ether
                                driver.write_variable(variable["Name"],
                                                      value)
                            case DriverType.MODBUS_TCP: # Modbus TCP
                                driver.write_variable(variable["Address"], 
                                                      variable["Type"],
                                                      value)
                            case DriverType.OPC_UA: # OPC-UA
                                driver.write_variable(variable["Name"],
                                                      value)
        
        except Exception as e:
            print("Error writing variables: {}".format(e))
    

    def __getDriverType(self, file):
        try:
            return int(file["Settings"]["driver"])
        except Exception as e:
            print("Error extracting driver parameters: {}".format(e))
        
        return None
    
    def is_writable(self, topic):
        for connection in self.connections:
            for variable in connection["Variables"]:
                if variable["Topic"] == topic:
                    return variable["Access"] == "w"
        return False