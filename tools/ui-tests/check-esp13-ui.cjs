const {chromium}=require('playwright');
const assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../..');
const out=process.env.UI_ARTIFACTS||path.join(root,'build/ui');fs.mkdirSync(out,{recursive:true});
(async()=>{
 const browser=await chromium.launch({...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{}),headless:true});
 try{
  const context=await browser.newContext({viewport:{width:1180,height:1000}});
  let current={name:'anden-esp13',platform:'esp8266',appId:'ANDEN_ESP13_OTA_V1',version:'0.1.1',bootId:41,connected:true,configured:true,mdnsReady:true,ssid:'Workshop Wi-Fi',ip:'192.168.86.50',apIp:'192.168.13.1',apReady:true,otaMaxBytes:448*1024,freeHeap:32000,layoutBytes:1048576,uno:{online:true,version:'0.1.0',enableA:false,enableB:false,adcA:0,adcB:1,roundTripMs:8,rxFrames:2,ledEdges:0}};
  let offline=false,uploadCount=0,acceptUpload=false,wifiCalls=[],bridgeCalls=[],dccCalls=0,refusePulse=false;
  await context.route('http://esp13.test/**',async route=>{
   const request=route.request(),pathname=new URL(request.url()).pathname;
   if(pathname==='/api/status')return offline?route.abort():route.fulfill({json:current});
   if(pathname==='/setup/wifi'||pathname==='/setup/wifi/forget'){
    wifiCalls.push({pathname,body:request.postData()});return route.fulfill({status:202,json:{message:'Settings accepted.'}});
   }
   if(pathname==='/api/uno/ping'||pathname==='/api/uno/led'){
    if(!current.uno.online)return route.fulfill({status:503,json:current.uno});
    if(pathname.endsWith('/led'))current.uno.ledEdges=15;
    return route.fulfill({json:current.uno});
   }
   if(pathname==='/api/uno/bridge'){
    bridgeCalls.push(new URLSearchParams(request.postData()).get('direction'));
    if(refusePulse){current.uno.bench={supported:true,active:false,state:4,remainingMs:0};return route.fulfill({status:409,json:{...current.uno,error:'Raw A0 cutoff tripped.'}})}
    current.uno.enableA=true;current.uno.bench={supported:true,active:true,state:1,remainingMs:3000};return route.fulfill({json:current.uno});
   }
   if(pathname==='/api/uno/dcc-test'){
    dccCalls++;current.uno.enableA=true;current.uno.bench={supported:true,dccSupported:true,mode:2,active:true,state:1,remainingMs:3000};current.dcc.ready=false;current.dcc.trackPower=true;return route.fulfill({json:current.uno});
   }
   if(pathname==='/api/uno/bench-stop'){
    current.uno.enableA=false;current.uno.bench={...current.uno.bench,supported:true,active:false,state:3,remainingMs:0};if(current.dcc){current.dcc.ready=true;current.dcc.trackPower=false}return route.fulfill({json:current.uno});
   }
   if(pathname==='/update'){
    uploadCount++;if(acceptUpload){current={...current,bootId:42,version:'0.1.2'};return route.fulfill({body:'Update Success! Rebooting...'});}
    return route.fulfill({body:'Update error: test rejection'});
   }
   return route.fulfill({contentType:'text/html',body:fs.readFileSync(path.join(root,'experiments/esp13-ota/web/index.html'),'utf8')});
  });
  const page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.goto('http://esp13.test/');await page.waitForFunction(()=>document.getElementById('version').textContent==='v0.1.1');
  assert.equal(await page.locator('#setup').getAttribute('open'),null);
  assert.equal(await page.locator('#network-state').textContent(),'Connected');
  assert.equal(await page.locator('#uno-state').textContent(),'Connected');
  await page.locator('#ping-uno').click();await page.waitForFunction(()=>document.getElementById('uno-message').textContent.includes('Serial link is working'));
  await page.locator('#blink-uno').click();await page.waitForFunction(()=>document.getElementById('uno-message').textContent.includes('Blink acknowledged'));
  current.uno.online=false;await page.evaluate(()=>refreshStatus());assert.equal(await page.locator('#blink-uno').isDisabled(),true);
  await page.locator('#ping-uno').click();await page.waitForFunction(()=>document.getElementById('uno-message').classList.contains('error'));
  current.uno.online=true;current.uno.enableA=true;await page.evaluate(()=>refreshStatus());assert.equal(await page.locator('#blink-uno').isDisabled(),true);
  current.uno.enableA=false;current.uno.ledEdges=0;await page.evaluate(()=>refreshStatus());
  await page.locator('#setup>summary').click();
  assert.equal(await page.locator('#bridge-positive').isDisabled(),true);
  current.uno.bench={supported:true,active:false,state:0,remainingMs:0};await page.evaluate(()=>refreshStatus());
  await page.locator('#bridge-positive').click();await page.waitForFunction(()=>document.getElementById('uno-message').textContent.includes('DC test started'));
  assert.equal(await page.locator('#bridge-negative').isDisabled(),true);assert.equal(await page.locator('#blink-uno').isDisabled(),true);
  assert((await page.locator('#uno-outputs').innerText()).includes('D3 HIGH'));
  await page.locator('#bridge-stop').click();await page.waitForFunction(()=>document.getElementById('bench-status').textContent.includes('Test stopped'));
  assert.equal(current.uno.enableA,false);await page.locator('#bridge-negative').click();await page.waitForFunction(()=>document.getElementById('uno-message').textContent.includes('DC test started'));
  current.uno.enableA=false;current.uno.bench={supported:true,active:false,state:2,remainingMs:0};await page.evaluate(()=>refreshStatus());
  assert.deepEqual(bridgeCalls,['1','0']);assert((await page.locator('#bench-status').innerText()).includes('Test complete'));
  refusePulse=true;await page.locator('#bridge-positive').click();await page.waitForFunction(()=>document.getElementById('uno-message').classList.contains('error'));
  assert((await page.locator('#bench-status').innerText()).includes('cutoff'));assert.equal(await page.locator('#bench-status').getAttribute('class'),'message error');
  current.uno.bench={supported:true,active:false,state:0,remainingMs:0};refusePulse=false;await page.evaluate(()=>refreshStatus());
  assert(await page.locator('#bridge-dcc').isDisabled());
  current.dcc={ready:false,running:true,pin:10,packets:100,timingFaults:0,locos:[]};current.uno.bench.dccSupported=true;await page.evaluate(()=>refreshStatus());assert(await page.locator('#bridge-dcc').isDisabled());
  current.dcc.ready=true;await page.evaluate(()=>refreshStatus());await page.locator('#bridge-dcc').click();
  await page.waitForFunction(()=>document.getElementById('uno-message').textContent.includes('DCC scope test started'));
  assert((await page.locator('#bench-status').innerText()).includes('DCC burst active'));assert(await page.locator('#bridge-dcc').isDisabled());assert.equal(dccCalls,1);
  await page.locator('#bridge-stop').click();await page.waitForFunction(()=>document.getElementById('bench-status').textContent.includes('Test stopped'));assert.equal(current.dcc.trackPower,false);assert.equal(current.uno.enableA,false);
  await page.locator('#ssid-input').fill('A draft network');await page.evaluate(()=>refreshStatus());
  assert.equal(await page.locator('#ssid-input').inputValue(),'A draft network');
  await page.locator('#password').fill('prototype-password');await page.locator('#save-wifi').click();
  await page.waitForFunction(()=>document.getElementById('wifi-message').textContent==='Settings accepted.');
  assert(wifiCalls[0].body.includes('A draft network'));assert(wifiCalls[0].body.includes('prototype-password'));
  assert.equal(await page.locator('#password').inputValue(),'');
  await page.locator('#forget-wifi').click();await page.waitForFunction(()=>!document.getElementById('save-wifi').disabled);
  assert.equal(wifiCalls[1].pathname,'/setup/wifi/forget');
  async function upload(buffer,name='esp13.bin'){
   await page.locator('#firmware').setInputFiles({name,mimeType:'application/octet-stream',buffer});
   await page.locator('#upload-button').click();
  }
  await upload(Buffer.from('not ESP firmware'));
  await page.waitForFunction(()=>document.getElementById('update-message').classList.contains('error'));
  assert.equal(uploadCount,0);
  const valid=Buffer.concat([Buffer.from([0xe9,1,3,0x20]),Buffer.from('ANDEN_ESP13_OTA_V1')]);
  const wrongProfile=Buffer.from(valid);wrongProfile[3]=0x40;await upload(wrongProfile);
  assert.equal(uploadCount,0);
  await upload(Buffer.concat([valid,Buffer.alloc(448*1024)]));assert.equal(uploadCount,0);
  await upload(valid);await page.waitForFunction(()=>document.getElementById('update-message').textContent.includes('test rejection'));
  assert.equal(uploadCount,1);assert.equal(await page.locator('#upload-button').isDisabled(),false);
  acceptUpload=true;await upload(valid);
  await page.waitForFunction(()=>document.getElementById('update-message').textContent.includes('Back online — firmware v0.1.2.'));
  assert.equal(uploadCount,2);assert.equal(await page.locator('#version').textContent(),'v0.1.2');
  current={...current,ssid:'<img src=x onerror=alert(1)>'};await page.evaluate(()=>refreshStatus());
  assert.equal(await page.locator('#ssid img').count(),0);assert.equal(await page.locator('#ssid').textContent(),current.ssid);
  current={...current,ssid:'Workshop Wi-Fi'};await page.evaluate(()=>refreshStatus());
  await page.screenshot({path:path.join(out,'esp13-setup-desktop.png'),fullPage:true});
  for(const width of [390,320]){
   await page.setViewportSize({width,height:1100});
   assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
  }
  await page.evaluate(()=>new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve))));
  await page.waitForTimeout(150);
  await page.screenshot({path:path.join(out,'esp13-setup-mobile.png'),fullPage:true});
  offline=true;await page.evaluate(()=>refreshStatus());
  assert.equal(await page.locator('#version').textContent(),'Offline');assert.equal(await page.locator('#upload-button').isDisabled(),true);
  assert.deepEqual(errors,[]);
  console.log('PASS: ESP13 bounded DC/DCC bridge tests/roster-readiness/mode/stop/completion/cutoff/legacy guards, Uno ping/LED/offline/enable guards, setup collapsed, Wi-Fi save/forget/drafts, safe SSID text, OTA wrong-image/profile/size rejection, failed upload recovery, verified reboot, offline state, desktop/mobile.');
 }finally{await browser.close()}
})().catch(error=>{console.error(error);process.exit(1)});
