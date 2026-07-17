CC = gcc
CFLAGS = -Wall
LDFLAGS = -lpaho-mqtt3c

SOURCES = main.c MQTT/mqtt_client.c config_reader/config_reader.c
TARGET = IoT_C

all:
	$(CC) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
