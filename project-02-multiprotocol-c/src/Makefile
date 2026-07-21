CC = gcc
CFLAGS = -Wall -I. -std=gnu99 -fcommon
LDFLAGS = -lpaho-mqtt3c -lpthread -lmodbus -lplctag -lsnap7 -Wl,--allow-multiple-definition

SOURCES = main.c \
          MQTT/mqtt_client.c \
          Field_protocols/Modbus/modbus_client.c \
          Field_protocols/EthernetIP/ethernet_client.c \
          Field_protocols/S7/s7_client.c \
          generalFunctions/config_reader/config_reader.c \
          generalFunctions/string_functions/string_functions.c

TARGET = IoT_C

all:
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)