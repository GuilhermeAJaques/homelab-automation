# Multi-Protocol Industrial IoT Collector (Python)

A field-to-cloud industrial data collector written in Python, implementing five industrial/IIoT field protocols behind a common driver interface, bridged to MQTT and an optional REST API. It is the higher-level counterpart to an equivalent [C implementation](../project-02-multiprotocol-c) of the same system.

## Architecture

*   **Field drivers:** GPIO (digital only), Siemens S7, Rockwell EtherNet/IP, CODESYS Modbus TCP, OPC-UA. Multiple connections of the same or different protocols can run at once.
*   **Connections to higher level:** MQTT (mandatory, always used) and REST API (optional).

```
                     ┌───────────────────────┐
                     │   ConnectionManager   │
                     │  (reads config/CSV)   │
                     └──────────┬────────────┘
                                │
        ┌───────┬───────┬───────┼───────┬────────┐
        │       │       │       │       │        │
     Modbus    S7   EtherNet/IP OPC-UA  GPIO    ...
        │       │       │       │       │
        └───────┴───────┴───────┴───────┴────────┘
                                │
                    ┌───────────┴────────────┐
                    │        main.py         │
                    └─────┬───────────────┬──┘
                          │               │
                    ┌─────┴─────┐   ┌─────┴───────┐
                    │   MQTT    │   │  REST API   │
                    │  publish  │   │   (Flask)   │
                    │ subscribe │   │  GET/POST   │
                    └───────────┘   └─────────────┘
```

Every driver exposes the same minimal interface — `connect`, `disconnect`, `read_variable`, `write_variable` — so the rest of the system never needs to know which protocol is behind a given variable.

## Supported protocols

| Protocol | Library | Notes |
| --- | --- | --- |
| MQTT | `paho-mqtt` | Publish/subscribe with automatic reconnection |
| Modbus TCP | `pymodbus` | CODESYS-style `%M`/`%Q`/`%I` address parsing, all IEC datatypes incl. STRING |
| EtherNet/IP | `pylogix` | Symbolic tag access (ControlLogix/CompactLogix) |
| Siemens S7 | `python-snap7` | Non-optimized DB access, all datatypes |
| OPC-UA | `asyncua` / `opcua` | Recursive node lookup by variable name |
| GPIO | `gpiod` (v2 API) | Digital I/O (Raspberry Pi) |

## Before you start

A `.env` file must be created inside the `MQTT` folder:

```
[broker]
MQTT_USERNAME="MQTT user name"
MQTT_PASSWORD="MQTT password"
```

## General parameters

The device cycle scan time (in seconds) is configured at `python/generalConf.txt`:

```
[General parameters]
 # Scan cycle in milliseconds
cycle=1000
```

## Configuring connections

### Field (OT) drivers

A new folder must be created under `python/connections/` named `Connection_N` (N being any number), containing two files: `driver.txt` and `variables.csv`. Example folders for each protocol are already provided.

#### GPIO

**driver.txt** — select driver number `0`

*   `chip`: full GPIO chip path (for Raspberry Pi 3 Model B+, `/dev/gpiochip0`)

**variables.csv** columns

1.  GPIO line number
2.  MQTT topic
3.  Access mode — `w` (write/digital output) or `r` (read/digital input). Case-sensitive.

#### Siemens S7

**driver.txt** — select driver number `1`

*   `ip`: module IP
*   `rack`: rack number (normally `0`)
*   `slot`: slot number (normally `1`)

**variables.csv** columns

1.  DB number
2.  Variable name (not used for addressing, only for reference/MQTT payload)
3.  Datatype (not case-sensitive)
4.  Offset (e.g. `258.0`)
5.  MQTT topic
6.  Access mode — `w`/`r`, case-sensitive

The DB must be marked as **non-optimized** in TIA Portal for byte/bit offsets to be addressable this way.

#### Rockwell EtherNet/IP

**driver.txt** — select driver number `2`

*   `ip`: module IP

**variables.csv** columns

1.  Variable (tag) name
2.  MQTT topic
3.  Access mode — `w`/`r`, case-sensitive

#### CODESYS Modbus TCP

**driver.txt** — select driver number `3`

*   `ip`: module IP
*   `port`: port number (normally `502`)

**variables.csv** columns

1.  Variable name (not used for addressing, only for reference/MQTT payload)
2.  Address (copied directly from CODESYS, e.g. `%QW10`; not case-sensitive)
3.  Datatype (not case-sensitive)
4.  MQTT topic
5.  Access mode — `w`/`r`, case-sensitive

Only `%Q` addresses are supported; the addresses must be released for Modbus access in the CODESYS Modbus driver configuration.

#### OPC-UA

**driver.txt** — select driver number `4`

*   `url`: OPC-UA server endpoint URL

**variables.csv** columns

1.  Variable name
2.  MQTT topic
3.  Access mode — `w`/`r`, case-sensitive

Node lookup is done through a recursive browse of the server's address space, so the variable name must be unique across the whole PLC. On the first cycle, the connection can take a few extra seconds before the read cycle actually starts.

### IT protocols

#### MQTT

Mandatory, cannot be disabled. Configured at `python/MQTT/mqttConf.txt`:

*   `host`: MQTT broker IP address
*   `port`: MQTT port (normally `1883`)

(Username and password come from the `.env` file described above.)

#### REST API

Optional. Three routes are available:

**GET** `http://<device-ip>:5000/variables`  
Returns a JSON array with every configured variable and its current value.

**GET** `http://<device-ip>:5000/variable/<topic>`  
Returns a JSON object with the current value of a single variable, identified by its configured MQTT topic.

**POST** `http://<device-ip>:5000/write`  
Body (JSON): `{"topic": "variable_topic", "value": value}`

## Logging (Grafana Loki)

All internal events (connection status, read/write errors, reconnection attempts) go through a single `Logger` class instead of scattered `print` calls, defined in `Logger.py`. Every call prints to the console **and** pushes the event to a [Grafana Loki](../project-00-setup/#logging-grafana-loki) instance, so operational history is queryable from Grafana alongside the process data.

Configured at `python/logConf.txt`:

```
[Settings]
host=localhost
port=3100
```

Usage from anywhere in the codebase:

```python
from Logger import Logger, Criticality, Class
 
Logger.log(Class.MODBUS, Criticality.ERROR, "Failed to connect to Modbus TCP server")
```

`**Criticality**` (sent as the `criticality` Loki label): `INFO`, `WARNING`, `ERROR`, `CRITICAL`.

`**Class**` (sent as the `class` Loki label): `GENERAL`, `REST_API`, `GPIO`, `CONNECTION_MANAGER`, `MQTT`, `ETHERNET`, `MODBUS`, `OPC`, `S7` — one per module/protocol, matching the equivalent enum in the C implementation so both collectors' logs share the same label values in Grafana.

If Loki is unreachable, the HTTP call times out after 2 seconds and is silently discarded — logging never blocks or crashes the main collection loop, it only stops being persisted remotely until connectivity is restored.

## Building and running

Make sure the Docker containers (MQTT broker, InfluxDB, Grafana, Node-RED) are running first.

```
cd python
python -m venv .venv
.venv/Scripts/activate      # Windows
source .venv/bin/activate   # Linux/Mac
pip install -r requirements.txt
python main.py
```

## Simulation environment

All PLCs run a small tank-process simulation for testing; the Siemens logic is available under `/Siemens`. Three PLCs run at the same time in the simulation environment:

*   Siemens — S7 and OPC-UA connections
*   CODESYS — Modbus TCP connection
*   Rockwell — EtherNet/IP connection

## Related project

A functionally equivalent implementation exists in C: [`project-02-multiprotocol-c`](../project-02-multiprotocol-c). The two share the same connection/configuration model, so a `connections/` folder can largely be reused between them (aside from the EtherNet/IP variables.csv, which the C version extends with an explicit datatype column).