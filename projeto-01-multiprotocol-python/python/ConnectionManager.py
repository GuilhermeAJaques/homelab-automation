from Field_protocols.s7_nonOptimized import S7_NonOptimized
from Field_protocols.OPC_UA import OPC_UA
from Field_protocols.ModbusTCP import ModbusTCP
from Field_protocols.EthernetIP import EthernetIP
import configparser
import os
import csv



class ConnectionManager:
    def __init__(self):

        base_path = "connections/"

        self.connections = []

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
                    case 0: # S7
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
                                reader = csv.reader(file)
                                for row in reader:
                                    variable = {
                                        "DB": int(row[0]),
                                        "Name": row[1],
                                        "Type": row[2],
                                        "Offset": row[3],
                                        "Topic": row[4]
                                    }
                                    variables.append(variable)
                        except Exception as e:
                            print("Error reading variables file: {}".format(e))


                    case 1: # Ethernet/IP
                        # Get parameters
                        parameters = {
                            "ip": configFile["Rockwell Ethernet/IP"]["ip"]
                        }
                        driver_instance = EthernetIP(ip=parameters["ip"])

                        # Get variables
                        try:
                            with open(connection_path + "/variables.csv") as file:
                                reader = csv.reader(file)
                                for row in reader:
                                    variable = {
                                        "Name": row[0],
                                        "Topic": row[1]
                                    }
                                    variables.append(variable)
                        except Exception as e:
                            print("Error reading variables file: {}".format(e))


                    case 2: # Modbus TCP
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
                                reader = csv.reader(file)
                                for row in reader:
                                    variable = {
                                        "Name": row[0],
                                        "Address": row[1],
                                        "Type": row[2],
                                        "Topic": row[3]
                                    }
                                    variables.append(variable)
                        except Exception as e:
                            print("Error reading variables file: {}".format(e))


                    case 3: # OPC-UA
                        # Get parameters
                        parameters = {
                            "url": configFile["OPC-UA"]["url"]
                        }
                        driver_instance = OPC_UA(endpoint=parameters["url"])

                        # Get variables
                        try:
                            with open(connection_path + "/variables.csv") as file:
                                reader = csv.reader(file)
                                for row in reader:
                                    variable = {
                                        "Name": row[0],
                                        "Topic": row[1]
                                    }
                                    variables.append(variable)
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
                case 0: # S7
                    for variable in connection["Variables"]:
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
                        
                case 1: # Ethernet/IP
                    for variable in connection["Variables"]:
                        value = driver.read_variable(variable["Name"])
                        
                        # Build variable struct
                        result = {
                            "Name": variable["Name"],
                            "Value": value,
                            "Topic": variable["Topic"]
                        }
                        variables.append(result)

                case 2: # Modbus TCP
                    for variable in connection["Variables"]:
                        value = driver.read_variable(variable["Address"], 
                                                     variable["Type"])
                        
                        # Build variable struct
                        result = {
                            "Name": variable["Name"],
                            "Value": value,
                            "Topic": variable["Topic"]
                        }
                        variables.append(result)

                case 3: # OPC-UA
                    for variable in connection["Variables"]:
                        value = driver.read_variable(variable["Name"])
                        
                        # Build variable struct
                        result = {
                            "Name": variable["Name"],
                            "Value": value,
                            "Topic": variable["Topic"]
                        }
                        variables.append(result)
        
        return variables
    

    def __getDriverType(self, file):
        try:
            return int(file["Settings"]["driver"])
        except Exception as e:
            print("Error extracting driver parameters: {}".format(e))
        
        return None