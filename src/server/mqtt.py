import json
import configparser

import paho.mqtt.client as mqtt

from datetime import datetime
from zoneinfo import ZoneInfo
from elasticsearch import Elasticsearch


# Global variables
es: Elasticsearch


def initiate_es_connection():
    """
    Initiate a connection with the ElasticSearch database
    """
    global es

    config = configparser.ConfigParser()
    config.read('config.ini')
    es_config = config['ELASTIC']

    es = Elasticsearch([es_config['server']], verify_certs=False)
    print(es.info())


class MQTTClient:
    """
    Class to define the MQTT Client
    """
    def __init__(self, username, password, broker, port, on_message_callback=None):
        self.client = mqtt.Client()
        self.username = username
        self.password = password
        self.broker = broker
        self.port = port

        # Assign callback functions
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client._client_id = 'brain'

        # Set the username and password
        self.client.username_pw_set(self.username, self.password)

        # Set the on_message callback
        # signature is: on_message(client, userdata, userdata, msg)
        self.on_message = on_message_callback

    @staticmethod
    def on_connect(client, userdata, flags, rc):
        """
        Callback when the client receives a CONNACK response from the server.

        :param client: The connected client
        :param userdata: The user info
        :param flags: Flags
        :param rc: Result code
        """
        print('Connected with result code ' + str(rc))
        # Subscribing in on_connect() means that if we lose the connection and
        # reconnect then subscriptions will be renewed.
        client.subscribe('sensors')

    @staticmethod
    def on_message(client, userdata, msg):
        """
        Callback when a PUBLISH message is received from the server. Send the
        message payload to ElasticSearch.

        :param client: The client sending the message
        :param userdata: The user info
        :param msg: The data sent by the user
        """
        global es
        print(f'{msg.topic} {msg.payload}')

        data = json.loads(msg.payload)
        data['@timestamp'] = datetime.now(ZoneInfo('Europe/Rome')).isoformat()

        es.index(index=msg.topic, body=data)

    def connect(self):
        """
        Connect to the broker.
        """
        self.client.connect(self.broker, self.port, 60)

    def start_loop(self):
        """
        Start the loop.
        """
        self.client.loop_forever()
