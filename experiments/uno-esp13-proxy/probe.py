#!/usr/bin/env python3
"""Read-only host/Uno/ESP checks for the experimental shared-UART proxy."""
import argparse
import secrets
import struct
import time
import zlib
import serial

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--port',default='/dev/cu.usbserial-110')
args=parser.parse_args()

def exchange(port,sequence,payload,operation=0,wait=1000):
    header=struct.pack('>HHHBB',sequence,len(payload),wait,0,operation)
    crc=zlib.crc32(header+payload)
    request=b'!A1'+(header+struct.pack('>I',crc)+payload).hex().upper().encode()+b'\n'
    port.write(request);port.flush()
    pending=bytearray();deadline=time.monotonic()+wait/1000+4
    while time.monotonic()<deadline:
        pending.extend(port.read(4096))
        while b'\n' in pending:
            line,_,rest=pending.partition(b'\n');pending=bytearray(rest)
            for tag in (b'!R1',b'!E1'):
                pos=line.find(tag)
                if pos<0:continue
                line=line[pos:].strip()
                if line[:3]==b'!E1':raise RuntimeError(line.decode(errors='replace'))
                raw=bytes.fromhex(line[3:].decode())
                seq,size,checksum=struct.unpack('>HHI',raw[:8])
                if seq!=sequence:continue
                data=raw[8:]
                if len(data)!=size or zlib.crc32(raw[:4]+data)!=checksum:
                    raise RuntimeError('Proxy report integrity failed')
                return data
    raise TimeoutError('No checked Uno proxy reply received')

def main():
    with serial.Serial(args.port,115200,timeout=.1,exclusive=True) as port:
        # Explicit CH340 control-line reset; opening alone is inconsistent.
        port.dtr=False;port.rts=False;time.sleep(.1)
        port.dtr=True;port.rts=True
        time.sleep(5);port.reset_input_buffer()
        token=secrets.token_bytes(32)
        reply=exchange(port,1,token)
        if reply!=token:raise RuntimeError('Host round trip did not match')
        print('PASS: Mac → Uno → Mac packet integrity, shield still stacked.',flush=True)
        sync=b'\xC0'+bytes([0,8,36,0,0,0,0,0,7,7,0x12,0x20])+b'\x55'*32+b'\xC0'
        response=exchange(port,2,sync,operation=1)
        frames=[part.replace(b'\xDB\xDC',b'\xC0').replace(b'\xDB\xDD',b'\xDB') for part in response.split(b'\xC0') if part]
        valid=[frame for frame in frames if len(frame)>=10 and frame[:2]==b'\x01\x08' and frame[-2:]==b'\0\0']
        print('ESP ROM SYNC:', 'OK' if valid else 'NO VALID REPLY', 'response bytes:',len(response),flush=True)

if __name__=='__main__':main()
