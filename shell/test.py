import argparse
from enum import Enum
import os
import serial
import struct
import sys
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

def make_msg(kind, data):
    length = len(data)
    crc = zlib.crc32(data)
    return struct.pack('<Bi{0}sI'.format(length), kind, length, data, crc)

def send_bytes(bytes, offset, msg_type):
    offset_bytes = offset.to_bytes(4, 'little', signed=False)
    # print("binary length: {0}".format(hex(len(bytes))))
    payload = offset_bytes + bytes
    # print("total bytes: {0}".format(hex(len(payload))))
    to_ser = make_msg(msg_type, payload)
    # print("to ser: {0}".format(hex(len(to_ser))))
    SER.write(to_ser)
    # get_gbaser_reply()

def send_gbaser_string(cmd):
    ascii = cmd.encode('ascii', 'ignore') + b'\r\n'
    print("ascii var", ascii)
    msg_type = Mtype.string.value # string
    print("msg length: {0}".format(len(ascii)))
    to_ser = make_msg(msg_type, ascii)
    print(to_ser)
    print("to ser: {0}".format(len(to_ser)))
    SER.write(to_ser)

if __name__ == "__main__":
    print("hello")
    init("/dev/tty.usbserial-FTHIY1SZ", 115200, False)
    # send_gbaser_string("hellooo")
    # values = bytearray(b'\x80#@')
    # SER.write(values)
    # send_bytes(values, 0, Mtype.undefined.value)        
    send_bytes(b'\x40', 0x00000000, 0x01)

