const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../..');
(async()=>{const browser=await chromium.launch({...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{}),headless:true});try{
 const page=await browser.newPage({viewport:{width:1180,height:1000}}),errors=[];page.on('pageerror',e=>errors.push(e.message));
 const state={appId:'ANDEN_ESP13_OTA_V1',version:'0.5.0',connected:true,configured:true,ssid:'Workshop',ip:'192.168.1.43',apIp:'192.168.13.1',apReady:true,otaMaxBytes:458752,freeHeap:40000,storageReady:true,uno:{online:true,version:'0.5.0',trackSupported:true,trackActive:false,enableA:false,enableB:false,adcA:0,adcB:0,rxFrames:5,roundTripMs:8,bench:{supported:true,dccSupported:true,active:false,state:0,mode:0,remainingMs:0}},dcc:{ready:true,running:true,packets:100,timingFaults:0,locos:[{id:1,address:3,type:'electric',f0:false,forward:true,speed:0},{id:2,address:4,type:'diesel',f0:false,forward:true,speed:0}]}};
 let on=0,off=0,keeps=0,refuseKeep=false,lease=0,settleUntil=0,failNextKeep=false;
 await page.route('http://esptrack.test/**',async route=>{
  const req=route.request(),u=new URL(req.url()),data=new URLSearchParams(req.postData());
  if(u.pathname==='/api/status'){if(state.dcc.finder)state.dcc.finder.waitMs=Math.max(0,settleUntil-Date.now());return route.fulfill({json:state})}
  if(u.pathname==='/api/track/on'){await new Promise(r=>setTimeout(r,350));on++;lease++;state.uno.enableA=state.uno.trackActive=true;state.uno.bench={supported:true,dccSupported:true,active:true,state:1,mode:3,remainingMs:2000};return route.fulfill({json:{lease}})}
  if(u.pathname==='/api/track/off'){if(data.has('lease')&&data.get('lease')!==String(lease))return route.fulfill({status:409,json:{error:'Stale session'}});await new Promise(r=>setTimeout(r,350));off++;state.uno.enableA=state.uno.trackActive=state.uno.bench.active=false;state.uno.bench.state=3;return route.fulfill({json:{ok:true}})}
  if(u.pathname==='/api/track/keep'){keeps++;if(failNextKeep){failNextKeep=false;return route.abort('failed')}if(refuseKeep){state.uno.enableA=state.uno.trackActive=state.uno.bench.active=false;}assert.equal(data.get('lease'),String(lease));return route.fulfill(refuseKeep?{status:409,json:{error:'Lease expired'}}:{json:{ok:true}})}
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
 await page.locator('#find-address').click();assert(await page.locator('#finder-first').isEditable());assert(await page.locator('#finder-last').isEditable());assert(await page.locator('#finder-range button').isDisabled());
 await page.locator('#finder-first').fill('7');await page.locator('#finder-last').fill('90');await page.evaluate(()=>refreshStatus());assert.equal(await page.locator('#finder-first').inputValue(),'7');assert.equal(await page.locator('#finder-last').inputValue(),'90');
 state.uno.online=false;await page.evaluate(()=>refreshStatus());assert(await page.locator('#finder-first').isEditable());assert(await page.locator('#finder-range button').isDisabled());assert.equal(await page.locator('#finder-last').inputValue(),'90');
 state.uno.online=true;await page.evaluate(()=>refreshStatus());await page.locator('#finder-first').fill('1');await page.locator('#finder-last').fill('100');await page.locator('#finder-close').click();
 assert.equal(on,0);assert(await page.locator('#track-on').isEnabled());assert(await page.locator('#track-on').isVisible());assert(await page.locator('#track-off').isHidden());assert.equal(await page.locator('#track-label').innerText(),'Track is OFF');
 await page.locator('.loco[data-id="1"] [data-forward="0"]').click();assert.equal(state.dcc.locos[0].forward,false);assert.equal(state.dcc.locos[1].forward,true);
 await page.locator('#track-on').click();await page.waitForFunction(()=>document.getElementById('track-label').textContent==='Track is ON');
 await page.waitForFunction(()=>document.getElementById('track-on').disabled);await page.waitForTimeout(1100);assert(keeps>=1);assert.equal(on,1);assert(await page.locator('#track-on').isHidden());assert(await page.locator('#track-off').isVisible());
 // Polling must preserve a pressed mouse target until mouseup/click.
 const mouseTarget=page.locator('.loco[data-id="1"] [data-on="1"]');await mouseTarget.scrollIntoViewIfNeeded();const originalNode=await mouseTarget.elementHandle();const rect=await mouseTarget.boundingBox();
 assert(await originalNode.evaluate(el=>{const r=el.getBoundingClientRect();return document.elementFromPoint(r.x+r.width/2,r.y+r.height/2)===el}));await page.mouse.move(rect.x+rect.width/2,rect.y+rect.height/2);await page.mouse.down();
 for(let i=0;i<4;i++){state.dcc.packets++;await page.evaluate(()=>refreshStatus());assert(await originalNode.evaluate(e=>e.isConnected));}
 await page.mouse.up();await page.waitForFunction(()=>!rosterBusy&&document.querySelector('.loco[data-id="1"] [data-on="1"]').getAttribute('aria-pressed')==='true');
 // Page visibility is not a stop command; one lost HTTP response is retried.
 const beforeOff=off;await page.evaluate(()=>{Object.defineProperty(document,'hidden',{configurable:true,get:()=>true});document.dispatchEvent(new Event('visibilitychange'))});await page.waitForTimeout(700);assert.equal(off,beforeOff);assert(state.uno.trackActive);await page.evaluate(()=>delete document.hidden);
 failNextKeep=true;await page.waitForFunction(()=>document.getElementById('track-message').textContent.includes('retrying'));await page.waitForFunction(()=>document.getElementById('track-message').textContent==='');assert.equal(off,beforeOff);assert(state.uno.trackActive);
 assert(await page.locator('.loco[data-id="1"] [data-forward="1"]').isEnabled());assert(await page.locator('.edit-loco').first().isDisabled());assert(await page.locator('#add-loco').isDisabled());
 await page.locator('.loco[data-id="1"] [data-forward="1"]').click();assert.equal(state.dcc.locos[0].forward,true);
 await page.locator('.loco[data-id="2"] [data-on="1"]').click();assert.equal(state.dcc.locos[1].f0,true);
 // Each illustration swaps its white/red ends with direction, and goes dark with F0 off.
 const colors=()=>page.locator('.loco[data-id="1"]').evaluate(card=>({left:getComputedStyle(card.querySelector('.lamp-left')).fill,right:getComputedStyle(card.querySelector('.lamp-right')).fill}));
 await page.waitForFunction(()=>!rosterBusy);
 for(const kind of ['electric','diesel','steam']){
  state.dcc.locos[0].type=kind;state.dcc.locos[0].f0=true;state.dcc.locos[0].forward=true;await page.evaluate(()=>refreshStatus());const forward=await colors();assert.notEqual(forward.left,forward.right);
  await page.locator('.loco[data-id="1"] [data-forward="0"]').click();await page.waitForFunction(()=>!rosterBusy&&document.querySelector('.loco[data-id="1"]').dataset.forward==='false');const reverse=await colors();assert.equal(reverse.left,forward.right);assert.equal(reverse.right,forward.left);
  await page.locator('.loco[data-id="1"] [data-on="0"]').click();await page.waitForFunction(()=>!rosterBusy&&!document.querySelector('.loco[data-id="1"]').classList.contains('lights-on'));const dark=await colors();assert.equal(dark.left,dark.right);
 }
 state.dcc.locos[0].type='electric';state.dcc.locos[0].f0=true;state.dcc.locos[0].forward=false;await page.evaluate(()=>refreshStatus());
 fs.mkdirSync(path.join(root,'build/ui'),{recursive:true});
 await page.evaluate(()=>window.scrollTo(0,0));await page.screenshot({path:path.join(root,'build/ui/esp13-power-lights.png'),fullPage:true});await page.screenshot({path:path.join(root,'build/ui/esp13-track.png')});
 await page.locator('#track-off').click();await page.waitForFunction(()=>document.getElementById('track-label').textContent==='Track is OFF');const stopped=keeps;await page.waitForTimeout(700);assert.equal(keeps,stopped);assert(off>=1);assert(await page.locator('#track-off').isHidden());const darkPower=await colors();assert.equal(darkPower.left,darkPower.right);
 await page.locator('#track-on').click();const offBeforeReject=off;refuseKeep=true;await page.waitForFunction(()=>document.getElementById('track-message').textContent.includes('Connection lost'));
 await page.waitForTimeout(700);assert.equal(state.uno.trackActive,false);assert.equal(on,2);assert.equal(off,offBeforeReject);
 refuseKeep=false;state.uno.trackSupported=false;await page.evaluate(()=>refreshStatus());assert(await page.locator('#track-on').isDisabled());assert(await page.locator('[data-forward="1"]').first().isDisabled());
 state.uno.online=false;await page.evaluate(()=>refreshStatus());assert.equal(await page.locator('#track-label').innerText(),'Status unavailable');assert(await page.locator('#track-off').isVisible());assert(await page.locator('#track-on').isHidden());state.uno.online=true;
 state.uno.trackSupported=true;await page.evaluate(()=>refreshStatus());await page.locator('#track-on').click();await page.evaluate(()=>leaveTrack());await page.waitForTimeout(800);assert.equal(state.uno.trackActive,false);
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
 await page.waitForFunction(()=>!finderBusy&&document.getElementById('finder-current').hidden);
 // Delayed replies must not reveal two buttons or move the card/loco content.
 const layout=()=>page.evaluate(()=>{
  const box=e=>{const r=e.getBoundingClientRect();return [r.x+scrollX,r.y+scrollY,r.width,r.height]};
  const visible=[...document.querySelectorAll('.track-action button')].filter(e=>!e.hidden);
  return {power:box(document.getElementById('track-power')),locos:box(document.getElementById('loco-list')),button:box(visible[0]),buttons:visible.length};
 });
 for(const width of [1180,600,320]){
  await page.setViewportSize({width,height:1100});await page.evaluate(()=>{message('track-message','');renderTrack()});
  const before=await layout();await page.locator('#track-on').click();const pending=await layout();assert.equal(pending.buttons,1);
  await page.waitForFunction(()=>document.getElementById('track-label').textContent==='Track is ON');const powered=await layout();
  await page.locator('#track-off').click();const stopping=await layout();assert.equal(stopping.buttons,1);
  await page.waitForFunction(()=>document.getElementById('track-label').textContent==='Track is OFF');const after=await layout();
  for(const value of [pending,powered,stopping,after]){
   assert.equal(value.buttons,1);
   for(const key of ['power','button'])for(let i=0;i<4;i++)assert(Math.abs(value[key][i]-before[key][i])<1.1,JSON.stringify({width,key,before,value}));
   assert(Math.abs(value.locos[1]-before.locos[1])<1.1,JSON.stringify({width,before,value}));
  }
 }
 assert.deepEqual(errors,[]);console.log('PASS: held mouse target across polling, hidden-page continuity, transient retry and no stale-session Stop, editable range drafts with power off/offline, guarded Start search, stable off/pending/on/off layout at 1180/600/320px with delayed replies, clear power state/actions/unknown status, directional white/red lamps on all three locos, stationary track explicit-on/heartbeat/off/expiry/no-restart, per-loco direction/F0 while powered, metadata locks, old-firmware guards, leave-page off, mobile layout and guided search to 37 with settling/locks/result editor.');
}finally{await browser.close()}})().catch(e=>{console.error(e);process.exit(1)});
