# Multi-Protocol Industrial IoT Collector (C)

A field-to-cloud industrial data collector written in C, implementing six industrial/IIoT protocols directly against their native libraries rather than a high-level automation framework. It is the low-level counterpart to an equivalent [Python implementation](../project-01-multiprotocol-python) of the same system.

## Architecture

*   **Field drivers:** GPIO (digital only), Siemens S7, Rockwell EtherNet/IP, CODESYS Modbus TCP, OPC-UA. Multiple connections of the same or different protocols can run at once.
*   **Connections to higher level:** MQTT (mandatory, always used) and REST API (optional).

```
                     ┌────────────────────────┐
                     │   ConnectionManager    │
                     │  (reads config/CSV)    │
                     └──────────┬─────────────┘
                                │
        ┌───────┬───────┬───────┼───────┬────────┐
        │       │       │       │       │        │
     Modbus    S7   EtherNet/IP OPC-UA  GPIO    ...
        │       │       │       │       │
        └───────┴───────┴───────┴───────┴────────┘
                                │
                    ┌───────────┴──────────────┐
                    │       main.c loop        │
                    │  (mutex-protected)       │
                    └─────┬───────────────┬────┘
                          │               │
                    ┌─────┴─────┐   ┌─────┴─────────┐
                    │   MQTT    │   │  REST API     │
                    │  publish  │   │(libmicrohttpd)│
                    │ subscribe │   │  GET/POST     │
                    └───────────┘   └───────────────┘
```

Every driver exposes the same minimal interface — `init`, `connect`, `disconnect`, `read`, `write` — all working through plain string values, so the rest of the system never needs to know which protocol is behind a given variable.

## Supported protocols

| Protocol | Library | Notes |
| --- | --- | --- |
| MQTT | `paho-mqtt-c` | Publish/subscribe with automatic reconnection (dedicated thread) |
| Modbus TCP | `libmodbus` | Full CODESYS-style `%M`/`%Q`/`%I` address parsing, all IEC datatypes incl. STRING |
| EtherNet/IP | `libplctag` | Symbolic tag access (ControlLogix/CompactLogix), including custom-length STRING |
| Siemens S7 | `snap7` | Non-optimized DB access, all datatypes incl. bit-level BOOL read/modify/write |
| OPC-UA | `open62541` | Recursive node lookup by variable name, automatic datatype detection, node caching |
| GPIO | `libgpiod` (v2 API) | Persistent line requests for reliable output state (Raspberry Pi) |

## Before you start

A `.env` file must be created inside the `MQTT` folder:

```
MQTT_USERNAME="MQTT user name"
MQTT_PASSWORD="MQTT password"
```

## General parameters

The device cycle scan time (in milliseconds) is configured at `src/generalConf.txt`:

```
cycle=1000
```

## Configuring connections

### Field (OT) drivers

A new folder must be created under `src/connections/` named `Connection_N` (N being any number), containing two files: `driver.txt` and `variables.csv`. Example folders for each protocol are already provided.

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
2.  Datatype (not case-sensitive)
3.  MQTT topic
4.  Access mode — `w`/`r`, case-sensitive

Datatype is required here (unlike some higher-level libraries) because `libplctag` needs the element size to build the tag request.

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

Node lookup is done through a recursive browse of the server's address space, so the variable name must be unique across the whole PLC. Every distinct name is cached after the first successful lookup, so only the very first read/write of a given variable pays the browsing cost.

### IT protocols

#### MQTT

Mandatory, cannot be disabled. Configured at `src/MQTT/mqttConf.txt`:

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
Body (JSON): `{"topic": "variable_topic", "value": "desired_value"}`

## Building and running

Requires `gcc`, `make`, and the following libraries (built from source or via `apt` — see `Makefile` for exact link flags): `paho-mqtt-c`, `libmodbus`, `libplctag`, `snap7`, `open62541`, `libgpiod` (v2 API), `libmicrohttpd`.

```
cd src
make
./IoT_C
```

Tested on x86\_64 (Debian/WSL2) and ARM64 (Raspberry Pi 3, Debian).

## Simulation environment

All PLCs run a small tank-process simulation for testing; the Siemens logic is available under `/Siemens`. Three PLCs run at the same time in the simulation environment:

*   Siemens — S7 and OPC-UA connections
*   CODESYS — Modbus TCP connection
*   Rockwell — EtherNet/IP connection

## Related project

A functionally equivalent implementation exists in Python: [`project-01-multiprotocol-python`](../project-01-multiprotocol-python). The two share the same connection/configuration model, so a `connections/` folder can largely be reused between them (aside from the EtherNet/IP datatype column noted above).