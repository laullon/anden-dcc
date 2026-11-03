const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../..');
(async()=>{const browser=await chromium.launch({...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{}),headless:true});try{
 const page=await browser.newPage({viewport:{width:1180,height:1000}}),errors=[];page.on('pageerror',e=>errors.push(e.message));
 const state={appId:'ANDEN_ESP13_OTA_V1',version:'0.5.0',connected:true,configured:true,ssid:'Workshop',ip:'192.168.1.43',apIp:'192.168.13.1',apReady:true,otaMaxBytes:458752,freeHeap:40000,storageReady:true,uno:{online:true,version:'0.5.0',trackSupported:true,trackActive:false,enableA:false,enableB:false,adcA:0,adcB:0,rxFrames:5,roundTripMs:8,bench:{supported:true,dccSupported:true,active:false,state:0,mode:0,remainingMs:0}},dcc:{ready:true,running:true,packets:100,timingFaults:0,locos:[{id:1,address:3,type:'electric',f0:false,forward:true,speed:0},{id:2,address:4,type:'diesel',f0:false,forward:true,speed:0}]}};
 let on=0,off=0,keeps=0,refuseKeep=false,lease=0,settleUntil=0;
 await page.route('http://esptrack.test/**',async route=>{
  const req=route.request(),u=new URL(req.url()),data=new URLSearchParams(req.postData());
  if(u.pathname==='/api/status'){if(state.dcc.finder)state.dcc.finder.waitMs=Math.max(0,settleUntil-Date.now());return route.fulfill({json:state})}
  if(u.pathname==='/api/track/on'){on++;lease++;state.uno.enableA=state.uno.trackActive=true;state.uno.bench={supported:true,dccSupported:true,active:true,state:1,mode:3,remainingMs:2000};return route.fulfill({json:{lease}})}
  if(u.pathname==='/api/track/off'){off++;state.uno.enableA=state.uno.trackActive=state.uno.bench.active=false;state.uno.bench.state=3;return route.fulfill({json:{ok:true}})}
  if(u.pathname==='/api/track/keep'){keeps++;assert.equal(data.get('lease'),String(lease));return route.fulfill(refuseKeep?{status:409,json:{error:'Lease expired'}}:{json:{ok:true}})}
  if(u.pathname==='/dcc/direction'){const l=state.dcc.locos.find(l=>l.address===Number(data.get('address')));l.forward=data.get('forward')==='1';assert.equal(l.speed,0);return route.fulfill({json:state.dcc})}
  if(u.pathname==='/dcc/f0'){state.dcc.locos.find(l=>l.address===Number(data.get('address'))).f0=data.get('on')==='1';return route.fulfill({json:state.dcc})}
  if(u.pathname==='/finder/start'){
   state.dcc.finder={stage:'verifyOn',active:true,first:Number(data.get('first')),last:Number(data.get('last')),low:Number(data.get('first')),high:Number(data.get('last')),step:1,result:0,waitMs:100};settleUntil=Date.now()+100;state.dcc.ready=false;return route.fulfill({json:state.dcc});
  }
  if(u.pathname==='/finder/answer'){
   const f=state.dcc.finder;assert.equal(data.get('step'),String(f.step));const answer=data.get('answer');
   if(answer!=='retry'){
    if(f.stage==='verifyOn')f.stage='verifyOff';else if(f.stage==='verifyOff')f.stage='split';
    else if(f.stage==='split'){if(answer==='on')f.high=f.middle;else f.low=f.middle+1;if(f.low===f.high)f.stage='confirmOn';}
    else if(f.stage==='confirmOn')f.stage='confirmOff';else if(f.stage==='confirmOff'){f.stage='found';f.active=false;f.result=f.low;state.dcc.ready=true;}
   }
   f.middle=Math.floor((f.low+f.high)/2);f.step++;f.waitMs=100;settleUntil=Date.now()+100;return route.fulfill({json:state.dcc});
  }
  if(u.pathname==='/finder/cancel'){state.dcc.finder={stage:'idle',active:false,step:state.dcc.finder.step+1};state.dcc.ready=true;return route.fulfill({json:state.dcc})}
  return route.fulfill({contentType:'text/html' ,body:fs.readFileSync(path.join(root,'experiments/esp13-ota/web/index.html'),'utf8')});
 });
 await page.goto('http://esptrack.test/');await page.waitForFunction(()=>document.querySelectorAll('.loco').length===2);
 assert.equal(on,0);assert(await page.locator('#track-on').isEnabled());
 await page.locator('.loco[data-id="1"] [data-forward="0"]').click();assert.equal(state.dcc.locos[0].forward,false);assert.equal(state.dcc.locos[1].forward,true);
 await page.locator('#track-on').click();await page.waitForFunction(()=>document.getElementById('track-state').textContent.includes('Power on'));
 await page.waitForFunction(()=>document.getElementById('track-on').disabled);await page.waitForTimeout(1100);assert(keeps>=1);assert.equal(on,1);
 assert(await page.locator('.loco[data-id="1"] [data-forward="1"]').isEnabled());assert(await page.locator('.edit-loco').first().isDisabled());assert(await page.locator('#add-loco').isDisabled());
 await page.locator('.loco[data-id="1"] [data-forward="1"]').click();assert.equal(state.dcc.locos[0].forward,true);
 await page.locator('.loco[data-id="2"] [data-on="1"]').click();assert.equal(state.dcc.locos[1].f0,true);
 fs.mkdirSync(path.join(root,'build/ui'),{recursive:true});await page.screenshot({path:path.join(root,'build/ui/esp13-track.png')});
 await page.locator('#track-off').click();await page.waitForFunction(()=>document.getElementById('track-state').textContent==='Power off');const stopped=keeps;await page.waitForTimeout(700);assert.equal(keeps,stopped);assert(off>=1);
 await page.locator('#track-on').click();refuseKeep=true;await page.waitForFunction(()=>document.getElementById('track-message').textContent.includes('Connection lost'));
 await page.waitForTimeout(700);assert.equal(state.uno.trackActive,false);assert.equal(on,2);
 refuseKeep=false;state.uno.trackSupported=false;await page.evaluate(()=>refreshStatus());assert(await page.locator('#track-on').isDisabled());assert(await page.locator('[data-forward="1"]').first().isDisabled());
 state.uno.trackSupported=true;await page.evaluate(()=>refreshStatus());await page.locator('#track-on').click();await page.evaluate(()=>leaveTrack());await page.waitForTimeout(100);assert.equal(state.uno.trackActive,false);
 await page.setViewportSize({width:320,height:1100});assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
 await page.evaluate(()=>refreshStatus());await page.locator('#track-on').click();await page.locator('#find-address').click();await page.locator('#finder-range button').click();
 await page.waitForFunction(()=>document.getElementById('finder-question').textContent.includes('turn the lights on'));
 assert(await page.locator('.loco [data-on="1"]').first().isDisabled());
 assert(await page.locator('[data-finder-answer="on"]').isDisabled());
 async function answer(value){await page.waitForFunction(v=>!document.querySelector('[data-finder-answer="'+v+'"]').disabled,value);await page.locator('[data-finder-answer="'+value+'"]').click();}
 await answer('on');await answer('off');
 while(state.dcc.finder.stage==='split')await answer(37<=state.dcc.finder.middle?'on':'off');
 await answer('on');await answer('off');assert.equal(state.dcc.finder.result,37);
 assert(await page.locator('#finder-add').isDisabled());await page.locator('#track-off').click();await page.waitForFunction(()=>!document.getElementById('finder-add').disabled);assert(await page.locator('#finder-add').isEnabled());
 await page.locator('#finder-add').click();assert.equal(await page.locator('#loco-address').inputValue(),'37');await page.locator('#cancel-loco').click();
 await page.locator('#finder-again').click();assert.equal(state.dcc.finder.stage,'idle');
 assert.deepEqual(errors,[]);console.log('PASS: stationary track explicit-on/heartbeat/off/expiry/no-restart, per-loco direction/F0 while powered, metadata locks, old-firmware guards, leave-page off, mobile layout and guided search to 37 with settling/locks/result editor.');
}finally{await browser.close()}})().catch(e=>{console.error(e);process.exit(1)});
