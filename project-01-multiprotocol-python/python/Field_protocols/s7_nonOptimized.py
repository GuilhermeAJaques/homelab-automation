import snap7
from Logger import Logger, Criticality, Class

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
            self.client.connect(self.ip, self.rack, self.slot)
            if not self.client.get_connected():
                Logger.log(Class.S7, Criticality.ERROR ,"Failed to connect to S7 PLC.")
                self.connected = False
            else:
                Logger.log(Class.S7, Criticality.INFO ,f"Connected to S7 PLC for PLC: {self.ip}")
                self.connected = True
        except Exception as e:
            Logger.log(Class.S7, Criticality.ERROR ,f"Error connecting to S7 PLC: {e}")
            self.connected = False

    def disconnect(self):
        try:
            # Disconnect from the PLC
            self.client.disconnect()
            self.connected = False
        except Exception as e:
            Logger.log(Class.S7, Criticality.ERROR ,f"Error disconnecting from S7 PLC: {e}")

    def read_variable(self, db_number, offset, datatype):
        try:
            # Check if connected to the PLC
            if not self.connected:
                return None
            
            # Reconection
            if not self.client.get_connected():
                Logger.log(Class.S7, Criticality.WARNING ,"Trying to read variable without connected, start reconnection")
                self.connect()
            
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
                value = self.__convert_to_value(data, datatype)

            return value
        
        except Exception as e:
            Logger.log(Class.S7, Criticality.ERROR ,f"Error reading data from S7 PLC: {e}")
            return None
        
    def write_variable(self, db_number, offset, datatype, value):
        try:
            # Check if connected to the PLC
            if not self.connected:
                return None
            
            # Reconection
            if not self.client.get_connected():
                Logger.log(Class.S7, Criticality.WARNING ,"Trying to read variable without connected, start reconnection")
                self.connect()
            
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

            
            if datatype.lower() == 'bool':
                data = self.client.db_read(db_number, start_address, 1)
                snap7.util.set_bool(data, 0, bit_offset, value)
                self.client.db_write(db_number, start_address, data)
            else:
                self.client.db_write(db_number, 
                                 start_address, 
                                 self.__convert_to_data(bytearray(self.__getDTsize(datatype)), 
                                                        datatype, 
                                                        value))

        except Exception as e:
            Logger.log(Class.S7, Criticality.ERROR ,f"Error writing data from S7 PLC: {e}")

        
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
                            Logger.log(Class.S7, Criticality.WARNING ,f"Invalid string length specified: {datatype}")
                            return None
                    else:
                        Logger.log(Class.S7, Criticality.WARNING ,f"Unsupported datatype: {datatype}")
                        return None
                
            # Return the size of the datatype
            return size
        

        except Exception as e:
            Logger.log(Class.S7, Criticality.ERROR ,f"Error determining size for datatype {datatype}: {e}")
            return None
    
    def __convert_to_value(self, data, datatype):
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
                            Logger.log(Class.S7, Criticality.WARNING ,f"Invalid string length specified: {datatype}")
                            return None
                    else:
                        Logger.log(Class.S7, Criticality.WARNING ,f"Unsupported datatype: {datatype}")
                        return None
        
        except Exception as e:
            Logger.log(Class.S7, Criticality.ERROR ,f"Error converting data for datatype {datatype}: {e}")
            return None
    
    def __convert_to_data(self, data, datatype, value):
        try:
            match datatype.lower():
                case 'byte':
                    snap7.util.set_byte(data, 0, value)
                    return data
                case 'word':
                    snap7.util.set_word(data, 0, value)
                    return data
                case 'dword':
                    snap7.util.set_dword(data, 0, value)
                    return data
                case 'usint':
                    snap7.util.set_usint(data, 0, value)
                    return data
                case 'sint':
                    snap7.util.set_sint(data, 0, value)
                    return data
                case 'uint':
                    snap7.util.set_uint(data, 0, value)
                    return data
                case 'int':
                    snap7.util.set_int(data, 0, value)
                    return data
                case 'udint':
                    snap7.util.set_udint(data, 0, value)
                    return data
                case 'dint':
                    snap7.util.set_dint(data, 0, value)
                    return data
                case 'real':
                    snap7.util.set_real(data, 0, value)
                    return data
                case 'lreal':
                    snap7.util.set_lreal(data, 0, value)
                    return data
                case 'string':
                    data = bytearray(self.__getDTsize(datatype))
                    snap7.util.set_string(data, 0, value, 254)
                    return data
                case _:
                    # Before to return error, check if is not string with length defined
                    if datatype.lower().startswith('string[') and datatype.endswith(']'):
                        try:
                            data = bytearray(self.__getDTsize(datatype))
                            snap7.util.set_string(data, 0, value, 254)
                            return data
                        except ValueError:
                            Logger.log(Class.S7, Criticality.WARNING ,f"Invalid string length specified: {datatype}")
                            return None
                    else:
                        Logger.log(Class.S7, Criticality.WARNING ,f"Unsupported datatype: {datatype}")
                        return None
        
        except Exception as e:
            Logger.log(Class.S7, Criticality.ERROR ,f"Error converting data for datatype {datatype}: {e}")
            return None