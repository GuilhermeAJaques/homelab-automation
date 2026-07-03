# project-00-setup

This project sets up the cloud layer using Docker containers managed by a docker-compose.yml file and a Raspberry Pi 3 as field computer.

## Raspberry

### Hardware

Model: Pi 3 Mod B v1.2
CPU: Broadcom BCM2837

### Operating system

Debian was used as operational system.

#### System configuration

The procedure described below is just valid for the first boot and must run direct on the Pi interface:
1 - Login with "root" user without password
2 - Create and give the correct permissions for a new user:
    - adduser "username"
    - usermod -aG sudo "username"
3 - Setup fixed IP:
    - nano /etc/systemd/network/10-eth0.network
    Enter with the data below:
    [Match]
    Name=eth0

    [Network]
    Address="DesireIPAddress"
    Gateway="DesireGatewayAddress"
    DNS=8.8.8.8
    - Run the commands:
    - apt install systemd-networkd
    - systemctl enable systemd-networkd
    - systemctl start systemd-networkd
4 - Install SSH server
    - apt install openssh-server
    - systemctl enable ssh
    - systemctl start ssh

Now can switch to SSH connection

5 - Connect by ssh
    - ssh "username"@"definedIpAddress"
6 - Install sudo
    - su -
    - apt update
    - apt install sudo
    - exit
    - sudo apt update && sudo apt upgrade -y
7 - Install base libraries
    - sudo apt install -y git python3 python3-pip build-essential libmodbus-dev libgpiod-dev
8 - Install python libraries
    - pip3 install pymodbus paho-mqtt --break-system-packages
    - pip3 install asyncua --break-system-packages
    - git clone https://github.com/gijzelaerr/python-snap7.git
    - cd python-snap7
    - pip3 install . --break-system-packages
    - cd ~
    - echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
    - source ~/.bashrc
9 - Connect to GitHub
    - git config --global user.name "YourUserName"
    - git config --global user.email "YourEmail"
    - ssh-keygen -t ed25519 -C "YourEmail"
    - cat ~/.ssh/id_ed25519.pub
    Add this key to GitHub

## Docker

### Services running

The docker starts 4 containers each one with a different service, as described below:

* Portainer: Works as a container manager, is useful to manage it remotely via web page.
* InfluxDB: Time-series database.
* Grafana: Visualization and dashboarding tool, connected to InfluxDB.
* Mosquitto: MQTT Broker.



### Getting started

#### Environment variables

An environment file must be created inside the docker/ folder following .env file with the structure:

INFLUXDB_USERNAME="InfluxUser"

INFLUXDB_PASSWORD="InfluxPassword"

INFLUXDB_ORG="Organization"

INFLUXDB_BUCKET="Bucket"

INFLUXDB_TOKEN="Token"

GRAFANA_PASSWORD="GrafanaPassword"

Replace the placeholder values with your own credentials. Do not use quotation marks.

Passwords must be between 8 and 72 characters long

This file cannot be commited on git.


#### Start docker compose

Navigate to the docker/ folder on terminal and run the command: docker compose up -d


#### Configuring Portainer

Run the code and search for Portainer setup-token
docker logs portainer

Access http://localhost:9000/, create your user and password and paste the setup-token

#### Configuring Grafana

Access http://localhost:3000 

Login with user admin, and the password defined on .env file. If the password doesn't work, try admin. This happens when the container was initialized before the .env file was created.

Configure the parameters below:



Connections -> Data sources -> Add data source -> Select InfluxDB -> Fill the parameters below:

* Query language: Flux
* URL: http://influxdb:8086
* Organization: Defined on .env file
* Token: Defined on .env file
* Default bucket: Defined on .env file



### How to validate if is running



The container that has a webpage, can be checked accessing below:

| Container | Port | URL |

|-----------|------|-----|

| Portainer | 9000 | http://localhost:9000 |

| InfluxDB  | 8086 | http://localhost:8086 |

| Grafana   | 3000 | http://localhost:3000 |



To test the connection with Mosquitto, open two separate terminals, as described below:



Terminal 1: docker exec -it mosquitto mosquitto_sub -t "test/topic"

Terminal 2: docker exec mosquitto mosquitto_pub -t "test/topic" -m "hello mqtt"

After command on Terminal 2, must be displayed on terminal 1 "hello mqtt"