#!/usr/bin/env python3
"""Upload a compatible anden-dcc .bin and verify its version after restart."""
import argparse
import json
from pathlib import Path
import time
import urllib.error
import urllib.request
import uuid


def status(base):
    with urllib.request.urlopen(base+'/api/status', timeout=5) as response:
        data=json.load(response)
    if data.get('name') != 'anden-dcc':
        raise RuntimeError('Target did not identify itself as anden-dcc')
    return data


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host',default='anden-dcc.local')
    parser.add_argument('--file',type=Path,required=True)
    parser.add_argument('--expect-version',required=True)
    args=parser.parse_args()
    if args.file.suffix.lower()!='.bin':
        parser.error('OTA needs .bin, not .uf2')
    firmware=args.file.read_bytes()
    if not firmware:
        parser.error('Empty firmware file')
    base='http://'+args.host.rstrip('/')
    previous=status(base)
    print('Target:',previous['name'],'running',previous['version'],flush=True)
    boundary='anden-'+uuid.uuid4().hex
    body=(f'--{boundary}\r\nContent-Disposition: form-data; name="firmware"; filename="firmware.bin"\r\nContent-Type: application/octet-stream\r\n\r\n'.encode()+firmware+f'\r\n--{boundary}--\r\n'.encode())
    request=urllib.request.Request(base+'/update',data=body,headers={'Content-Type':f'multipart/form-data; boundary={boundary}'})
    with urllib.request.urlopen(request,timeout=60) as response:
        result=response.read().decode(errors='replace')
    if 'Update Success' not in result:
        raise RuntimeError('Updater did not report success: '+result[:300])
    print('Upload accepted. Waiting for restart…',flush=True)
    for attempt in range(12):
        time.sleep(2)
        try:
            current=status(base)
            if current.get('version')==args.expect_version:
                print('Verified:',current['version'],'Wi-Fi connected:',current['connected'],flush=True)
                return 0
        except (OSError,ValueError,urllib.error.URLError):
            pass
    raise RuntimeError('Expected version not confirmed after upload')


if __name__=='__main__':
    raise SystemExit(main())
