const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../..');
(async()=>{const browser=await chromium.launch({...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{}),headless:true});try{
 const page=await browser.newPage({viewport:{width:1180,height:1100}}),errors=[],calls=[];page.on('pageerror',e=>errors.push(e.message));
 const loco=(id,address,type)=>({id,address,name:id===1?'Night express':'Freight',type,forward:true,f0:false,functions:0,favorites:[],bytes:['03','80','83']});
 const state={appId:'ANDEN_ESP13_OTA_V1',version:'0.6.0',connected:true,configured:true,ssid:'Workshop',ip:'192.168.1.43',apIp:'192.168.13.1',apReady:true,mdnsReady:true,otaMaxBytes:458752,freeHeap:40000,storageReady:true,uno:{online:true,version:'0.6.0',enableA:true,enableB:false,trackActive:true,trackSupported:true,adcA:0,adcB:0,rxFrames:5,roundTripMs:8},dcc:{ready:true,running:true,functionsSupported:true,packets:100,timingFaults:0,finder:{active:false,stage:'idle',first:1,last:100,step:0},locos:[loco(1,3,'electric'),loco(2,6,'diesel')]}};
 let failSave=false;
 await page.route('http://functions.test/**',async route=>{const req=route.request(),url=new URL(req.url()),data=new URLSearchParams(req.postData());
  if(url.pathname==='/api/status')return route.fulfill({json:state});
  if(req.method()==='POST'){
   calls.push({path:url.pathname,data:Object.fromEntries(data)});
   if(url.pathname==='/api/track/off'){state.uno.enableA=state.uno.trackActive=false;return route.fulfill({json:{ok:true}})}
   if(url.pathname==='/api/track/keep')return route.fulfill({json:{ok:true}});
   const l=state.dcc.locos.find(l=>data.has('id')?l.id===+data.get('id'):l.address===+data.get('address'));
   if(url.pathname==='/dcc/function'||url.pathname==='/dcc/f0'){
    const n=url.pathname.endsWith('/f0')?0:+data.get('function'),on=data.get('on')==='1';l.functions=on?l.functions|(1<<n):l.functions&~(1<<n);l.f0=!!(l.functions&1);
   }else if(url.pathname==='/locos/favorite'){
    if(failSave)return route.fulfill({status:500,json:{error:'Could not save favorite functions.'}});
    const n=+data.get('function'),f=l.favorites.find(f=>f.number===n);if(f)f.name=data.get('name');else l.favorites.push({number:n,name:data.get('name')});
   }else if(url.pathname==='/locos/favorite/remove')l.favorites=l.favorites.filter(f=>f.number!==+data.get('function'));
   else throw Error('Unexpected request '+url.pathname);
   return route.fulfill({json:state.dcc});
  }
  return route.fulfill({contentType:'text/html',body:fs.readFileSync(path.join(root,'firmware/esp13-ota/web/index.html'),'utf8')});
 });
 const card=id=>page.locator('.loco[data-id="'+id+'"]'),select=id=>card(id).locator('.function-select'),name=id=>card(id).locator('.function-name');
 const settled=()=>page.waitForFunction(()=>!rosterBusy&&!statusRefreshing);
 const refresh=async()=>{await settled();await page.evaluate(()=>refreshStatus());await settled()};
 await page.goto('http://functions.test/');await page.waitForFunction(()=>document.querySelectorAll('.loco').length===2);await settled();
 assert.equal(await card(1).locator('details').getAttribute('open'),null);
 await card(1).locator('summary').click();await card(2).locator('summary').click();
 assert.equal(await select(1).locator('option').count(),29);
 await select(1).selectOption('28');assert.equal(calls.length,0);
 await card(1).locator('.function-on').click();await settled();assert.equal(state.dcc.locos[0].functions,1<<28);assert.equal(state.dcc.locos[1].functions,0);
 await select(1).selectOption('1');await card(1).locator('.function-on').click();await settled();assert.equal(state.dcc.locos[0].functions,(1<<28)|2);
 await select(2).selectOption('5');await card(2).locator('.function-on').click();await settled();assert.equal(state.dcc.locos[1].functions,32);
 await name(1).fill('Engine sound');await card(1).locator('.pin-function').click();await settled();
 const quick=card(1).locator('.favorite-button[data-function="1"]');assert.equal(await quick.textContent(),'Engine sound · F1');
 await card(1).locator('summary').click();assert(await quick.isVisible());await quick.click();await settled();assert.equal(state.dcc.locos[0].functions,1<<28);assert.equal(state.dcc.locos[1].functions,32);
 await card(1).locator('summary').click();await name(1).fill('Sound draft');await name(1).focus();
 await refresh();assert.equal(await name(1).inputValue(),'Sound draft');assert(await name(1).evaluate(e=>e===document.activeElement));assert.equal(await select(1).inputValue(),'1');assert.equal(await card(1).locator('details').getAttribute('open'),'');
 const pin=card(1).locator('.pin-function');await pin.scrollIntoViewIfNeeded();const box=await pin.boundingBox();await page.mouse.move(box.x+box.width/2,box.y+box.height/2);await page.mouse.down();await refresh();await page.mouse.up();await settled();assert.equal(state.dcc.locos[0].favorites[0].name,'Sound draft');
 failSave=true;await name(1).fill('Unsaved');await pin.click();await settled();assert.equal(state.dcc.locos[0].favorites[0].name,'Sound draft');assert.equal(await name(1).inputValue(),'Unsaved');failSave=false;
 await name(1).fill('<b>Sound</b>');await pin.click();await settled();assert.equal(await quick.locator('b').count(),0);assert.equal(await quick.textContent(),'<b>Sound</b> · F1');
 await page.reload();await page.waitForFunction(()=>document.querySelector('.favorite-button'));await settled();assert.equal(await card(1).locator('details').getAttribute('open'),null);assert.equal(await quick.textContent(),'<b>Sound</b> · F1');
 await card(1).locator('summary').click();await select(1).selectOption('0');await name(1).fill('Cab lights');await pin.click();await settled();assert((await card(1).innerText()).includes('F0 · Cab lights'));assert.equal(await card(1).locator('.favorite-button[data-function="0"]').count(),0);
 state.uno.trackActive=state.uno.enableA=false;await refresh();assert(await card(1).locator('.function-on').isDisabled());assert.equal(await select(1).isDisabled(),false);await select(1).selectOption('2');await name(1).fill('Horn');assert.equal(await pin.isDisabled(),false);await pin.click();await settled();
 for(let n=3;n<=7;n++)state.dcc.locos[0].favorites.push({number:n,name:'Effect '+n});await refresh();assert.equal(state.dcc.locos[0].favorites.length,8);await select(1).selectOption('28');await name(1).fill('Extra');assert(await pin.isDisabled());
 await select(1).selectOption('1');await card(1).locator('.unpin-function').click();await settled();assert.equal(await quick.count(),0);assert.equal(state.dcc.locos[0].favorites.length,7);
 await select(1).selectOption('3');await name(1).fill('é'.repeat(13));assert(await pin.isDisabled());
 state.uno.enableA=state.uno.trackActive=true;state.dcc.functionsSupported=false;await refresh();assert(await card(1).locator('.function-on').isDisabled());await select(1).selectOption('0');assert.equal(await card(1).locator('.function-on').isDisabled(),false);
 state.dcc.functionsSupported=true;state.dcc.finder.active=true;await refresh();assert(await select(1).isDisabled());assert(await pin.isDisabled());state.dcc.finder.active=false;await refresh();
 await select(1).selectOption('28');await name(1).fill('Smoke');
 state.dcc.locos[0].favorites=[{number:1,name:'Engine sound'},{number:2,name:'Horn'}];state.dcc.locos[1].favorites=[{number:3,name:'Cab lights'}];await refresh();await page.locator('#theme [data-theme-choice=dark]').click();
 const out=path.join(root,'build/ui');fs.mkdirSync(out,{recursive:true});await page.screenshot({path:path.join(out,'esp13-functions-dark.png'),fullPage:true});
 for(const theme of ['light','dark']){await page.locator('#theme [data-theme-choice='+theme+']').click();await page.setViewportSize({width:320,height:1100});assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false)}
 await page.screenshot({path:path.join(out,'esp13-functions-mobile.png'),fullPage:true});assert.deepEqual(errors,[]);
 console.log('PASS: F0–F28, independent loco states, collapsed named favorites, rename/remove/reload, safe names, failed-save draft, stable held mouse/poll/focus/selection, eight-pin limit, power-off editing, legacy Uno/finder guards and 320px themes.');
}finally{await browser.close()}})().catch(e=>{console.error(e);process.exit(1)});
