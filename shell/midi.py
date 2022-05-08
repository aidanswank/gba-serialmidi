#!/usr/bin/env python
#
# midiin_callback.py
#
"""Show how to receive MIDI input by setting a callback function."""

from __future__ import print_function

import logging
import sys
import time

##### gba stuff
import argparse
from enum import Enum
import os
import serial
import struct
import zlib

class Mtype(Enum):
    undefined = 0x00
    string = 0x01
    binary = 0x02
    multiboot = 0x03
    ret_ok = 0xff
    ret_error = 0xfe
    ret_crc_error = 0xfd

def init(port, baudrate, rtscts):
    global SER
    SER = serial.Serial(port, baudrate, timeout=1, rtscts=rtscts)

def send_bytes(bytes, offset, msg_type):
    offset_bytes = offset.to_bytes(4, 'little', signed=False)
    # print("binary length: {0}".format(hex(len(bytes))))
    payload = offset_bytes + bytes
    # print("total bytes: {0}".format(hex(len(payload))))
    to_ser = make_msg(msg_type, payload)
    # print("to ser: {0}".format(hex(len(to_ser))))
    SER.write(to_ser)
    # get_gbaser_reply()

def make_msg(kind, data):
    length = len(data)
    crc = zlib.crc32(data)
    return struct.pack('<Bi{0}sI'.format(length), kind, length, data, crc)

def send_gbaser_string(cmd):
    ascii = cmd.encode('ascii', 'ignore') + b'\r\n'
    msg_type = Mtype.string.value # string
    print("msg length: {0}".format(len(ascii)))
    to_ser = make_msg(msg_type, ascii)
    print("to ser: {0}".format(len(to_ser)))
    SER.write(to_ser)

#### end gba

from rtmidi.midiutil import open_midiinput

log = logging.getLogger('midiin_callback')
logging.basicConfig(level=logging.DEBUG)


class MidiInputHandler(object):
    def __init__(self, port):
        self.port = port
        self._wallclock = time.time()

    def __call__(self, event, data=None):
        message, deltatime = event
        self._wallclock += deltatime
        # print("[%s] @%0.6f %r" % (self.port, self._wallclock, message))
        # print(message[0])
        # str1=''
        # for x in message:
        #     str1 += chr(x);
        # print(str1)
        str2=''
        for x in message:
            str2 += str(x)+',';
        # print(str2)
        if message[0]==144 or message[0]==145 or message[0]==146 or message[0]==147:
            values = bytearray(message)
            print(values)
            # SER.write(values)
            time.sleep(0.004 );
            # send_gbaser_string(str2)
            send_bytes(values, 0x00000000, 0x01)



init("/dev/tty.usbserial-FTHIY1SZ", 115200, False)

# Prompts user for MIDI input port, unless a valid port number or name
# is given as the first argument on the command line.
# API backend defaults to ALSA on Linux.
port = sys.argv[1] if len(sys.argv) > 1 else None

try:
    midiin, port_name = open_midiinput(port)
except (EOFError, KeyboardInterrupt):
    sys.exit()

print("Attaching MIDI input callback handler.")
midiin.set_callback(MidiInputHandler(port_name))

print("Entering main loop. Press Control-C to exit.")
try:
    # Just wait for keyboard interrupt,
    # everything else is handled via the input callback.
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print('')
finally:
    print("Exit.")
    midiin.close_port()
    del midiin