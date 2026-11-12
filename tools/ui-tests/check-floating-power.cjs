const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../..');
const currentVersion=fs.readFileSync(path.join(root,'firmware/esp13-ota/esp13-ota.ino'),'utf8').match(/VERSION\[\]\s*=\s*"([^"]+)"/)[1];
(async()=>{const browser=await chromium.launch({headless:true,...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{})});try{
 const page=await browser.newPage({viewport:{width:1180,height:900}}),errors=[];page.on('pageerror',e=>errors.push(e.message));
 const state={appId:'ANDEN_ESP13_OTA_V1',version:currentVersion,connected:true,configured:true,ssid:'Workshop',ip:'192.168.1.43',apIp:'192.168.13.1',apReady:true,mdnsReady:true,otaMaxBytes:458752,freeHeap:32000,storageReady:true,uno:{online:true,version:'0.6.0',trackSupported:true,trackActive:false,enableA:false,enableB:false,adcA:0,adcB:0,roundTripMs:8,rxFrames:3,bench:{supported:true,dccSupported:true,active:false,state:0,mode:0}},dcc:{ready:true,running:true,functionsSupported:true,packets:100,timingFaults:0,finder:{stage:'idle',active:false},locos:[{onDiorama:true,id:1,address:3,name:'Night express',type:'electric',f0:false,forward:true,functions:0,favorites:[{number:1,name:'Engine sound'},{number:2,name:'Horn'}]},{onDiorama:true,id:2,address:6,name:'Freight',type:'diesel',f0:false,forward:true,functions:0,favorites:[{number:3,name:'Cab lights'}]}]}};
 let starts=0,stops=0,keeps=0;
 await page.route('http://power.test/**',async route=>{const u=new URL(route.request().url());
  if(u.pathname==='/api/status')return route.fulfill({json:state});
  if(u.pathname==='/api/track/on'){starts++;await new Promise(r=>setTimeout(r,200));state.uno.trackActive=state.uno.enableA=true;return route.fulfill({json:{lease:starts}})}
  if(u.pathname==='/api/track/off'){stops++;await new Promise(r=>setTimeout(r,200));state.uno.trackActive=state.uno.enableA=false;return route.fulfill({json:{ok:true}})}
  if(u.pathname==='/api/track/keep'){keeps++;return route.fulfill({json:{ok:true}})}
  assert.equal(route.request().method(),'GET');return route.fulfill({contentType:'text/html',body:fs.readFileSync(path.join(root,'firmware/esp13-ota/web/index.html'),'utf8')});
 });
 const bar=page.locator('#floating-power'),on=page.locator('#floating-on'),off=page.locator('#floating-off');
 const settle=()=>page.waitForFunction(()=>!statusRefreshing&&!trackBusy&&!trackTurningOff);
 const refresh=async()=>{await settle();await page.evaluate(()=>refreshStatus());await settle()};
 await page.goto('http://power.test/');await page.waitForFunction(()=>document.querySelectorAll('.loco').length===2);await settle();assert.equal(await bar.isVisible(),false);
 const out=path.join(root,'build/ui');fs.mkdirSync(out,{recursive:true});await page.screenshot({path:path.join(out,'stack-desktop.png'),fullPage:true});
 for(const width of [1180,320]){
  await page.setViewportSize({width,height:900});await page.evaluate(()=>{const r=document.getElementById('track-power').getBoundingClientRect();scrollTo(0,scrollY+r.bottom+20)});await bar.waitFor({state:'visible'});
  const y=await page.evaluate(()=>scrollY);assert(await on.isVisible());assert.equal(await on.isDisabled(),false);
  const box=await bar.boundingBox();assert(box.x>=0&&box.x+box.width<=width&&box.y>=0&&box.y+box.height<=900);assert.equal(await on.evaluate(e=>e.getBoundingClientRect().height>=44),true);
  await on.click();await page.waitForFunction(()=>trackBusy);assert.equal(await bar.getAttribute('data-power'),'off');assert((await bar.innerText()).includes('Starting'));assert(await off.isVisible());
  await settle();assert.equal(await bar.getAttribute('data-power'),'on');assert(await off.isVisible());assert.equal(await page.evaluate(()=>scrollY),y);
  const before=keeps;await page.waitForTimeout(600);assert(keeps>before);
  await off.click();await page.waitForFunction(()=>trackTurningOff);assert(await off.isDisabled());await settle();assert.equal(await bar.getAttribute('data-power'),'off');
  state.uno.online=false;await refresh();assert.equal(await bar.getAttribute('data-power'),'unknown');assert(await off.isVisible());assert.equal(await off.isDisabled(),false);assert.equal(await on.isVisible(),false);
  state.uno.online=true;state.dcc.ready=false;await refresh();assert(await on.isDisabled());state.dcc.ready=true;state.dcc.finder.active=true;await refresh();assert(await on.isDisabled());state.dcc.finder.active=false;
  state.uno.enableA=true;await refresh();assert.equal(await bar.getAttribute('data-power'),'bench');assert((await off.innerText()).includes('Stop test'));await off.click();await settle();
  await page.locator('#theme [data-theme-choice=dark]').click();assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
  await page.evaluate(()=>{const r=document.getElementById('track-power').getBoundingClientRect();scrollTo(0,scrollY+r.bottom+20)});await bar.waitFor({state:'visible'});await page.screenshot({path:path.join(out,'floating-power-'+width+'.png'),fullPage:false});
  await page.evaluate(()=>scrollTo(0,0));await bar.waitFor({state:'hidden'});
 }
 await page.locator('#theme [data-theme-choice=light]').click();await page.evaluate(()=>document.getElementById('finder-panel').hidden=true);await page.screenshot({path:path.join(out,'stack-mobile.png'),fullPage:true});
 assert.equal(starts,2);assert.equal(stops,4);assert.deepEqual(errors,[]);console.log('PASS: compact power after scroll, shared On/Off/lease, stable scroll and pending states, 44px touch controls, unknown/offline/bench/finder guards, top-panel handoff and desktop/320px themes.');
}finally{await browser.close()}})().catch(e=>{console.error(e);process.exit(1)});
