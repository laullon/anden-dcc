"""Synthetic Arduino ESP8266 image metadata for OTA regression checks."""
import struct

APP_ID = b'ANDEN_ESP13_OTA_V1'


def seal_image(data):
    """Fill size and CRC independently of the production table-driven helper."""
    data = bytearray(data)
    if len(data) < 4120:
        raise ValueError('Fixture needs room for the Arduino image metadata')
    data[4112:4120] = b'\0' * 8
    crc = 0xFFFFFFFF
    for value in data:
        crc ^= value << 24
        for _ in range(8):
            crc = ((crc << 1) ^ (0x04C11DB7 if crc & 0x80000000 else 0)) & 0xFFFFFFFF
    struct.pack_into('<II', data, 4112, len(data), crc)
    return bytes(data)


def make_image(size=4176):
    if size < 4120 + len(APP_ID):
        raise ValueError('Fixture needs room for its application identifier')
    data = bytearray(size)
    data[:4] = bytes([0xE9, 2, 3, 0x20])
    data[4096:4100] = bytes([0xE9, 5, 3, 0x20])
    data[4120:4120 + len(APP_ID)] = APP_ID
    return seal_image(data)
