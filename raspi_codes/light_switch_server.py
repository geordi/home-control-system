#!/usr/bin/env python3

import datetime
import logging
import os
import select
import socketserver
import sys
import time

from common import *
import localconfig
import sun_rise_sun_set_processor as ssp

REPEAT_TRANSMISSION_TIMES = 10

SERVER_HOST, SERVER_PORT = '0.0.0.0', localconfig.PI_PORT

MAX_TIMEDELTA_BETWEEN_TRANSMISSIONS = datetime.timedelta(seconds=4)


def is_close_time_transmission(transmission_db, now, magnet_id):
    last_transmission_time = transmission_db[magnet_id]
    td = now - last_transmission_time
    if td < MAX_TIMEDELTA_BETWEEN_TRANSMISSIONS:
        return True
    else:
        return False


def handle_magnet_switch(magnet_desc, db, sun_db, socket, client_address):
    SUN_OFFSET = datetime.timedelta(hours=STAIRCASE_SUN_OFFSET_HOURS)

    now = datetime.datetime.now()
    today = datetime.date.today()
    _, magnet_id = magnet_desc.split(':')
    magnet_id = int(magnet_id)
    #print("Got magnet id: ", magnet_id)
    #print("Current DB state:")
    #print(db)
    #print("now:", now)
    #print("Sun rise: ", sun_db.sun_rise(today))
    #print("Sun set: ", sun_db.sun_set(today))
    #print("Sun rise + offset: ", sun_db.sun_rise(today) + SUN_OFFSET,  now < (sun_db.sun_rise(today) + SUN_OFFSET))
    #print("Sun set - offset: ", sun_db.sun_set(today) - SUN_OFFSET, now > (sun_db.sun_set(today) - SUN_OFFSET))

    SUN_OFFSET = datetime.timedelta(hours=STAIRCASE_SUN_OFFSET_HOURS)

    if now < (sun_db.sun_rise(today) + SUN_OFFSET) or now > (sun_db.sun_set(today) - SUN_OFFSET):
        if db.get_value(magnet_id):
            #print("Magnet already in DB")
            if magnet_id == STAIRCASE_DOOR_MAGNET_ID:
                if not is_close_time_transmission(db.db, now, magnet_id):
                    #print("NOT close transmission")
                    handle_light_switch('C:ON', socket, client_address)
        else:
            #print('Magnet not in DB')
            if magnet_id == STAIRCASE_DOOR_MAGNET_ID:
                #print("Spravny magnet")
                handle_light_switch('C:ON', socket, client_address)

        db.set_value(magnet_id, now)


def handle_rf(magnet_desc, db, sun_db, socket, client_address):
    _, rf_id = magnet_desc.split(':')
    rf_id = int(rf_id)
    #print("Got rf id: ", rf_id)
    #print("Current DB state:")
    #print(db)

    if rf_id == STAIRCASE_DOOR_MAGNET_ID:
        handle_magnet_switch(magnet_desc, db, sun_db, socket, client_address)
    else:
        now = datetime.datetime.now()
        db.set_value(rf_id, now)

    socket.sendto('RF handled'.encode(), client_address)


def create_light_switch_datagram(light_spec):
    socket_id, command = light_spec.split(':')

    if command == 'ON':
        command = 1
    else:
        command = 0

    datagram = 'rf {}:{}:{}'.format(socket_id, REPEAT_TRANSMISSION_TIMES, command)
    return datagram.encode()


def handle_light_switch(light_spec, socket, client_address):
    """
    'rf light_switch:repeats:state'
    light_switch = [A-E]
    repeats = 10
    state = [1, 0]
    """

    datagram = create_light_switch_datagram(light_spec)
    socket.sendto(datagram, client_address)


class MyUDPHandler(socketserver.BaseRequestHandler):

    def process_db_communication(self, db, data_decoded):
        if data_decoded.startswith('GET-KEYS'):
            keys = db.get_keys()
            keys = [ str(x) for x in keys ]
            response = ';'.join(keys)
            return response.encode()
        if data_decoded.startswith('GET-VALUE'):
            _, key = data_decoded.split(';')
            key = int(key)
            db_value = db.get_value(key)
            if type(db_value) == datetime.datetime:
                db_value = db_value.strftime('%Y-%m-%d %H:%M:%S.%f')
            response = db_value.encode()
            return response
        if data_decoded.startswith('DELETE-VALUE'):
            _, key = data_decoded.split(';')
            key = int(key)
            db.delete_value(key)
            return 'DELETE-DONE'.encode()
        return 'Unknown DB operation'.encode()


    def handle(self):
        data = self.request[0].strip()
        socket = self.request[1]
        #print("{} wrote:".format(self.client_address[0]))
        #print('Data:', data)
        data_decoded = data.decode()
        #print('Data decoded:', data_decoded)
    
        if data_decoded.startswith('RF'):
            datagram = handle_rf(data_decoded, self.server.db, self.server.sun_db, socket, self.client_address)
        else:
            response = self.process_db_communication(self.server.db, data_decoded)
            socket.sendto(response, self.client_address)


class MyServer(socketserver.UDPServer):

    def __init__(self, server_address, db, sun_db, handler_class=MyUDPHandler):
        socketserver.UDPServer.__init__(self, server_address, handler_class)
        self.db = db
        self.sun_db = sun_db


    def server_activate(self):
        socketserver.UDPServer.server_activate(self)


    def handle_request(self):
        return socketserver.UDPServer.handle_request(self)


def main():
    logging.basicConfig(filename=LOG_FILENAME, format='%(asctime)s %(levelname)s:%(message)s', level=logging.DEBUG)

    init_db = {}

    db = InMemoryDB(init_db)
    sun_db = ssp.SunRiseSunSetDB(os.path.join(localconfig.HOME_DIR, 'sun_rise_set.txt'))

    server = MyServer((SERVER_HOST, SERVER_PORT), db, sun_db, MyUDPHandler)

    while True:
        # this is relict from having second Arduino connected to the serial port
        input_ready, output_ready, except_ready = select.select([server], [], [])
        for s in input_ready:
            if s is server:
                logging.info('Receiving data on UDP server...')
                s.handle_request()
            #print("Transmission DB state:")
            #print(db)


if __name__ == '__main__':
    main()
