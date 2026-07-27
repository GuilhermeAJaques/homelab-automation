import requests
import time
import configparser
from enum import Enum

class Criticality(Enum):
    INFO = "Info"
    WARNING = "Warning"
    ERROR = "Error"
    CRITICAL = "Critical"

class Class(Enum):
    GENERAL = "General"
    REST_API = "Rest API"
    GPIO = "GPIO"
    CONNECTION_MANAGER = "Connection manager"
    MQTT = "MQTT"
    ETHERNET = "Ethernet/IP"
    MODBUS = "Modbus TCP"
    OPC = "OPC-UA"
    S7 = "S7 Connection"

class Logger:
    _host = None
    _port = None
    _initialized = False

    @classmethod
    def _load_config(cls):
        if cls._initialized:
            return
        config = configparser.ConfigParser()
        config.read("logConf.txt")
        cls._host = config["Settings"]["host"]
        cls._port = config["Settings"]["port"]
        cls._initialized = True

    @classmethod
    def log(cls, className, criticality, description):
        print(f"[{criticality.value.upper()}] [{className.value}] {description}")
        cls._load_config()
        try:
            timestamp = str(time.time_ns())
            payload = {
                "streams": [
                    {
                        "stream": {
                            "class": className.value,
                            "criticality": criticality.value
                        },
                        "values": [
                            [timestamp, description]
                        ]
                    }
                ]
            }
            url = f"http://{cls._host}:{cls._port}/loki/api/v1/push"
            response = requests.post(url, json=payload, timeout=2)
        except Exception as e:
            pass