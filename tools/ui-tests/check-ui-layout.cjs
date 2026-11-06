const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../..');
(async()=>{const browser=await chromium.launch({headless:true,...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{})});try{
 const page=await browser.newPage({viewport:{width:320,height:900}}),errors=[],failures=[];page.on('pageerror',error=>errors.push(error.message));
 const state={appId:'ANDEN_ESP13_OTA_V1',version:'0.6.1',connected:true,configured:true,ssid:'Workshop',ip:'192.168.1.43',apIp:'192.168.13.1',apReady:true,mdnsReady:true,otaMaxBytes:458752,freeHeap:32000,storageReady:true,uno:{online:true,version:'0.6.0',trackSupported:true,trackActive:true,enableA:true,enableB:false,adcA:0,adcB:0,roundTripMs:8,rxFrames:3},dcc:{ready:true,running:true,functionsSupported:true,packets:100,timingFaults:0,finder:{stage:'idle',active:false},locos:[{id:1,address:3,name:'Night express',type:'electric',f0:false,forward:true,functions:0,favorites:[{number:0,name:'ABCDEFGHIJKLMNOPQRSTUVWX'}]}]}};
 let offline=false;
 await page.route('http://layout.test/**',route=>{const request=route.request();if(new URL(request.url()).pathname==='/api/status'&&offline)return route.abort();assert.equal(request.method(),'GET','Layout checks must never send controller commands');return route.fulfill(new URL(request.url()).pathname==='/api/status'?{json:state}:{contentType:'text/html',body:fs.readFileSync(path.join(root,'firmware/esp13-ota/web/index.html'),'utf8')})});
 await page.goto('http://layout.test/');await page.waitForFunction(()=>document.querySelector('.loco'));
 const card=page.locator('.loco'),row=card.locator('.loco-bottom').first(),name=card.locator('.function-name');
 for(const theme of ['light','dark']){
  await page.locator('#theme [data-theme-choice='+theme+']').click();
  await row.evaluate(element=>element.scrollIntoView({block:'center'}));
  const cardBox=await card.boundingBox();
  for(const on of [0,1]){
   const button=row.locator('[data-on="'+on+'"]'),box=await button.boundingBox();
   try{assert(box.x>=cardBox.x&&box.x+box.width<=cardBox.x+cardBox.width,theme+' F0 '+(on?'On':'Off')+' must fit inside its clipped card; got '+JSON.stringify({box,cardBox}));assert(await button.evaluate(element=>{const r=element.getBoundingClientRect();return document.elementFromPoint(r.x+r.width/2,r.y+r.height/2)?.closest('button')===element}),theme+' F0 button must be reachable by mouse/touch')}catch(error){failures.push(error.message)}
  }
  if((await card.locator('details').getAttribute('open'))===null)await card.locator('summary').click();
  // A field may be fully inside the viewport while covered by the fixed toolbar.
  // Native Tab then leaves its position alone, so scroll-padding must reserve room.
  await name.evaluate(element=>element.scrollIntoView({block:'end'}));
  await page.locator('#floating-power').waitFor({state:'visible'});
  await card.locator('.function-on').evaluate(element=>element.focus({preventScroll:true}));
  await page.keyboard.press('Tab');assert(await name.evaluate(element=>element===document.activeElement));
  const box=await name.boundingBox(),bar=await page.locator('#floating-power').boundingBox();
  try{assert(box.y+box.height<=bar.y,theme+' keyboard-focused name must stay above the floating power bar; got '+JSON.stringify({box,bar}));assert(await name.evaluate(element=>{const r=element.getBoundingClientRect();return document.elementFromPoint(r.x+r.width/2,r.y+r.height/2)===element}),theme+' name field must be reachable by mouse/touch')}catch(error){failures.push(error.message)}
 }
 // An editor opened while connected must react to later controller changes.
 state.uno.trackActive=state.uno.enableA=false;await page.evaluate(()=>refreshStatus());
 await card.locator('.edit-loco').click();await page.locator('#loco-name').fill('Draft stays here');await page.locator('#loco-address').fill('37');await page.locator('#loco-type').selectOption('steam');
 const checkDraft=async()=>{assert.equal(await page.locator('#loco-name').inputValue(),'Draft stays here');assert.equal(await page.locator('#loco-address').inputValue(),'37');assert.equal(await page.locator('#loco-type').inputValue(),'steam')};
 const checkGuard=async(reason,editable=true)=>{
  assert(await page.locator('#save-loco').isDisabled(),reason+' must disable Save');assert(await page.locator('#remove-loco').isDisabled(),reason+' must disable Remove');await checkDraft();
  if(editable){for(const selector of ['#loco-name','#loco-address','#loco-type','#cancel-loco'])assert.equal(await page.locator(selector).isDisabled(),false,reason+' must preserve editable drafts and Cancel')}
  assert(await page.locator('#editor-guard').isVisible(),reason+' needs a visible explanation');assert((await page.locator('#editor-guard').innerText()).trim(),reason+' needs an explanation');
 };
 const checkRecovered=async()=>{await page.evaluate(()=>refreshStatus());assert.equal(await page.locator('#save-loco').isDisabled(),false);assert.equal(await page.locator('#remove-loco').isDisabled(),false);assert.equal(await page.locator('#editor-guard').isVisible(),false);await checkDraft()};
 offline=true;await page.evaluate(()=>refreshStatus());await checkGuard('ESP disconnected');offline=false;await checkRecovered();
 state.uno.online=false;await page.evaluate(()=>refreshStatus());await checkGuard('Uno disconnected');state.uno.online=true;await checkRecovered();
 state.uno.trackActive=state.uno.enableA=true;await page.evaluate(()=>refreshStatus());await checkGuard('Track power became active');state.uno.trackActive=state.uno.enableA=false;await checkRecovered();
 state.uno.enableB=true;await page.evaluate(()=>refreshStatus());await checkGuard('Another output became active');state.uno.enableB=false;await checkRecovered();
 state.dcc.finder.active=true;await page.evaluate(()=>refreshStatus());await checkGuard('Address search became active');state.dcc.finder.active=false;await checkRecovered();
 state.storageReady=false;await page.evaluate(()=>refreshStatus());await checkGuard('Storage unavailable');state.storageReady=true;await checkRecovered();
 await page.evaluate(()=>{rosterBusy=true;lock(true)});await checkGuard('Own save pending',false);await page.evaluate(()=>{rosterBusy=false;lock(false)});await checkRecovered();
 await page.locator('#cancel-loco').click();assert.equal(await page.locator('#loco-dialog').evaluate(element=>element.open),false);
 assert.deepEqual(errors,[]);assert.deepEqual(failures,[]);
 console.log('PASS: long F0 names keep both controls inside their cards, and scrolled/native-keyboard focus remains above compact power in 320px light/dark themes; open editor guards offline, output/search/storage changes and pending save without losing drafts.');
}finally{await browser.close()}})().catch(error=>{console.error(error);process.exit(1)});
