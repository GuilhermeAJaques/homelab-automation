from pymodbus.client.sync import ModbusTcpClient
from pymodbus.payload import BinaryPayloadDecoder
from pymodbus.payload import BinaryPayloadBuilder
from pymodbus.constants import Endian
import math

class ModbusTCP:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.client = ModbusTcpClient(host=self.host, port=self.port)
        self.connected = False

    def connect(self):
        try:
            self.connected = self.client.connect()
            if self.connected:
                print(f"Connected to Modbus TCP server at {self.host}:{self.port}")
            else:
                print(f"Failed to connect to Modbus TCP server at {self.host}:{self.port}")
        except Exception as e:
            print(f"Error connecting to Modbus TCP server: {e}")

    def disconnect(self):
        try:
            self.client.close()
            self.connected = False
            print(f"Disconnected from Modbus TCP server at {self.host}:{self.port}")
        except Exception as e:
            print(f"Error disconnecting from Modbus TCP server: {e}")

    def __reconnect(self):
        try:
            self.client.close()
            self.connected = self.client.connect()
            if self.connected:
                print(f"Reconnected to Modbus TCP server at {self.host}:{self.port}")
        except Exception as e:
            print(f"Error reconnecting to Modbus TCP server: {e}")

    def read_variable(self, fullAddress, datatype):
        try:
            if not self.connected:
                print("Not connecdted to Modbus TCP server.")
                return None
            
            fc, address = self.__getAddressAndReadType(fullAddress, 'r')

            match (fc):
                case 1:
                    response = self.client.read_coils(address, self.__getDTsize(datatype))
                    if response.isError():
                        print(f"Error reading coils at address {address}: {response}")
                        self.__reconnect()
                        return None
                    
                    return response.bits[0]
                
                case 2:
                    response = self.client.read_discrete_inputs(address, self.__getDTsize(datatype))
                    if response.isError():
                        print(f"Error reading discrete inputs at address {address}: {response}")
                        self.__reconnect()
                        return None
                    
                    return response.bits[0]

                case 3:
                    response = self.client.read_holding_registers(address, self.__getDTsize(datatype))
                    if response.isError():
                        print(f"Error reading holding registers at address {address}: {response}")
                        self.__reconnect()
                        return None
                    
                    return self.__convert_registers(response.registers, datatype)
                
                case 4:
                    response = self.client.read_input_registers(address, self.__getDTsize(datatype))
                    if response.isError():
                        print(f"Error reading input registers at address {address}: {response}")
                        self.__reconnect()
                        return None
                    
                    return self.__convert_registers(response.registers, datatype)
            
        except Exception as e:
            print(f"Error reading ModbusTCP at address {fullAddress}: {e}")
            return None

    def write_variable(self, fullAddress, datatype, value):
        try:
            if not self.connected:
                print("Not connecdted to Modbus TCP server.")
                return None
            
            fc, address = self.__getAddressAndReadType(fullAddress, 'w')

            match (fc):
                case 5:
                    self.client.write_coil(address, bool(value))
                case 6:
                    self.client.write_register(address, self.__convert_value(datatype, value)[0])
                case 16:
                    self.client.write_registers(address, self.__convert_value(datatype, value))
                    
        except Exception as e:
            print(f"Error reading ModbusTCP at address {fullAddress}: {e}")
            return None
        
    def __getDTsize(self, datatype):
        try:
            match datatype.lower():
                case 'bool':
                    size = 1
                case 'byte':
                    size = 1
                case 'word':
                    size = 1
                case 'dword':
                    size = 2
                case 'usint':
                    size = 1
                case 'sint':
                    size = 1
                case 'uint':
                    size = 1
                case 'int':
                    size = 1
                case 'udint':
                    size = 2
                case 'dint':
                    size = 2
                case 'real':
                    size = 2 
                case 'lreal':
                    size = 4
                case 'string':
                    size = 40  # Assuming a maximum string length of 256 characters
                case _:
                    # Before to return error, check if is not string with length defined
                    if datatype.lower().startswith('string[') and datatype.endswith(']'):
                        try:
                            length = int(datatype[7:-1])
                            size = math.ceil(length / 2)
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
        
    def __convert_registers(self, register, datatype):
        decoder = BinaryPayloadDecoder.fromRegisters(register, 
                                                     byteorder=Endian.Big,  
                                                     wordorder=Endian.Little)

        try:
            match datatype.lower():
                case 'bool':
                    return decoder.decode_bits()
                case 'byte':
                    return decoder.decode_8bit_int()
                case 'word':
                    return decoder.decode_16bit_int()
                case 'dword':
                    return decoder.decode_32bit_int()
                case 'usint':
                    return decoder.decode_8bit_int()
                case 'sint':
                    return decoder.decode_8bit_int()
                case 'uint':
                    return decoder.decode_16bit_int()
                case 'int':
                    return decoder.decode_16bit_int()
                case 'udint':
                    return decoder.decode_32bit_int()
                case 'dint':
                    return decoder.decode_32bit_int()
                case 'real':
                    return decoder.decode_32bit_float()
                case 'lreal':
                    return decoder.decode_64bit_float()
                case 'string':
                    return self.__organizeStringForRead(register)
                case _:
                    # Before to return error, check if is not string with length defined
                    if datatype.lower().startswith('string[') and datatype.endswith(']'):
                        try:
                            return self.__organizeStringForRead(register)
                        except ValueError:
                            print(f"Invalid string length specified: {datatype}")
                            return None
                    else:
                        print(f"Unsupported datatype: {datatype}")
                        return None
        
        except Exception as e:
            print(f"Error converting data for datatype {datatype}: {e}")
            return None

    def __convert_value(self, datatype, value):
        builder = BinaryPayloadBuilder(byteorder=Endian.Big, wordorder=Endian.Little)

        try:
            match datatype.lower():
                case 'byte':
                    builder.add_8bit_int(value)
                    return builder.to_registers()
                case 'word':
                    builder.add_16bit_int(value)
                    return builder.to_registers()
                case 'dword':
                    builder.add_32bit_int(value)
                    return builder.to_registers()
                case 'usint':
                    builder.add_8bit_int(value)
                    return builder.to_registers()
                case 'sint':
                    builder.add_8bit_int(value)
                    return builder.to_registers()
                case 'uint':
                    builder.add_16bit_int(value)
                    return builder.to_registers()
                case 'int':
                    builder.add_16bit_int(value)
                    return builder.to_registers()
                case 'udint':
                    builder.add_32bit_int(value)
                    return builder.to_registers()
                case 'dint':
                    builder.add_32bit_int(value)
                    return builder.to_registers()
                case 'real':
                    builder.add_32bit_float(value)
                    return builder.to_registers()
                case 'lreal':
                    builder.add_64bit_float(value)
                    return builder.to_registers()
                case 'string':
                    return self.__organizeStringForWrite(value, self.__getDTsize(datatype))
                case _:
                    # Before to return error, check if is not string with length defined
                    if datatype.lower().startswith('string[') and datatype.endswith(']'):
                        try:
                            return self.__organizeStringForWrite(value, self.__getDTsize(datatype))
                        except ValueError:
                            print(f"Invalid string length specified: {datatype}")
                            return None
                    else:
                        print(f"Unsupported datatype: {datatype}")
                        return None
        
        except Exception as e:
            print(f"Error converting data for datatype {datatype}: {e}")
            return None

    def __getAddressAndReadType(self, fullAddress, access):
        prefix = fullAddress[:3].lower()
        if (access == "r"):
            if prefix == '%mx' or prefix == '%qx':
                fc = 1
            elif prefix == '%ix':
                fc = 2
            elif prefix in ['%mw', '%qw', '%md', '%mb', '%qb', '%qd']:
                fc = 3
            elif prefix in ['%iw', '%id', '%ib']:
                fc = 4
            else:
                print(f"Unknown address prefix: {prefix}")
                return None, None
        else:
            if prefix in ['%mx', '%qx', '%ix']:
                fc = 5
            elif prefix in ['%mw', '%qw', '%iw']:
                fc = 6
            elif prefix in ['%md', '%qd', '%mb', '%qb', '%id', '%ib']:
                fc = 16
            else:
                print(f"Unknown address prefix: {prefix}")
                return None, None
        
        address = fullAddress[3:]

        if '.' in address:
            parts = address.split('.')
            byte = int(parts[0])
            bit = int(parts[1])

            # Invert bytes inside word
            if (byte % 2 == 0):
                byte += 1
            else:
                byte -= 1

            return [fc, ((byte * 8) + (bit))]
        
        if prefix in ['%md', '%qd', '%id']:
            address = int(address) * 2
        elif prefix in ['%mb', '%qb']:
            address = int(address) // 2

        return [fc, int(address)]
    
    def __organizeStringForRead(self, decoder):
        raw_bytes = b''
        for reg in decoder:
            raw_bytes += reg.to_bytes(2, byteorder='big')
        
        # Swap bytes within each word
        swapped = b''
        for i in range(0, len(raw_bytes), 2):
            swapped += raw_bytes[i+1:i+2] + raw_bytes[i:i+1]
        
        if b'\x00' in swapped:
            swapped = swapped[:swapped.index(b'\x00')]
    
        return swapped.decode('ascii', errors='ignore').rstrip()
        
    def __organizeStringForWrite(self, value, max_length):
        # Encode string to bytes
        raw_bytes = value.encode('ascii', errors='ignore')

        # Fill the rest of the bytes with 0
        raw_bytes = raw_bytes[:max_length]
        raw_bytes = raw_bytes.ljust(max_length, b'\x00')
        
        # Pad to even length
        if len(raw_bytes) % 2 != 0:
            raw_bytes += b'\x00'
        
        # Swap bytes within each word (inverse of __organizeString)
        swapped = b''
        for i in range(0, len(raw_bytes), 2):
            swapped += raw_bytes[i+1:i+2] + raw_bytes[i:i+1]
        
        # Convert to list of registers
        registers = []
        for i in range(0, len(swapped), 2):
            registers.append(int.from_bytes(swapped[i:i+2], byteorder='big'))
        
        return registers