#!/usr/bin/env python3

"""A MQTT to InfluxDB Bridge

This script receives MQTT data and saves those to InfluxDB.

"""
import os
import json
from datetime import datetime
from cayennelpp.lpp_frame import LppFrame
import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient
import requests 
import geohash


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

def on_connect(client, userdata, flags, rc):
    """ The callback for when the client receives a CONNACK response from the server."""
    print('Connected with result code ' + str(rc))
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    process_message(msg.topic, msg.payload)

def get_by_channel(frame, channel, index:int=0):
    """Return sub list of data with items matching given type."""
    v = [d for d in frame.data if int(d.channel) == channel]
    return v[0].value[index]

def add_reading(frame, json_body, channel, name, index:int=0):
    try:
        value = get_by_channel(frame, channel, index)
        json_body[0]['fields'][name] = value
        return value
    except Exception as e:
        pass

def process_message(topic, payload:bytes):
    try:
        """The callback for when a PUBLISH message is received from the server."""
        now = datetime.now()

        print(payload)

        frame = LppFrame().from_bytes(payload)

        measurement = "sensor"

        id1 = get_by_channel(frame,0)
        id2 = get_by_channel(frame,1)
        sensor = int(id1) + (int(id2) << 16)
        sensor_hex = hex(sensor)[2:]
        json = [
                    {
                        "measurement": measurement,
                        "tags": {
                            "sensor": sensor_hex,
                        },
                        "time": str(now), 
                        "fields": 
                        { 
                        }
                    }
                ]

        add_reading(frame, json, 10, "moist" )
        add_reading(frame, json, 14, "snr" )
        add_reading(frame, json, 15, "rssi" )
        add_reading(frame, json, 16, "pfe" )
        add_reading(frame, json, 6, "temp" )
        add_reading(frame, json, 7, "voltsR" )
        add_reading(frame, json, 8, "voltsS" )
        add_reading(frame, json, 3, "distance" )
        lat = add_reading(frame, json, 2, "longitude", 0 )
        lng = add_reading(frame, json, 2, "latitude", 1 )
        alt = add_reading(frame, json, 2, "altitude", 2 )

        if lat is not None and lng is not None:
            json[0]['fields']["geohash"] =  geohash.encode(lat, lng)
            json[0]['fields']["alt"] =  alt
            json[0]['tags']["location"] =  True

        print(json)

        influxdb_client.write_points(json)

        send_to_farmos(sensor, payload)
    except Exception as e:
        print(e)

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
    _read_config()
    _init_influxdb_database()
    _init_mqtt()


def getSensorValue(frame, type, channel):
    _read_config()
    _init_influxdb_database()
    _init_mqtt()


if __name__ == '__main__':
    print('MQTT to InfluxDB bridge v1.12')
    
    # for testing
    #payload = b'\x00f\x80\x01f:\x02\x88\x07\xd6\x08\x00\x03\xfd\x00}\x14\x08t\x00\x00\x0ed\x00\x00\x00\x0b\x0fd\x00\x00\x009\x10d\x00\x00\x05\x1d'
    #payload = b'\x00f\x14\x01f\x83\nd\x00\x00\x03\x1b\x06g\x00\x97\x07t\x01L\x08t\x01\x8e\x0ed\x00\x00\x00\t\x0fd\x00\x00\x003\x10d\x00\x00Cl'
    #payload = b'\x00d\x00\x00:\x14\x01d\x00\x00Y\x83\nd\x00\x00\x03\x1d\x06g\x00\x9f\x07t\x01K\x08t\x01\xa3\x0ed\x00\x00\x00\t\x0fd\x00\x00\x002\x10d\x00\x00C\x1c'
    #process_message("topic", payload)

    main()
