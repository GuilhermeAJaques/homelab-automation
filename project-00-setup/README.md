# project-00-setup

This project sets up the cloud layer using Docker containers managed by a docker-compose.yml file and a Raspberry Pi 3 as field computer.

## Raspberry

### Hardware

Model: Pi 3 Mod B v1.2  
CPU: Broadcom BCM2837  
OS architecture: aarch64 (64-bit)

### Operating system

Debian was used as operational system.

#### System configuration

The procedure described below is just valid for the first boot and must run direct on the Pi interface:

1 - Login with "root" user without password

2 - Create and give the correct permissions for a new user:  
`adduser "username"`  
`usermod -aG sudo "username"`

3 - Setup fixed IP:  
`nano /etc/systemd/network/10-eth0.network`  
Enter with the data below:

```
[Match]
Name=eth0

[Network]
Address="DesireIPAddress"
Gateway="DesireGatewayAddress"
DNS=8.8.8.8
```

\- Run the commands:  
`apt install systemd-networkd`  
`systemctl enable systemd-networkd`  
`systemctl start systemd-networkd`

4 - Install SSH server  
`apt install openssh-server`  
`systemctl enable ssh`  
`systemctl start ssh`

Now can switch to SSH connection

5 - Connect by ssh  
`ssh "username"@"definedIpAddress"`

6 - Install sudo  
`su -`  
`apt update`  
`apt install sudo`  
`exit`  
`sudo apt update && sudo apt upgrade -y`

7 - Install base libraries  
`sudo apt install -y git python3 python3-pip build-essential libmodbus-dev`

8 - Install python libraries  
`pip3 install pymodbus paho-mqtt --break-system-packages`  
`pip3 install asyncua --break-system-packages`  
`git clone https://github.com/gijzelaerr/python-snap7.git`  
`cd python-snap7`  
`pip3 install . --break-system-packages`  
`cd ~`  
`echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc`  
`source ~/.bashrc`

9 - Connect to GitHub  
`git config --global user.name "YourUserName"`  
`git config --global user.email "YourEmail"`  
`ssh-keygen -t ed25519 -C "YourEmail"`  
`cat ~/.ssh/id_ed25519.pub`  
Add this key to GitHub

10 - Install Grafana Loki (log aggregation service, runs natively on the Pi) — see the "Logging (Grafana Loki)" section below.

11 - Install the C project's field protocol libraries (`paho-mqtt-c`, `libplctag`, `snap7`, `open62541`, `libgpiod` v2, `libmicrohttpd`) — see "C project dependencies" below. Only needed if you intend to build/run `project-02-multiprotocol-c` on this Pi.

## C project dependencies (project-02)

`project-02-multiprotocol-c` links against several libraries that are either not packaged for this Debian/ARM64 combination, or are packaged in a version too old to use. This section documents the exact steps that produced a working build on a Raspberry Pi 3 (aarch64). Every library that is compiled from source is installed under `/usr/local` (headers) and `/usr/lib` (shared object), and `sudo ldconfig` is run after each one so the linker picks it up.

### General build tools

```
sudo apt update
sudo apt install -y build-essential cmake git pkg-config libssl-dev curl wget unzip
```

### libmodbus and libmicrohttpd (via apt — no build needed)

```
sudo apt install -y libmodbus-dev libmicrohttpd-dev
```

### paho-mqtt-c

```
cd ~
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
make
sudo make install
sudo ldconfig
```

### libplctag (EtherNet/IP)

```
cd ~
git clone https://github.com/libplctag/libplctag.git
cd libplctag
sed -i '1i#define _GNU_SOURCE' src/vendor/libyafl/src/yafl.c
mkdir build && cd build
cmake -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF ..
make
sudo make install
sudo ldconfig
```

The `sed` line patches a small proof-of-concept module (`libyafl`) bundled with the library, which otherwise fails to compile on this Debian release with `mkdir: MAP_ANONYMOUS undeclared` — the fix simply makes sure the `_GNU_SOURCE` feature macro is defined before that file includes the system headers that declare it.

### snap7 (Siemens S7)

The upstream project does not ship a build target for 64-bit ARM, only 32-bit (`arm_v6`/`arm_v7`). A minimal target file needs to be created:

```
cd ~
git clone https://github.com/SCADACS/snap7.git snap7-full
cd snap7-full/build/unix
cat > arm_v8_64_linux.mk << 'EOF'
##
## ARM64 (aarch64) - Raspberry Pi 3/4/5 64-bit OS
##
TargetCPU  :=arm_v8_64
OS         :=linux
CXXFLAGS   := -O3 -g -fPIC -pedantic -std=gnu++14
# Standard part
include common.mk
EOF
make -f arm_v8_64_linux.mk
sudo cp ../bin/arm_v8_64-linux/libsnap7.so /usr/lib/
sudo cp ../../release/Wrappers/c-cpp/snap7.h /usr/local/include/
sudo ldconfig
```

`-std=gnu++14` is required: with the default C++ standard on this g++ version, `snap7`'s own `byte` typedef collides with `std::byte` (introduced in C++17), and the build fails with "reference to 'byte' is ambiguous".

### open62541 (OPC-UA)

```
cd ~
git clone https://github.com/open62541/open62541.git
cd open62541
git submodule update --init --recursive
mkdir build && cd build
cmake -DUA_ENABLE_AMALGAMATION=ON ..
make
sudo cp open62541.h /usr/local/include/
gcc -shared -fPIC -DUA_LOGLEVEL=400 -o libopen62541.so open62541.c -I.
sudo cp libopen62541.so /usr/lib/
sudo ldconfig
```

`UA_LOGLEVEL` controls how verbose the library's own internal connection/session logs are (100 = trace, ..., 600 = fatal only). It only has an effect if it is set **when the** `**.so**` **itself is compiled**, as shown above — setting it in the consuming application's own `Makefile`/`CFLAGS` has no effect on this library. `400` silences routine connection tracing (`info`) while keeping `warning`/`error`; raise it further (e.g. `500` or `600`) and rebuild the `.so` if even fewer messages are wanted.

### libgpiod v2 (GPIO)

The `libgpiod-dev`/`libgpiod2` packages on this Debian release only provide the older v1 (line-request-by-offset) API. This project uses the v2 API (persistent line requests, `gpiod_line_settings`), which has to be built from source:

```
sudo apt remove -y libgpiod-dev libgpiod2
sudo apt install -y meson ninja-build autoconf-archive libtool autoconf automake
cd ~
git clone https://github.com/brgl/libgpiod.git
cd libgpiod
meson setup build --prefix=/usr/local
ninja -C build
sudo ninja -C build install
sudo ldconfig
```

This build is memory-heavy relative to the Pi 3's 1 GB of RAM and can make the SSH session briefly unresponsive (sometimes for a few minutes) while `ninja` is running. Let it finish rather than interrupting it or opening a second SSH session to check on it — the build does complete on its own.

### Verifying everything is installed

```
/sbin/ldconfig -p | grep -E "paho|plctag|snap7|open62541|gpiod|modbus|microhttpd"
find /usr/local/include -iname "MQTTClient.h" -o -iname "libplctag.h" -o -iname "snap7.h" -o -iname "open62541.h" -o -iname "gpiod.h"
```

Every library above should appear in both outputs before attempting to build `project-02-multiprotocol-c` (`cd project-02-multiprotocol-c/src && make`).

## Docker

### Services running

The docker starts 4 containers each one with a different service, as described below:

*   Portainer: Works as a container manager, is useful to manage it remotely via web page.
*   InfluxDB: Time-series database.
*   Grafana: Visualization and dashboarding tool, connected to InfluxDB and to Loki (see Logging section below).
*   Mosquitto: MQTT Broker.
*   Node-red: Work as front end and operation terminal
*   Inlufx-Subscribe: Code created by me to send data from MQTT broker to InluxDB in python

### Getting started

#### Environment variables

An environment file must be created inside the docker/ folder following .env file with the structure:

INFLUXDB\_USERNAME="InfluxUser"  
INFLUXDB\_PASSWORD="InfluxPassword"  
INFLUXDB\_ORG="Organization"  
INFLUXDB\_BUCKET="Bucket"  
INFLUXDB\_TOKEN="Token"  
GRAFANA\_PASSWORD="GrafanaPassword"  
MQTT\_USERNAME="MQTT Usename"  
MQTT\_PASSWORD="MQTT Password"

Replace the placeholder values with your own credentials. Do not use quotation marks.

Passwords must be between 8 and 72 characters long

This file cannot be commited on git.

#### Start docker compose

Navigate to the docker/ folder on terminal and run the command: docker compose up -d

#### Configuring Portainer

Run the code and search for Portainer setup-token  
`docker logs portainer`

Access http://localhost:9000/, create your user and password and paste the setup-token

#### Configuring Grafana

Access http://localhost:3000

Login with user admin, and the password defined on .env file. If the password doesn't work, try admin. This happens when the container was initialized before the .env file was created.

Configure the parameters below:

Connections -> Data sources -> Add data source -> Select InfluxDB -> Fill the parameters below:

*   Query language: Flux
*   URL: http://influxdb:8086
*   Organization: Defined on .env file
*   Token: Defined on .env file
*   Default bucket: Defined on .env file

The Loki data source (used for application logs, see below) is provisioned automatically through `docker/grafana/provisioning/datasources/datasources.yaml` and does not need to be added manually, as long as the URL inside that file points at the Raspberry Pi's actual IP address.

### How to validate if is running

The container that has a webpage, can be checked accessing below:

<table><tbody><tr><td>Container&nbsp;</td><td>Port&nbsp;</td><td>URL</td></tr><tr><td>Portainer</td><td>9000</td><td>http://localhost:9000</td></tr><tr><td>InfluxDB</td><td>8086</td><td>http://localhost:8086</td></tr><tr><td>Grafana</td><td>3000</td><td>http://localhost:3000</td></tr><tr><td>Node-red</td><td>1880</td><td>http://localhost:1880</td></tr></tbody></table>

To test the connection with Mosquitto, open two separate terminals, as described below:

Terminal 1: `docker exec -it mosquitto mosquitto_sub -t "test/topic"`

Terminal 2: `docker exec mosquitto mosquitto_pub -t "test/topic" -m "hello mqtt"`

After command on Terminal 2, must be displayed on terminal 1 "hello mqtt"

## Logging (Grafana Loki)

Application logs from both collectors (Python and C, see project-01 and project-02) are centralized in [Grafana Loki](https://grafana.com/oss/loki/), a lightweight log aggregation system. Unlike the rest of the cloud stack, Loki runs natively on the Raspberry Pi itself (not inside Docker), so logs stay local to the edge device even if the Docker host is unreachable.

### Installing and running Loki on the Pi

The `loki/` folder in this project contains everything needed:

*   `config.yaml`: Loki configuration, using local filesystem storage (no external database required).
*   `loki.service`: systemd unit file, so Loki starts automatically on boot and restarts on failure.
*   `install.sh`: installs its own dependencies (`curl`, `wget`, `unzip`), downloads the latest Loki release for `arm64`, installs the binary to `/usr/local/bin`, creates the data directories under `/var/lib/loki`, copies the configuration and systemd service files into place, then enables and starts the service.

To install and start Loki:

`cd loki`  
`chmod +x install.sh`  
`./install.sh`

The script finishes by waiting 15 seconds for Loki to come up and checking `http://localhost:3100/ready`, which should print `ready`. From then on Loki starts automatically on every boot (`systemctl enable loki` was run by the script); no further action is needed.

To check its status or logs manually at any point:

`sudo systemctl status loki --no-pager`  
`sudo journalctl -u loki -n 50 --no-pager`

### How logs get in

Both collectors send log entries as an HTTP `POST` to `http://<pi-ip>:3100/loki/api/v1/push`, with a JSON body carrying two labels (`class` and `criticality`) and the log message itself. No agent or forwarder is needed between the collector and Loki.

### Viewing logs

Since Grafana is already provisioned with Loki as a data source (see above), logs can be queried directly from **Explore**, selecting **Loki** and using a query such as:

`{criticality=~"Error|Critical"}`

or filtered by module:

`{class="Modbus TCP"}`

### Resetting stored logs

Loki's data lives under `/var/lib/loki` on the Pi. To wipe it and start fresh:

`sudo systemctl stop loki`  
`sudo rm -rf /var/lib/loki/chunks/* /var/lib/loki/tsdb-index/* /var/lib/loki/tsdb-cache/* /var/lib/loki/tsdb-shipper-cache/*`  
`sudo systemctl start loki`