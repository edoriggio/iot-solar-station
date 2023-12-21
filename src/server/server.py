import configparser

from mqtt import MQTTClient, initiate_es_connection


def main():
    # Read config file
    config = configparser.ConfigParser()
    config.read('config.ini')
    mqtt_config = config['MQTT']

    # Initiate connection to ElasticSearch
    initiate_es_connection()

    # Initiate connection to the MQTT server
    mqtt_client = MQTTClient(
        mqtt_config['username'],
        mqtt_config['password'],
        mqtt_config['broker'],
        int(mqtt_config['port'])
    )
    mqtt_client.connect()
    mqtt_client.start_loop()


if __name__ == '__main__':
    main()
