# Multi protocol IoT with python

This project is a multiprotocol IoT to collect data from field and send to a higher level IT systems.

## Architecture

*   Field drivers: Embed IO (just digital), Siemens S7 connection, Rockwell Ethernet/IP, Codesys Modbus TCP, OPC-UA. Could be connected many connections at same device.
*   Connections to higher level: MQTT (main connection, always must use), and REST API connection

## General Parameters

The device cycle scan time can be configured at /python/generalConf.txt

## Connections methods

### OT drivers

A new folder on /python/connections/ must be created with name Connection\_N (N is the number of driver), with two files inside driver.txt and variables.csv, inside the folder already has some examples how to configure each driver

#### GPIO

##### Driver.txt

Select driver number with 0

*   chip: GPIO full path name (For Raspbarry PI 3 Model B+ is /dev/gpiochip0)

##### Variable.csv

*   Column A/1: GPIO number
*   Column B/2: MQTT topic
*   Column C/3: Access mode. w: Only write/Digital output, r: Only read/Digital input. (It's case sensitive)

#### Siemens S7 connection

##### Driver.txt

Select driver number with 1

*   IP: Module IP
*   rack: Rack number (normally is 0)
*   slot: Slot number (normally is 1)

##### Variable.csv

*   Column A/1: DB number
*   Column B/2: Variable name (not used for connection just to address on MQTT)
*   Column C/3: Datatype (It's not case sensitive)
*   Column D/4: Offset
*   Column E/5: MQTT topic
*   Column F/6: Access mode. w: Only write, r: Only read. (It's case sensitive)

Could copy the field direct from DB, and remember to mark as Non optimized data block.

#### Rockwell Ethernet/IP connection

##### Driver.txt

Select driver number with 2

*   IP: Module IP

##### Variable.csv

*   Column A/1: Variable name
*   Column B/2: MQTT topic
*   Column C/3: Access mode. w: Only write, r: Only read. (It's case sensitive)

#### Codesys Modbus TCP connection

##### Driver.txt

Select driver number with 3

*   IP: Module IP
*   Port: Port number (normally is 502)

##### Variable.csv

*   Column A/1: Variable name (not used for connection just to address on MQTT)
*   Column B/2: Address (just copy from codesys like %QW.., it's not case sensitive)
*   Column C/3: Datatype (It's not case sensitive)
*   Column D/4: MQTT topic
*   Column E/5: Access mode. w: Only write, r: Only read. (It's case sensitive)

Could copy the field direct from codesys, and remember to just use %Q.. address, and release the addresses in Modbus driver.

#### OPC-UA connection

##### Driver.txt

Select driver number with 4

*   URL: OPC-UA server URL

##### Variable.csv

*   Column A/1: Variable name
*   Column B/2: MQTT topic
*   Column C/3: Access mode. w: Only write, r: Only read. (It's case sensitive)

This protocol uses a recursive search, because of that the variable name must be unique inside PLC, and in the first cycle the connection can take some seconds before to start the cycle connection.

### IT protocols

#### MQTT

The MQTT protocol is mandatory, and cannot be disable.

The MQTT parameters must be configured at /MQTT/mqttConf.txt with fields:

*   host: MQTT broker IP address
*   port: MQTT port (normally 1883)
*   username=MQTT username
*   password=MQTT password

#### Rest API

Rest API is optional.

There are three routes:

GET:

http://IP:5000/variables

*   ip must be replaced by IoT device IP
*   This route return a json with all variables configured at connections

http://IP:5000/variable/variable\_topic

*   ip must be replaced by IoT device IP
*   variable\_topic: must be replaced by topic configured at connections
*   This route return a json with the selected variable

POST:

http://ip:5000/write

Body must be configured as json with:

*   {"topic": "variable\_topic", "value": value}
*   ip must be replaced by IoT device IP
*   variable\_topic: must be replaced by topic configured at connections
*   value: Desired value for variable

## How to run

Before to start, make sure the docker containers is running.

### Python collection

1.  Navigate to python folder
2.  python -m venv .venv
3.  .venv/Scripts/activate # Windows
4.  source .venv/bin/activate # Linux/Mac
5.  pip install -r requirements.txt
6.  python main.py

## Simulation

!\[Architecture\](images/Architecture.png)

A full simulation video can be found at:

All the PLCs has a tank simulation, the Siemens code can be found at /Siemens folder.

Three PLCs running at same time in simulation environment:

*   Siemens running S7 connection and OPC-UA
*   Codesys
*   Rockwell