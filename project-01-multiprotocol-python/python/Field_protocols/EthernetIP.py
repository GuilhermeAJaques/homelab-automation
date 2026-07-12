from pycomm3 import LogixDriver

class EthernetIP:
    def __init__(self, ip):
        # Initialize start variables
        self.ip = ip
        self.client = LogixDriver(ip)
        self.connected = False

    def connect(self):
        try:
            # Connect to Ethernet/IP server
            self.client.open()
            self.connected = True
            print(f"Connected to EtherNet/IP at {self.ip}")
        except Exception as e:
            print(f"Error connecting to EtherNet/IP: {e}")
 
    def disconnect(self):
        try:
            # Disconect from server
            self.client.close()
            self.connected = False
            print(f"Disconnected from EtherNet/IP at {self.ip}")
        except Exception as e:
            print(f"Error disconnecting from EtherNet/IP: {e}")

    def __reconnect(self):
        try:
            self.client.close()
            self.client.open()
            self.connected = True
            print(f"Reconnected to EtherNet/IP at {self.ip}")
        except Exception as e:
            self.connected = False
            print(f"Error reconnecting to EtherNet/IP: {e}")

    def read_variable(self, variableName):
        try:
            if not self.connected:
                print(f"Not connected")
                self.__reconnect()
                return None
            # Read variable values
            result = self.client.read(variableName)
            if result.error:
                print(f"Error reading {variableName}: {result.error}")
                self.__reconnect()
                return None
            value = result.value
            return value
        
        except Exception as e:
            print(f"Error reading variable {variableName} from Ethernet/IP: {e}")
            self.__reconnect()

    def write_variable(self, variableName, value):
        try:
            if not self.connected:
                print(f"Not connected")
                self.__reconnect()
            # Write variable values
            result = self.client.write((variableName, value))
            if result.error:
                print(f"Error wrinting {variableName}: {result.error}")
                self.__reconnect()
        
        except Exception as e:
            print(f"Error wrinting variable {variableName} from Ethernet/IP: {e}")
            self.__reconnect()