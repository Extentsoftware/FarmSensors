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
import logging
import logging.config

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
MQTT_SENSOR_TOPIC = 'bongo/+/+'
MQTT_REGEX = 'bongo/([^/]+)/([^/]+)'
MQTT_CLIENT_ID = 'MQTTInfluxDBBridgeB'

influxdb_client = InfluxDBClient(INFLUXDB_ADDRESS, INFLUXDB_PORT, INFLUXDB_USER, INFLUXDB_PASSWORD, None)

LOGGING_CONFIG = {
    "version": 1,
    "disable_existing_loggers": True,
    "formatters": {
        "default": {
            "format": "[%(asctime)s] %(levelname)s in %(module)s: %(message)s",
        },
        "access": {
            "format": "%(message)s",
        }
    },
    "handlers": {
        "console": {
            "level": "INFO",
            "class": "logging.StreamHandler",
            "formatter": "default",
            "stream": "ext://sys.stdout",
        },
    },
    "root": {
        "level": "INFO",
        "handlers": ["console"]
    }
}


def send_to_farmos(sensor, data):
    if CONFIG["farmos"]["enabled"]:
        if sensor in CONFIG["sensors"]:
            sensor_config = CONFIG["sensors"][sensor]
            if "farmos_privatekey" in sensor_config:
                privatekey = sensor_config["farmos_privatekey"]
                publickey = sensor_config["farmos_publickey"]
                endpoint = CONFIG["farmos"]["address"]
                url = endpoint + publickey + '/data/basic?private_key=' + privatekey                
                r = requests.post(url = url, data = data) 
            else:
                logging.info(f"Sensor {sensor} not configured for farmos" )
        else:
            logging.info(f"Sensor {sensor} does not exist in sensor config file" )

def on_connect(client, userdata, flags, rc):
    """ The callback for when the client receives a CONNACK response from the server."""
    logging.info(f"Connected with result code {str(rc)}" )    
    client.subscribe(MQTT_SENSOR_TOPIC)

def disconnect_callback(client, userdata, rc):
    logging.info(f"Disconnected with result code {str(rc)}" )    

def subscribe_callback(client, userdata, mid, granted_qos):
    logging.info(f"On subscribe from {client._host}" )    

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
        logging.info(f"Topic {topic} Payload {payload}" )

        topic_parts = topic.split("/")
        if topic_parts[2]=='hub':
            process_hub_message(topic_parts[1], payload)            

        if topic_parts[2]=='sensor':
            process_sensor_message(payload)

    except Exception as e:
        logging.error(f"Exception {e}" )

def process_hub_message(hub_id, payload:bytes):
    frame = LppFrame().from_bytes(payload)
    status = json.loads(payload.decode("utf-8", "ignore"))
    name = hub_id
    geohash_code= None
    if hub_id in CONFIG["hubs"]:
        hub_config = CONFIG["hubs"][hub_id]
        if "name" in hub_config:
            name = hub_config["name"]
        if "geohash" in hub_config:
            geohash_code = hub_config["geohash"]

    json_packet = [
                {
                    "measurement": "hub",
                    "tags": {
                        "hub": hub_id,
                        "name": name,
                    },
                    "time": str(datetime.now()), 
                    "fields": 
                    { 
                        "status" : status["state"]
                    }
                }
            ]

    if geohash_code is not None:
        json_packet[0]['fields']["geohash"] =  geohash_code
        json_packet[0]['tags']["location"] =  True

    logging.info(f"Influx data {json_packet}" )
    influxdb_client.write_points(json_packet)

def process_sensor_message(payload:bytes):
    frame = LppFrame().from_bytes(payload)

    id1 = get_by_channel(frame,0)
    id2 = get_by_channel(frame,1)
    sensor = int(id1) + (int(id2) << 16)
    sensor_hex = hex(sensor)[2:]
    name = sensor_hex
    geohash_code = None

    if sensor_hex in CONFIG["sensors"]:
        sensor_config = CONFIG["sensors"][sensor_hex]
        if "name" in sensor_config:
            name = sensor_config["name"]
        if "geohash" in sensor_config:
            geohash_code = sensor_config["geohash"]

    json_packet = [
                {
                    "measurement": "sensor",
                    "tags": {
                        "sensor": sensor_hex,
                        "name": name,
                    },
                    "time": str(datetime.utcnow()), 
                    "fields": 
                    { 
                    }
                }
            ]

    add_reading(frame, json_packet, 10, "moist" )
    add_reading(frame, json_packet, 14, "snr" )
    add_reading(frame, json_packet, 15, "rssi" )
    add_reading(frame, json_packet, 16, "pfe" )
    add_reading(frame, json_packet, 6, "temp" )
    add_reading(frame, json_packet, 7, "voltsR" )
    add_reading(frame, json_packet, 8, "voltsS" )
    add_reading(frame, json_packet, 3, "distance" )
    lat = add_reading(frame, json_packet, 2, "longitude", 0 )
    lng = add_reading(frame, json_packet, 2, "latitude", 1 )
    alt = add_reading(frame, json_packet, 2, "altitude", 2 )
    
    json_packet[0]['fields']["timestamp"] =  int(datetime.utcnow().timestamp())

    if lat is not None and lng is not None:
        geohash_code =  geohash.encode(lat, lng)
        json_packet[0]['tags']["location"] =  True

    if alt is not None:
        json_packet[0]['fields']["alt"] =  alt

    if geohash_code is not None:
        json_packet[0]['fields']["geohash"] =  geohash_code
        json_packet[0]['tags']["location"] =  True

    logging.info(f"Influx data {json.dumps(json_packet)}" )

    farmos_data = json.dumps(json_packet[0]['fields'])
    logging.info(f"farmos data {sensor_hex} {farmos_data}" )

    send_to_farmos(sensor_hex, farmos_data)
    influxdb_client.write_points(json_packet)


def _init_influxdb_database():
    logging.info('Connecting to influx db')
    databases = influxdb_client.get_list_database()
    if len(list(filter(lambda x: x['name'] == INFLUXDB_DATABASE, databases))) == 0:
        influxdb_client.create_database(INFLUXDB_DATABASE)
    influxdb_client.switch_database(INFLUXDB_DATABASE)

def _init_mqtt():
    logging.info('Connecting to MQTT')
    mqtt_client = mqtt.Client(MQTT_CLIENT_ID)
    mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_disconnect = disconnect_callback
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message    
    mqtt_client.on_subscribe = subscribe_callback
    mqtt_client.connect(MQTT_ADDRESS, MQTT_PORT)
    mqtt_client.loop_forever()

def _read_config():
    global CONFIG
    with open('sensors.json') as json_file:
        CONFIG = json.load(json_file)
    logging.info('sensor configuration read OK')

def main():
    _init_influxdb_database()
    _init_mqtt()

if __name__ == '__main__':
    logging.config.dictConfig(LOGGING_CONFIG)
    logging.info('MQTT to InfluxDB bridge v1.13')

    _read_config()
    
    # for testing
    #payload = b'\x00f\x80\x01f:\x02\x88\x07\xd6\x08\x00\x03\xfd\x00}\x14\x08t\x00\x00\x0ed\x00\x00\x00\x0b\x0fd\x00\x00\x009\x10d\x00\x00\x05\x1d'
    #payload = b'\x00f\x14\x01f\x83\nd\x00\x00\x03\x1b\x06g\x00\x97\x07t\x01L\x08t\x01\x8e\x0ed\x00\x00\x00\t\x0fd\x00\x00\x003\x10d\x00\x00Cl'
    #payload = b'\x00d\x00\x00:\x14\x01d\x00\x00Y\x83\nd\x00\x00\x03\x1d\x06g\x00\x9f\x07t\x01K\x08t\x01\xa3\x0ed\x00\x00\x00\t\x0fd\x00\x00\x002\x10d\x00\x00C\x1c'
    #payload = "{ \"state\" : \"connected\" }".encode("utf-8")
    #process_message("bongo/test/hub", payload)
    payload = b'\x00d\x00\x00\x13C\x01d\x00\x00\x14\xe7\nd\x00\x00\x03\x1e\x06g\x00\xca\x07t\x00\xe6\x08t\x01\x99\x0ed\x00\x00\x00\n\x0fd\x00\x00\x00:\x10d\x00\x006\xa2'
    process_message("bongo/14e71042/sensor", payload)
    
    # hub
    #payloadx =b'\x02\x88\x07\xd6\x07\x00\x03\xfd\x00$\xd6'
    #process_message("bongo/349454bf5c70/hub", payloadx)

    main()
