import os,termios,fcntl,struct,time,select
import argparse
from pathlib import Path
parser=argparse.ArgumentParser()
parser.add_argument('--monitor',action='store_true')
parser.add_argument('--seconds',type=int,default=22)
parser.add_argument('--port',default='/dev/cu.usbserial-110')
parser.add_argument('--log',default='/tmp/anden-uno-esp13-probe.log')
args=parser.parse_args()
port=args.port
fd=os.open(port,os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
try:
 fcntl.ioctl(fd,termios.TIOCEXCL)
 attrs=termios.tcgetattr(fd)
 attrs[0]=attrs[1]=attrs[3]=0
 attrs[2]=termios.CS8|termios.CREAD|termios.CLOCAL
 attrs[4]=attrs[5]=termios.B115200
 attrs[6][termios.VMIN]=attrs[6][termios.VTIME]=0
 termios.tcsetattr(fd,termios.TCSANOW,attrs)
 termios.tcflush(fd,termios.TCIOFLUSH)
 mask=struct.pack('I',termios.TIOCM_DTR|termios.TIOCM_RTS)
 fcntl.ioctl(fd,termios.TIOCMBIC,mask);time.sleep(.1)
 fcntl.ioctl(fd,termios.TIOCMBIS,mask)
 raw=b'';pending=b'';deadline=time.monotonic()+args.seconds
 while time.monotonic()<deadline:
  if not select.select([fd],[],[],.3)[0]:continue
  try:chunk=os.read(fd,4096)
  except BlockingIOError:continue
  raw+=chunk;pending+=chunk
  while b'\n' in pending:
   line,pending=pending.split(b'\n',1)
   # Only our structured diagnostic is printed; save raw data privately.
   for marker in [b'ANDEN_UNO_',b'PROBE_BEGIN',b'PROBE_RESULT',b'PROBE_DONE',b'BOOT_MONITOR_READY',b'BOOT_CAPTURE',b'ROM_SYNC']:
    if marker in line:
     print(line[line.index(marker):].decode('ascii','replace').strip(),flush=True)
  if not args.monitor and b'PROBE_DONE' in raw:break
 Path(args.log).write_bytes(raw)
 if b'PROBE_DONE' not in raw:print('No complete diagnostic received; raw bytes:',len(raw),flush=True)
finally:
 os.close(fd)
