#!/usr/bin/env python3

"""A MQTT to InfluxDB Bridge

This script receives MQTT data and saves those to InfluxDB.

"""
import os
import math
import decimal
import json
from datetime import date
from datetime import time
from datetime import datetime
import re
from typing import NamedTuple
from cayennelpp.lpp_frame import LppFrame
import binascii


import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient
import requests 

INFLUXDB_ADDRESS = os.environ.get('INFLUXDB_ADDRESS')
#INFLUXDB_ADDRESS = 'localhost'
INFLUXDB_PORT = 8086
INFLUXDB_USER = 'root'
INFLUXDB_PASSWORD = 'root'
INFLUXDB_DATABASE = 'home_db'
 
MQTT_PORT= 1883
MQTT_ADDRESS = os.environ.get('MOSQUITTO_ADDRESS')
#MQTT_ADDRESS = 'localhost'
MQTT_USER = 'mqttuser'
MQTT_PASSWORD = 'mqttpassword'
MQTT_TOPIC = 'bongo/+/+'  # bongo/[sensor]/[measurement]
MQTT_REGEX = 'bongo/([^/]+)/([^/]+)'
MQTT_CLIENT_ID = 'MQTTInfluxDBBridgeA'

influxdb_client = InfluxDBClient(INFLUXDB_ADDRESS, INFLUXDB_PORT, INFLUXDB_USER, INFLUXDB_PASSWORD, None)

def get_sensor_geohash(sensor):
    if sensor in CONFIG["sensors"]:
        sensor_config = CONFIG["sensors"][sensor]
        if "geohash" in sensor_config:
            return sensor_config["geohash"]
    return None

def send_to_farmos(sensor, data):
    if CONFIG["farmos"]["enabled"]:
        if sensor in CONFIG["sensors"]:
            sensor_config = CONFIG["sensors"][sensor]
            if "farmos_privatekey" in sensor_config:
                privatekey = sensor_config["farmos_privatekey"]
                publickey = sensor_config["farmos_publickey"]
                endpoint = CONFIG["farmos"]["address"]
                url = endpoint + publickey + '?private_key=' + privatekey
                print(url)
                r = requests.post(url = url, data = data) 
            else:
                print(sensor, " not configure for farmos" )
        else:
            print(sensor, " does not exist in sensor config file" )
    else:
        print( "FarmOS disabled" )

def on_connect(client, userdata, flags, rc):
    """ The callback for when the client receives a CONNACK response from the server."""
    print('Connected with result code ' + str(rc))
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    process_message(msg.topic, msg.payload)
    
def process_message(topic, payload):
    """The callback for when a PUBLISH message is received from the server."""
    now = datetime.now()

    print(binascii.hexlify(payload));

    frame = LppFrame().from_bytes(payload)

    measurement = "sensor"

    moist = frame.get_by_type(100)[0].value[0]
    snr = frame.get_by_type(100)[1].value[0]
    rssi = frame.get_by_type(100)[2].value[0]
    pfe = frame.get_by_type(100)[3].value[0]
    
    temp = frame.get_by_type(103)[0].value[0]
    voltsR = frame.get_by_type(116)[0].value[0]
    voltsS = frame.get_by_type(116)[1].value[0]
    id1 = frame.get_by_type(102)[0].value[0]
    id2 = frame.get_by_type(102)[1].value[0]

    sensor = int(id1) + (int(id2) << 16)

    json_body = [
                {
                    "measurement": measurement,
                    "tags": {
                        "sensor": sensor,
                    },
                    "time": str(now), 
                    "fields": 
                    { 
                        "moist": moist, 
                        "temp": temp, 
                        "voltsR": voltsR, 
                        "voltsS": voltsS, 
                        "snr":snr, 
                        "rssi":rssi, 
                        "pfe":pfe 
                    }
                }
            ]

    print(json_body)

    influxdb_client.write_points(json_body)

    send_to_farmos(sensor, payload);

def _init_influxdb_database():
    databases = influxdb_client.get_list_database()
    if len(list(filter(lambda x: x['name'] == INFLUXDB_DATABASE, databases))) == 0:
        influxdb_client.create_database(INFLUXDB_DATABASE)
    influxdb_client.switch_database(INFLUXDB_DATABASE)

def _init_mqtt():
    mqtt_client = mqtt.Client(MQTT_CLIENT_ID)
    mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message    
    mqtt_client.connect(MQTT_ADDRESS, MQTT_PORT)
    mqtt_client.loop_forever()

def _read_config():
    global CONFIG
    with open('sensors.json') as json_file:
        CONFIG = json.load(json_file)

def main():
    _read_config();
    _init_influxdb_database()
    _init_mqtt()


def getSensorValue(frame, type, channel):
    _read_config();
    _init_influxdb_database()
    _init_mqtt()


if __name__ == '__main__':
    print('MQTT to InfluxDB bridge v1.7')
    # buffer = bytearray([0x00,0x66,0x12,0x01,0x66,0x83,0x0a,0x64,0x00,0x00,0x00,0x00,0x06,0x67,0x00,0x00,0x07,0x74,0x00,0x00,0x08,0x74,0x01,0x8b,0x0e,0x64,0x00,0x00,0x00,0x0a,0x0f,0x64,0x00,0x00,0x00,0x5c,0x10,0x64,0x00,0x00,0x1c,0x7a])
    # frame = LppFrame().from_bytes(buffer)

    # moist = frame.get_by_type(100)[0].value[0]
    # temp = frame.get_by_type(103)[0].value[0]
    # voltsR = frame.get_by_type(116)[0].value[0]
    # voltsS = frame.get_by_type(116)[1].value[0]
    # id1 = frame.get_by_type(102)[0].value[0]
    # id2 = frame.get_by_type(102)[1].value[0]
    # sensor = int(id1) + (int(id2) << 16)

    # x = frame.get_by_type(116)
    # snr = frame.get_by_type(100)[1].value[0]
    # rssi = frame.get_by_type(100)[2].value[0]
    # pfe = frame.get_by_type(100)[3].value[0]

    main()
