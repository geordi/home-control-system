#!/usr/bin/env python3

import datetime
import logging
import socket
import sys

from common import *
from localconfig import PI_IP, PI_PORT, ARDUINO_IP, ARDUINO_PORT


MIN_TIMEOUT = datetime.timedelta(minutes=STAIRCASE_TIMEOUT_MINUTES)


def print_transmission_db_to_log(transmission_db):
    logging.debug("Transmission DB:")
    for magnet_id in transmission_db:
        logging.debug("{:08d}: {}".format(magnet_id, transmission_db[magnet_id]))


def read_db_keys():
    request = 'GET-KEYS'

    response = send_udp_data(PI_IP, PI_PORT, request)
    response.rstrip().strip()

    if len(response) > 0 and not response.startswith('None'):
        keys = response.split(';')
        keys = [ int(x) for x in keys ]
        return keys
    else:
        return None


def read_db_value(key):
    request = 'GET-VALUE'

    if type(key) == int:
        request += ';' + str(key)
    elif type(key) == str:
        request += ';' + key

    response = send_udp_data(PI_IP, PI_PORT, request)
    response.rstrip().strip()

    if len(response) > 0 and not response.startswith('None'):
        last_time = datetime.datetime.strptime(response, '%Y-%m-%d %H:%M:%S.%f')
        return last_time
    else:
        return None


def delete_db_value(key):
    request = 'DELETE-VALUE'

    if type(key) == int:
        request += ';' + str(key)
    elif type(key) == str:
        request += ';' + key

    response = send_udp_data(PI_IP, PI_PORT, request)
    response.rstrip().strip()


def send_off_signal(switch_id):
    data = "rf {}:10:0".format(switch_id)
    received = send_udp_data(ARDUINO_IP, ARDUINO_PORT, data)

    return received


def send_udp_data(host, port, data_to_send):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    sock.sendto(bytes(data_to_send + "\n", "utf-8"), (host, port))
    received = str(sock.recv(1024), "utf-8")

    logging.info("Sent UDP data:     {}".format(data_to_send))
    logging.info("Received UDP data: {}".format(received))

    return received


def process_transmission_db_keys(transmission_db_keys):
    now = datetime.datetime.now()

    magnet_ids_to_be_removed = []

    for magnet_id in transmission_db_keys:
        
        last_time = read_db_value(magnet_id)

        if magnet_id == STAIRCASE_DOOR_MAGNET_ID:
            if now - last_time > MIN_TIMEOUT:
                logging.info("Switching off")
                send_off_signal('C')
                if magnet_id in transmission_db_keys:
                    logging.info("Removing staircase door magnet record since it's too old.")
                    magnet_ids_to_be_removed.append(magnet_id)

    for magnet_id in magnet_ids_to_be_removed:
        delete_db_value(magnet_id)

    return magnet_ids_to_be_removed


def main():
    logging.basicConfig(filename=LOG_TIMEOUT_FILENAME, format='%(asctime)s %(levelname)s:%(message)s', level=logging.DEBUG)

    logging.info("Starting light timeout.")

    db_keys = read_db_keys()

    magnet_ids_to_be_removed = process_transmission_db_keys(db_keys)

    logging.info("Light timeout FINISHED.")


if __name__ == '__main__':
    main()
