import datetime
import os

import localconfig


TRANSMISSION_DB_FILENAME = os.path.join(localconfig.HOME_DIR, 'transmissions.db')
LOG_FILENAME =             os.path.join(localconfig.HOME_DIR, 'light_switch_server.log')
LOG_TIMEOUT_FILENAME =     os.path.join(localconfig.HOME_DIR, 'light_timeout.log')

STAIRCASE_DOOR_MAGNET_ID   = 16256053
STAIRCASE_TIMEOUT_MINUTES  = 1
STAIRCASE_SUN_OFFSET_HOURS = 1


def write_transmission_db_to_file(transmission_db, db_filename, debug_print=False):
    if debug_print:
        print("Writing DB...")
    with open(db_filename, 'w') as db_file:
        for magnet_id in transmission_db:
            if debug_print:
                print("Writing: {:08d} -> {}".format(magnet_id, transmission_db[magnet_id]))
            db_row = '{:08d};{}\n'.format(magnet_id, transmission_db[magnet_id])
            db_file.write(db_row)

    if debug_print:
        print("Writing DB... DONE.")


def read_transmission_db(db_filename):
    transmission_db = {}

    with open(db_filename, 'r') as db_file:
        for db_row in db_file:
            db_row = db_row.strip().rstrip()
            magnet_id, last_time = db_row.split(';')
            
            magnet_id = int(magnet_id)
            last_time = datetime.datetime.strptime(last_time, '%Y-%m-%d %H:%M:%S.%f')

            transmission_db[magnet_id] = last_time

    return transmission_db


class InMemoryDB:

    def __init__(self, init_values={}):
        self.db = init_values

    def get_value(self, key):
        if key in self.db:
            return self.db[key]
        else:
            return None

    def get_keys(self):
        return self.db.keys()

    def set_value(self, key, value):
        self.db[key] = value

    def delete_value(self, key):
        if key in self.db:
            del self.db[key]

    def __str__(self):
        db_content_to_str = ( "{} -> {} (type: {})".format(key, self.db[key], type(self.db[key])) for key in self.db )
        return '\n'.join(db_content_to_str)
