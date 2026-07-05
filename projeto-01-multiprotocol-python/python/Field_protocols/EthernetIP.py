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

    def read_variable(self, variableName):
        try:
            if not self.connected:
                print(f"Not connected")
                return None
            # Read variable values
            result = self.client.read(variableName)
            if result.error:
                print(f"Error reading {variableName}: {result.error}")
                return None
            value = result.value
            return value
        
        except Exception as e:
            print(f"Error reading variable {variableName} from Ethernet/IP: {e}")