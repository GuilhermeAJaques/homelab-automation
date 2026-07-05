import snap7

class S7_NonOptimized:
    def __init__(self, ip, rack, slot):
        # Initialize the S7 client and connect to the PLC
        self.ip = ip
        self.rack = rack
        self.slot = slot
        self.client = snap7.client.Client()
        self.connected = False

    def connect(self):
        try:
            # Connect to the PLC
            self.client.connect(self.ip, self.rack, self.slot)
            if not self.client.get_connected():
                print("Failed to connect to S7 PLC.")
            else:
                print(f"Connected to S7 PLC for PLC: {self.ip}")
                self.connected = True
        except Exception as e:
            print(f"Error connecting to S7 PLC: {e}")

    def disconnect(self):
        try:
            # Disconnect from the PLC
            self.client.disconnect()
            self.connected = False
        except Exception as e:
            print(f"Error disconnecting from S7 PLC: {e}")

    def read_variable(self, db_number, offset, datatype):
        try:
            # Check if connected to the PLC
            if not self.connected:
                print("Not connected to S7 PLC.")
                return None
            
            # Get the start address
            if '.' in offset: # If user enter the offset like TIA e.g. 10.0, 45.3
                if datatype.lower() == 'bool':
                    # Bool must be read the bit number
                    start_address = int(offset.split('.')[0])   
                    bit_offset = int(offset.split('.')[1])
                else:
                    start_address = int(offset.split('.')[0])   
                    bit_offset = 0
            else:
                start_address = int(offset)
                bit_offset = 0

            # Read data from PLC
            data = self.client.db_read(db_number, start_address, self.__getDTsize(datatype))

            if datatype.lower() == 'bool':
                # bool must be read the bit number
                value = snap7.util.get_bool(data, 0, bit_offset)
            else:
                value = self.__convert_data(data, datatype)

            return value
        
        except Exception as e:
            print(f"Error reading data from S7 PLC: {e}")
            return None
        
    def __getDTsize(self, datatype):
        try:
            match datatype.lower():
                case 'bool':
                    size = 1
                case 'byte':
                    size = 1
                case 'word':
                    size = 2
                case 'dword':
                    size = 4
                case 'usint':
                    size = 1
                case 'sint':
                    size = 1
                case 'uint':
                    size = 2
                case 'int':
                    size = 2
                case 'udint':
                    size = 4
                case 'dint':
                    size = 4
                case 'real':
                    size = 4
                case 'lreal':
                    size = 8
                case 'string':
                    size = 256  # Assuming a maximum string length of 256 characters
                case _:
                    # Before to return error, check if is not string with length defined
                    if datatype.lower().startswith('string[') and datatype.endswith(']'):
                        try:
                            length = int(datatype[7:-1])
                            size = length + 2  # Add 2 bytes for the string length prefix
                        except ValueError:
                            print(f"Invalid string length specified: {datatype}")
                            return None
                    else:
                        print(f"Unsupported datatype: {datatype}")
                        return None
                
            # Return the size of the datatype
            return size
        

        except Exception as e:
            print(f"Error determining size for datatype {datatype}: {e}")
            return None
    
    def __convert_data(self, data, datatype):
        try:
            match datatype.lower():
                case 'byte':
                    return snap7.util.get_byte(data, 0)
                case 'word':
                    return snap7.util.get_word(data, 0)
                case 'dword':
                    return snap7.util.get_dword(data, 0)
                case 'usint':
                    return snap7.util.get_usint(data, 0)
                case 'sint':
                    return snap7.util.get_sint(data, 0)
                case 'uint':
                    return snap7.util.get_uint(data, 0)
                case 'int':
                    return snap7.util.get_int(data, 0)
                case 'udint':
                    return snap7.util.get_udint(data, 0)
                case 'dint':
                    return snap7.util.get_dint(data, 0)
                case 'real':
                    return snap7.util.get_real(data, 0)
                case 'lreal':
                    return snap7.util.get_lreal(data, 0)
                case 'string':
                    return snap7.util.get_string(data, 0)
                case _:
                    # Before to return error, check if is not string with length defined
                    if datatype.lower().startswith('string[') and datatype.endswith(']'):
                        try:
                            return snap7.util.get_string(data, 0)
                        except ValueError:
                            print(f"Invalid string length specified: {datatype}")
                            return None
                    else:
                        print(f"Unsupported datatype: {datatype}")
                        return None
        
        except Exception as e:
            print(f"Error converting data for datatype {datatype}: {e}")
            return None