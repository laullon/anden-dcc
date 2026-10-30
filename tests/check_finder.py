#!/usr/bin/env python3
"""Exercise the production search state and transmitted address/function words."""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'finder.cpp';exe=Path(d)/'finder'
 p.write_text(r'''
#include "AddressFinder.h"
#include <cassert>
#include <cstdio>
#include <algorithm>
using S=AddressFinder::Stage;
void check(const AddressFinder& f) {
 uint32_t w[128];f.schedule(w);
 bool seen[128]={};
 for(auto v:w) {
  unsigned a=v>>24, instruction=(v>>15)&255, checksum=(v>>6)&255;
  assert(a>=f.first && a<=f.last && a>=1 && a<=127);seen[a]=true;
  bool on=f.stage==S::VerifyOn || (f.stage==S::Split && a>=f.low && a<=f.middle()) || (f.stage==S::ConfirmOn && a==f.low);
  assert(instruction==(on?0x90:0x80));assert((a^instruction^checksum)==0);
 }
 for(unsigned a=f.first;a<=f.last;++a)assert(seen[a]);
}
void find(unsigned lo,unsigned hi,unsigned target,uint32_t now=0) {
 AddressFinder f;assert(f.start(lo,hi,now));check(f);
 assert(!f.answer(true,f.step,now+3999));
 auto token=f.step;assert(f.answer(true,token,now+=4000));check(f);
 assert(!f.answer(false,token,now+4000)); // A delayed duplicate cannot advance.
 assert(f.answer(false,f.step,now+=4000));check(f);
 unsigned questions=0;
 while(f.stage==S::Split) {
  auto old=f.step;assert(f.retry(old,now+=4000));assert(f.low<=target&&f.high>=target);check(f);
  assert(!f.answer(true,old,now+4000));
  assert(f.answer(target<=f.middle(),f.step,now+=4000));check(f);questions++;
 }
 assert(questions<=7);assert(f.stage==S::ConfirmOn&&f.low==target);
 assert(f.answer(true,f.step,now+=4000));assert(f.stage==S::ConfirmOff);check(f);
 assert(f.answer(false,f.step,now+=4000));assert(f.stage==S::Cleanup);check(f);
 assert(f.active());f.tick(now+3999);assert(f.active());f.tick(now+4000);
 assert(!f.active()&&f.stage==S::Found&&f.result==target);
}
int main() {
 for(unsigned lo=1;lo<=127;++lo) for(unsigned hi=lo;hi<=127;++hi)
  for(unsigned a:{lo,(lo+hi)/2,hi}) find(lo,hi,a);
 for(unsigned a=1;a<=127;++a)find(1,127,a);
 for(unsigned a=1;a<=100;++a)find(1,100,a);
 find(1,127,127,0xfffff000u); // Wrap-safe waits.
 AddressFinder f;assert(!f.start(0,100,0));assert(!f.start(1,128,0));assert(!f.start(80,20,0));
 assert(f.start(1,100,0));assert(!f.start(1,5,1));assert(f.answer(false,f.step,4000));
 assert(f.stage==S::Cleanup&&!f.result);check(f);f.tick(8000);assert(f.stage==S::Failed);
 assert(f.start(3,5,9000));f.cancel(9001);check(f);f.tick(13001);assert(!f.active());
 f.dismiss();assert(f.stage==S::Idle&&!f.result);
 assert(f.start(1,127,20000));f.dismiss();assert(f.stage==S::VerifyOn);assert(f.tick(20000+AddressFinder::TIMEOUT_MS));check(f);
 f.tick(24000+AddressFinder::TIMEOUT_MS);assert(f.stage==S::Failed);
 assert(f.start(1,1,300000));assert(f.answer(true,f.step,304000));assert(f.answer(true,f.step,308000));assert(f.stage==S::Cleanup);check(f);
 for(bool failOn:{true,false}) {
  AddressFinder one;assert(one.start(37,37,0));
  assert(one.answer(true,one.step,4000));assert(one.answer(false,one.step,8000));
  assert(one.stage==S::ConfirmOn);
  if(failOn)assert(one.answer(false,one.step,12000));
  else {assert(one.answer(true,one.step,12000));assert(one.answer(true,one.step,16000));}
  assert(one.stage==S::Cleanup&&!one.result);check(one);
 }
 std::puts("PASS: every short-address range (endpoints/midpoint), every target in 1-100 and 1-127; full-range ON/OFF coverage, excluded-address OFF, retry/stale tokens, settling, singletons, confirmation failures, cancellation, timeout and timer wrap.");
}
''')
 subprocess.run(['c++','-std=c++17','-O2','-I',str(root/'firmware/anden-dcc'),str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
