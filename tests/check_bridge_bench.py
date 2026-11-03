#!/usr/bin/env python3
"""Check the no-load pulse policy and independent half-bit guard."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='anden-bridge-') as temp:
    source=Path(temp)/'test.cpp';exe=Path(temp)/'test'
    source.write_text(r'''
#include <cassert>
#include "BridgeBench.h"
int main(){
 BridgeBench b;assert(!b.active()&&b.remaining(0)==0);
 assert(!b.start(true,0,100,true));assert(b.state==BridgeBench::RawTrip);
 assert(!b.start(true,0,0,false));assert(b.state==BridgeBench::TimingFault);
 assert(b.start(true,100,0,true));assert(b.direction&&b.remaining(100)==3000);
 assert(!b.start(false,200,0,true));assert(b.direction&&b.remaining(200)==2900);
 b.poll(3099,99,true,false);assert(b.active());b.poll(3100,0,true,false);assert(b.state==BridgeBench::Complete);
 assert(b.start(false,4000,0,true));assert(!b.direction);b.stop();assert(b.state==BridgeBench::Stopped);
 assert(b.start(true,5000,0,true));b.poll(5001,100,true,false);assert(b.state==BridgeBench::RawTrip);b.stop();assert(b.state==BridgeBench::RawTrip);
 assert(b.start(true,6000,0,true));b.poll(6001,0,false,false);assert(b.state==BridgeBench::TimingFault);
 assert(b.start(true,7000,0,true));b.poll(7001,0,true,true);assert(b.state==BridgeBench::GuardExpired);
 assert(b.start(false,UINT32_MAX-999,0,true));b.poll(1999,0,true,false);assert(b.active()&&b.remaining(1999)==1);b.poll(2000,0,true,false);assert(b.state==BridgeBench::Complete);
 assert(!b.start(4,3000,0,true));assert(!b.active());
 assert(b.start(BridgeBench::Dcc,3000,0,true));assert(b.mode==BridgeBench::Dcc);assert(!b.start(BridgeBench::DcForward,3100,0,true));assert(b.mode==BridgeBench::Dcc);
 b.poll(6000,0,true,false);assert(b.state==BridgeBench::Complete);
 assert(b.start(BridgeBench::Track,10000,0,true));assert(b.remaining(10000)==2000);
 b.poll(11999,0,true,false);assert(b.active());assert(b.renew(11999));
 b.poll(13998,0,true,false);assert(b.active());b.poll(13999,0,true,false);assert(b.state==BridgeBench::LeaseExpired);assert(!b.renew(14000));
 assert(b.start(BridgeBench::Track,20000,0,true));b.poll(20001,100,true,false);assert(b.state==BridgeBench::RawTrip);assert(!b.renew(20002));
 assert(b.start(BridgeBench::Track,UINT32_MAX-999,0,true));b.poll(1000,0,true,false);assert(b.state==BridgeBench::LeaseExpired);
 assert(b.start(BridgeBench::Track,30000,0,true));assert(!b.renew(32000));assert(b.state==BridgeBench::LeaseExpired);
 BridgeGuard g;assert(!g.tick());g.arm();for(unsigned i=1;i<60000;++i)assert(!g.tick());assert(g.tick()&&g.expired);assert(!g.tick());
 g.arm();assert(!g.expired);g.disarm();for(unsigned i=0;i<60001;++i)assert(!g.tick());
 assert(uint32_t(BridgeGuard::HALVES)*100<=6000000UL);
}
''')
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++11','-I',str(root/'experiments/uno-stack-test'),str(source),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: bridge default-off, fixed 3-second DC/DCC modes and no-extension pulse, manual stop, raw cutoff, timing/guard faults, rollover and independent half-bit guard.')
