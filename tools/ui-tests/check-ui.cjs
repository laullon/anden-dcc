const {chromium}=require('playwright');
const assert=require('node:assert/strict');
const fs=require('fs'),path=require('path');
const root=path.resolve(__dirname,'../..');
const out=process.env.UI_ARTIFACTS||path.join(root,'build/ui');fs.mkdirSync(out,{recursive:true});
const makeLoco=(id,address,name='',f0=false)=>({id,address,name,f0,bytes:[address,f0?0x90:0x80,address^(f0?0x90:0x80)].map(n=>n.toString(16).toUpperCase().padStart(2,'0')).join(' ')});
(async()=>{
 const browser=await chromium.launch({...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{}),headless:true});
 try {
 const context=await browser.newContext({viewport:{width:1440,height:1100},deviceScaleFactor:1});
 let state={name:'anden-dcc',version:fs.readFileSync(path.join(root,'firmware/anden-dcc/anden-dcc.ino'),'utf8').match(/VERSION = "([^"]+)"/)[1],connected:true,mdnsReady:true,storageReady:true,ssid:'Studio Wi-Fi',ip:'192.168.1.42',trackPower:false,dcc:{ready:true,running:true,pin:2,maxLocos:8,locos:[makeLoco(1,3),makeLoco(2,4)]}},offline=false,rejectSave=false,delayStatus=null,saveCount=0;
 await context.route('http://anden.test/**',async route=>{
  const u=new URL(route.request().url()),data=new URLSearchParams(route.request().postData());
  if(u.pathname==='/api/status') {
   if(offline)return route.abort();
   const snapshot=JSON.stringify(state);if(delayStatus)await delayStatus;
   return route.fulfill({body:snapshot,contentType:'application/json'});
  }
  if(u.pathname==='/dcc/f0') {
   const index=state.dcc.locos.findIndex(l=>l.address===Number(data.get('address')));
   if(index<0)return route.fulfill({status:404});
   const l=state.dcc.locos[index];state.dcc.locos[index]=makeLoco(l.id,l.address,l.name,data.get('on')==='1');
   state.dcc.running=true;return route.fulfill({json:state.dcc});
  }
  if(u.pathname==='/locos/save') {
   saveCount++;if(rejectSave)return route.fulfill({status:500});
   let id=Number(data.get('id')),address=Number(data.get('address')),name=data.get('name');
   if(state.dcc.locos.some(l=>l.address===address&&l.id!==id)||(!id&&state.dcc.locos.length===8))return route.fulfill({status:409});
   if(id){const i=state.dcc.locos.findIndex(l=>l.id===id);if(i<0)return route.fulfill({status:404});const old=state.dcc.locos[i];state.dcc.locos[i]=makeLoco(id,address,name,old.address===address&&old.f0);}
   else {id=1;while(state.dcc.locos.some(l=>l.id===id))id++;state.dcc.locos.push(makeLoco(id,address,name));}
   return route.fulfill({json:state.dcc});
  }
  if(u.pathname==='/locos/remove'){state.dcc.locos=state.dcc.locos.filter(l=>l.id!==Number(data.get('id')));return route.fulfill({json:state.dcc});}
  if(u.pathname==='/update')return route.fulfill({body:'Update error: test rejection',contentType:'text/html'});
  if(u.pathname==='/icon.svg')return route.fulfill({body:fs.readFileSync(path.join(root,'web/icon.svg'),'utf8'),contentType:'image/svg+xml'});
  return route.fulfill({body:fs.readFileSync(path.join(root,'web/index.html'),'utf8'),contentType:'text/html'});
 });
 const page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
 const card=id=>page.locator('.train-card[data-id="'+id+'"]');
 const light=(id,on)=>card(id).locator('.dcc-action[data-on="'+Number(on)+'"]');
 async function waitReady(){await page.waitForFunction(()=>document.getElementById('connection-text').textContent==='Connected');}
 async function save(){await page.locator('#save-loco').click();await page.waitForFunction(()=>!document.getElementById('loco-dialog').open);}
 async function edit(id,name,address){await card(id).locator('.edit-loco').click();if(name!==null)await page.locator('#loco-name').fill(name);if(address!==null)await page.locator('#loco-address').fill(String(address));await save();}
 async function press(id,on){await light(id,on).click();await page.waitForFunction(({id,on})=>document.querySelector('.train-card[data-id="'+id+'"] .dcc-action[data-on="'+Number(on)+'"]').getAttribute('aria-pressed')==='true',{id,on});}
 await page.goto('http://anden.test/');await waitReady();
 assert.equal(await page.locator('#setup').getAttribute('open'),null);assert.equal(await page.locator('#wifi').isVisible(),false);
 assert.equal(await page.locator('#show-programming,#programming,#forgot-address').count(),0);
 assert.equal(await page.locator('.train-card').count(),2);
 const first=await card(1).boundingBox(),second=await card(2).boundingBox();assert.equal(first.y,second.y);assert(second.x>first.x);
 await press(1,true);assert.equal(await light(2,false).getAttribute('aria-pressed'),'true');
 await press(2,true);await press(1,false);assert.equal(await light(2,true).getAttribute('aria-pressed'),'true');
 await edit(1,'Evening express',null);await edit(2,'The little shunter',37);
 assert.equal(await light(2,false).getAttribute('aria-pressed'),'true');
 await press(1,true);await press(2,true);
 await page.waitForTimeout(300);await page.screenshot({path:path.join(out,'ui-desktop.png'),fullPage:true});
 // A duplicate address is rejected without a server mutation; draft survives polling.
 await card(1).locator('.edit-loco').click();await page.locator('#loco-address').fill('37');const saved=saveCount;
 await page.locator('#save-loco').click();assert.equal(await page.locator('#loco-dialog').evaluate(e=>e.open),true);assert.equal(saveCount,saved);assert.match(await page.locator('#editor-message').textContent(),/already/);
 await page.locator('#loco-name').fill('Draft name');await page.evaluate(()=>refresh());assert.equal(await page.locator('#loco-name').inputValue(),'Draft name');
 await page.locator('#loco-address').fill('3');rejectSave=true;await page.locator('#save-loco').click();await page.waitForFunction(()=>document.getElementById('editor-message').classList.contains('error'));assert.equal(await card(1).locator('.loco-name').textContent(),'Evening express');rejectSave=false;
 await page.locator('#cancel-editor').click();
 await page.reload();await waitReady();assert.equal(await card(1).locator('.loco-name').textContent(),'Evening express');
 // A slow poll captured before a command cannot roll the UI back afterwards.
 let release;delayStatus=new Promise(resolve=>release=resolve);await page.evaluate(()=>{window.slowPoll=refresh();});await page.waitForTimeout(50);
 await press(1,false);release();delayStatus=null;await page.evaluate(()=>window.slowPoll);assert.equal(await light(1,false).getAttribute('aria-pressed'),'true');
 // Add, safe rendering of names, delete, capacity and empty roster.
 await page.locator('#add-loco').click();await page.locator('#loco-name').fill('<img src=x onerror=alert(1)>');await page.locator('#loco-address').fill('12');await save();
 assert.equal(await card(3).locator('.loco-name img').count(),0);assert.match(await card(3).locator('.loco-name').textContent(),/<img/);
 await page.screenshot({path:path.join(out,'ui-three-locos.png'),fullPage:true});
 page.on('dialog',d=>d.accept());await card(3).locator('.edit-loco').click();await page.locator('#remove-loco').click();await page.waitForFunction(()=>document.querySelectorAll('.train-card').length===2);
 for(let n=3;n<=8;n++){await page.locator('#add-loco').click();await page.locator('#loco-name').fill('Display train '+n);await save();}
 assert.equal(await page.locator('.train-card').count(),8);assert.equal(await page.locator('#add-loco').isDisabled(),true);
 for(const loco of [...state.dcc.locos].slice(2)){await card(loco.id).locator('.edit-loco').click();await page.locator('#remove-loco').click();await page.waitForFunction(()=>!document.getElementById('loco-dialog').open);}
 await page.locator('[data-lang="es"]').click();assert.equal(await page.locator('h1').textContent(),'Dale vida a tu escena.');assert.equal(await card(1).locator('.edit-loco').textContent(),'Editar');
 await page.reload();await page.waitForFunction(()=>document.documentElement.lang==='es');assert.equal(await page.locator('#setup').getAttribute('open'),null);
 await page.locator('[data-lang="en"]').click();
 await page.locator('#setup>summary').click();await page.screenshot({path:path.join(out,'ui-setup.png'),fullPage:true});
 await page.locator('#firmware-file').setInputFiles({name:'test.bin',mimeType:'application/octet-stream',buffer:Buffer.from('UI mock upload')});await page.locator('#update-button').click();await page.waitForFunction(()=>document.getElementById('update-message').classList.contains('error'));assert.equal(await page.locator('#update-button').isDisabled(),false);
 await card(1).locator('.edit-loco').click();offline=true;await page.evaluate(()=>refresh());assert.equal(await page.locator('#offline-note').isVisible(),true);assert.equal(await light(1,true).isDisabled(),true);assert.equal(await page.locator('#add-loco').isDisabled(),true);assert.equal(await page.locator('#cancel-editor').isDisabled(),false);await page.locator('#cancel-editor').click();offline=false;await page.evaluate(()=>refresh());
 await page.goto('http://anden.test/?setup=firmware');await page.waitForFunction(()=>document.getElementById('setup').open);
 await page.setViewportSize({width:390,height:1800});await page.goto('http://anden.test/');await waitReady();
 const m1=await card(1).boundingBox(),m2=await card(2).boundingBox();assert(m2.y>m1.y);assert.equal(m1.x,m2.x);
 await page.screenshot({path:path.join(out,'ui-mobile.png')});await page.locator('[data-lang="es"]').click();await page.screenshot({path:path.join(out,'ui-mobile-es.png')});
 await page.setViewportSize({width:320,height:900});assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
 await card(1).locator('.edit-loco').click();assert.equal(await page.evaluate(()=>document.getElementById('loco-dialog').scrollWidth>document.getElementById('loco-dialog').clientWidth),false);await page.locator('#cancel-editor').click();
 for(const loco of [...state.dcc.locos]){await card(loco.id).locator('.edit-loco').click();await page.locator('#remove-loco').click();await page.waitForFunction(()=>!document.getElementById('loco-dialog').open);}
 assert.equal(await page.locator('#empty-roster').isVisible(),true);assert.equal(await page.locator('#add-loco').isDisabled(),false);
 await page.locator('#add-loco').click();await page.locator('#loco-name').fill('A new beginning');await save();assert.equal(await page.locator('.train-card').count(),1);
 assert.deepEqual(errors,[]);
 console.log('PASS: independent loco F0, shared roster, rename/address edits, duplicate prevention, failed save, draft preservation, stale-poll protection, add/remove/empty/max-8, safe names, EN/ES, setup/OTA/offline, desktop/mobile; no JS errors.');
 } finally {await browser.close();}
})().catch(e=>{console.error(e);process.exit(1)});
