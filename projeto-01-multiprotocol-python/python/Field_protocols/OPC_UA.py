from opcua import Client

class OPC_UA:
    def __init__(self, endpoint):

        # Get server url
        self.endpoint = endpoint
        self._node_cache = {} # start empty cache for database names

        # Create client opcua instance
        self.client = Client(url=self.endpoint)
        self.connected = False
        
    def connect(self):
        try:
            # Connect to the OPC UA server
            self.client.connect()
            self.connected = True
            print(f"Connected to OPC UA server at {self.endpoint}")

        except Exception as e:
            print(f"Error connecting to OPC UA server: {e}")

    def disconnect(self):
        try:
            # Disconnect from the OPC UA server
            self.client.disconnect()
            self.connected = False
            print(f"Disconnected from OPC UA server at {self.endpoint}")
        except Exception as e:
            print(f"Error disconnecting from OPC UA server: {e}")

    def read_variable(self, variableName):
        try:
            if not self.connected:
                print("Not connected to OPC UA server.")
                return None
            
            if variableName in self._node_cache:
                node = self._node_cache[variableName]
            else:
                root = self.client.get_root_node()
                node = self.__find_node(root, variableName, 0)
                if node is not None:
                    self._node_cache[variableName] = node
                else:
                    print(f"Variable {variableName} not found in OPC UA server.")
                    return None
            value = node.get_value()
            return value
        
        except Exception as e:
            print(f"Error reading variable {variableName} from OPC UA server: {e}")
            return None
        
    def __find_node(self, parent, var_name, deepCount):
        for child in parent.get_children():
            tmpDeepCount = deepCount + 1

            # Threshold to avoid infinite recursion 
            if (tmpDeepCount > 20):  
                print(f"Reached maximum depth while searching for variable {var_name}.")
                return None

            # Check if the child's browse name matches the variable name
            if child.get_browse_name().Name == var_name:
                return child  # return the node if found
            
            # Recursively search in the child's children
            children = child.get_children()
            if len(children) > 0:
                result = self.__find_node(child, var_name, tmpDeepCount )
                if result is not None:
                    return result
        
        return None