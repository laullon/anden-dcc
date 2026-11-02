#!/usr/bin/env python3
"""Update an already-installed anden ESP13 OTA build and confirm its reboot."""
import argparse
import json
from pathlib import Path
import re
import time
import urllib.error
import urllib.request
import uuid

APP_ID = b'ANDEN_ESP13_OTA_V1'
IMAGE_LIMIT = 448 * 1024


def validate_image(data, capacity=IMAGE_LIMIT):
    if len(data) < 4 or data[0] != 0xE9 or data[3] >> 4 != 2:
        raise ValueError('Expected an ESP8266 .bin with the 1 MB flash profile')
    if APP_ID not in data:
        raise ValueError('Image does not identify as anden ESP13 OTA firmware')
    if len(data) > min(IMAGE_LIMIT, capacity):
        raise ValueError('Image exceeds the ESP13 OTA image limit')


def status(base):
    with urllib.request.urlopen(base + '/api/status', timeout=5) as response:
        current = json.load(response)
    if (current.get('name') != 'anden-esp13' or
            current.get('platform') != 'esp8266' or
            current.get('appId') != APP_ID.decode() or
            current.get('layoutBytes') != 1024 * 1024 or
            not isinstance(current.get('bootId'), int)):
        raise ValueError('Target is not the compatible anden ESP13 OTA firmware')
    return current


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', default='anden-esp13.local')
    parser.add_argument('--file', type=Path, required=True)
    parser.add_argument('--expect-version', required=True)
    parser.add_argument('--dry-run', action='store_true', help='Check target/image without uploading')
    args = parser.parse_args()
    if args.file.suffix.lower() != '.bin':
        parser.error('Use the ESP13 .bin file')
    image = args.file.read_bytes()
    validate_image(image)
    base = 'http://' + args.host.rstrip('/')
    previous = status(base)
    validate_image(image, previous.get('otaMaxBytes', 0))
    print('Target:', previous['name'], 'v' + previous['version'], 'image:', len(image), 'bytes', flush=True)
    if args.dry_run:
        print('Preflight passed; no upload sent.')
        return
    boundary = 'anden-esp13-' + uuid.uuid4().hex
    body = (f'--{boundary}\r\nContent-Disposition: form-data; name="firmware"; filename="esp13.bin"\r\n'
            'Content-Type: application/octet-stream\r\n\r\n').encode()
    body += image + f'\r\n--{boundary}--\r\n'.encode()
    request = urllib.request.Request(base + '/update', data=body,
        headers={'Content-Type': f'multipart/form-data; boundary={boundary}'})
    with urllib.request.urlopen(request, timeout=120) as response:
        result = response.read().decode(errors='replace')
    if 'Update Success!' not in result:
        raise RuntimeError('Updater rejected image: ' + re.sub('<[^>]*>', ' ', result).strip()[:300])
    print('Upload accepted. Waiting for reboot…', flush=True)
    for _ in range(35):
        time.sleep(2)
        try:
            current = status(base)
        except (OSError, ValueError, urllib.error.URLError):
            continue
        if current['bootId'] != previous['bootId']:
            if current.get('version') != args.expect_version:
                raise RuntimeError('ESP restarted, but the expected version is not running')
            print('Verified reboot and firmware v' + current['version'], flush=True)
            return
    raise RuntimeError('Reboot not confirmed. Reconnect to anden-esp13 or check the board IP.')


if __name__ == '__main__':
    main()
