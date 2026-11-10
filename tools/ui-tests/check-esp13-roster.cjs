const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../..');
const make=(id,address,name='',type='electric',f0=false,onDiorama=false)=>({id,address,name,type,f0,onDiorama,bytes:[address,f0?0x90:0x80,address^(f0?0x90:0x80)].map(n=>n.toString(16).toUpperCase().padStart(2,'0'))});
(async()=>{const browser=await chromium.launch({...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{}),headless:true});try{
 const page=await browser.newPage({viewport:{width:1180,height:1000}}),errors=[];page.on('pageerror',e=>errors.push(e.message));
 const state={name:'anden-esp13',appId:'ANDEN_ESP13_OTA_V1',version:'0.2.0',connected:true,configured:true,ssid:'Workshop',ip:'192.168.1.43',apIp:'192.168.13.1',apReady:true,mdnsReady:true,otaMaxBytes:458752,freeHeap:40000,storageReady:true,uno:{online:true,version:'0.2.0',enableA:false,enableB:false,adcA:0,adcB:0,rxFrames:5,roundTripMs:8},dcc:{ready:true,running:true,cabFilterSupported:true,pin:10,packets:100,timingFaults:0,locos:[make(1,3),make(2,4,'','diesel')]}};
 let saves=0,holdNext=false,release=null;
 await page.route('http://espdcc.test/**',async route=>{const request=route.request(),u=new URL(request.url()),data=new URLSearchParams(request.postData());
  if(u.pathname==='/api/status'){const body=JSON.stringify(state);if(holdNext){holdNext=false;await new Promise(r=>release=r)}return route.fulfill({body,contentType:'application/json'})}
  if(u.pathname==='/dcc/f0'){let i=state.dcc.locos.findIndex(l=>l.address===Number(data.get('address'))),l=state.dcc.locos[i];state.dcc.locos[i]=make(l.id,l.address,l.name,l.type,data.get('on')==='1',l.onDiorama);return route.fulfill({json:state.dcc})}
  if(u.pathname==='/locos/save'){saves++;const id=Number(data.get('id')),address=Number(data.get('address'));let i=state.dcc.locos.findIndex(l=>l.id===id),old=state.dcc.locos[i];if(i<0)i=state.dcc.locos.length;state.dcc.locos[i]=make(id||i+1,address,data.get('name'),data.get('type'),old?.address===address?old.f0:false,data.get('onDiorama')==='1');return route.fulfill({json:state.dcc})}
  if(u.pathname==='/locos/remove'){state.dcc.locos=state.dcc.locos.filter(l=>l.id!==Number(data.get('id')));return route.fulfill({json:state.dcc})}
  return route.fulfill({contentType:'text/html',body:fs.readFileSync(path.join(root,'firmware/esp13-ota/web/index.html'),'utf8')});
 });
 await page.goto('http://espdcc.test/');await page.waitForFunction(()=>document.querySelectorAll('.loco').length===2);
 const card=id=>page.locator('.loco[data-id="'+id+'"]');
 const on=id=>card(id).locator('.favorite-strip [data-function="0"]');
 const a=await card(1).boundingBox(),b=await card(2).boundingBox();assert.equal(a.y,b.y);
 // A delayed old poll must not undo an acknowledged F0 command.
 holdNext=true;await page.evaluate(()=>{window.oldPoll=refreshStatus()});await page.waitForTimeout(20);
 await on(1).click();await page.waitForFunction(()=>document.querySelector('.loco[data-id="1"] .favorite-strip [data-function="0"]').getAttribute('aria-pressed')==='true');
 release();await page.evaluate(()=>window.oldPoll);assert.equal(await on(1).getAttribute('aria-pressed'),'true');assert.equal(await on(2).getAttribute('aria-pressed'),'false');
 await card(1).locator('.edit-loco').click();await page.locator('#loco-name').fill('Night express');await page.getByLabel('On diorama').check();await page.locator('#loco-type').selectOption('steam');assert.equal(await page.locator('#loco-preview svg').getAttribute('data-model'),'santa-fe');await page.locator('#save-loco').click();await page.waitForFunction(()=>!document.getElementById('loco-dialog').open);assert.equal(state.dcc.locos[0].f0,true);assert.equal(state.dcc.locos[0].onDiorama,true);assert((await card(1).textContent()).includes('On diorama'));
 await card(1).locator('.edit-loco').click();assert(await page.getByLabel('On diorama').isChecked());await page.getByLabel('On diorama').uncheck();await page.locator('#loco-address').fill('4');await page.locator('#save-loco').click();assert.equal(saves,1);assert((await page.locator('#editor-message').textContent()).includes('already'));await page.locator('#loco-address').fill('37');await page.locator('#save-loco').click();await page.waitForFunction(()=>!document.getElementById('loco-dialog').open);assert.equal(state.dcc.locos[0].f0,false);assert.equal(state.dcc.locos[0].onDiorama,false);assert(!(await card(1).textContent()).includes('On diorama'));
 await page.locator('#add-loco').click();await page.locator('#loco-name').fill('<img src=x onerror=alert(1)>');await page.locator('#save-loco').click();await page.waitForFunction(()=>document.querySelectorAll('.loco').length===3);assert.equal(await page.locator('.loco-name img').count(),0);
 page.on('dialog',d=>d.accept());await card(3).locator('.edit-loco').click();await page.locator('#remove-loco').click();await page.waitForFunction(()=>document.querySelectorAll('.loco').length===2);
 state.uno.enableA=true;state.uno.bench={supported:true,active:true,state:1,remainingMs:2000};await page.evaluate(()=>refreshStatus());
 assert(await on(1).isDisabled());assert(await card(1).locator('.edit-loco').isDisabled());assert(await page.locator('#add-loco').isDisabled());
 state.uno.enableA=false;state.uno.bench={supported:true,active:false,state:2,remainingMs:0};await page.evaluate(()=>refreshStatus());assert.equal(await on(1).isDisabled(),false);
 state.dcc.ready=false;await page.evaluate(()=>refreshStatus());assert(await on(1).isDisabled());assert.equal(await card(1).locator('.edit-loco').isDisabled(),false);
 await page.setViewportSize({width:320,height:1100});assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
 await card(1).locator('.edit-loco').click();assert.equal(await page.evaluate(()=>document.getElementById('loco-dialog').scrollWidth>document.getElementById('loco-dialog').clientWidth),false);await page.locator('#cancel-loco').click();
 assert.deepEqual(errors,[]);console.log('PASS: ESP roster two-card layout, independent F0/bytes, stale-poll protection, saved diorama marking, edit/type preview, address reset, duplicates, add/remove, safe names, DC-bench roster lock, offline Uno and 320px layout.');
}finally{await browser.close()}})().catch(e=>{console.error(e);process.exit(1)});
