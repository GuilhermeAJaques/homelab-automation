# project-00-setup

This project sets up the cloud layer using Docker containers managed by a docker-compose.yml file.



## Services running

The docker starts 4 containers each one with a different service, as described below:

* Portainer: Works as a container manager, is useful to manage it remotely via web page.
* InfluxDB: Time-series database.
* Grafana: Visualization and dashboarding tool, connected to InfluxDB.
* Mosquitto: MQTT Broker.



## Getting started

### Environment variables

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


### Start docker compose

Navigate to the docker/ folder on terminal and run the command: docker compose up -d


### Configuring Portainer

Run the code and search for Portainer setup-token
docker logs portainer

Access http://localhost:9000/, create your user and password and paste the setup-token

### Configuring Grafana

Access http://localhost:3000 

Login with user admin, and the password defined on .env file. If the password doesn't work, try admin. This happens when the container was initialized before the .env file was created.

Configure the parameters below:



Connections -> Data sources -> Add data source -> Select InfluxDB -> Fill the parameters below:

* Query language: Flux
* URL: http://influxdb:8086
* Organization: Defined on .env file
* Token: Defined on .env file
* Default bucket: Defined on .env file



## How to validate if is running



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