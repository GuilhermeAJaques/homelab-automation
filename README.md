# homelab-automation

This repository is a study for Linux and its application for industrial automation. Inside it has a series of projects, where each one will treat a specific application in industry.

## Architecture

The general architecture for this repository is to simulate the levels in a real industry scenario.

*   Field: Simulated by a PLC using SIEMENS PLC SIM Advanced, CODESYS simulation, and raspberry as a controller using C, and CODESYS.
*   Edge device: Raspberry Pi.
*   Cloud: Docker running in my Windows computer.

The communication between the levels may change based on the project.

## Project 00: Setup base software

Setup Cloud level on docker with the services, Portainer, InfluxDB, Grafana, and Mosquitto. Configure Git and create a repository structure.

## Project 01: Multiprotocol IoT using python

Create a multiprotocol IoT to connect in multiples PLCs and MQTT broker, using Raspbarry PI 3 as hardware, with programable language in Python