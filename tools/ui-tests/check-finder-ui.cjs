const {chromium}=require('playwright');
const assert=require('node:assert/strict'),fs=require('fs'),path=require('path');
const root=path.resolve(__dirname,'../..'),out=process.env.UI_ARTIFACTS||path.join(root,'build/ui');fs.mkdirSync(out,{recursive:true});
(async()=>{
 const browser=await chromium.launch({headless:true,...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{})});
 try {
  const context=await browser.newContext({viewport:{width:1100,height:1100}});
  let state={name:'anden-dcc',version:'0.6.0',connected:true,mdnsReady:true,storageReady:true,ssid:'Studio Wi-Fi',ip:'192.168.1.42',trackPower:false,dcc:{ready:true,running:true,maxLocos:8,locos:[{id:1,address:3,name:'Shunter',f0:true,bytes:'03 90 93'}],finder:{stage:'idle',active:false,step:0}}};
  let reject=false,offline=false,requests=0,readyAt=0,cleanup=null;
  function result(){const f=state.dcc.finder;f.waitMs=Math.max(0,readyAt-Date.now());return state.dcc;}
  function finish(ok){const f=state.dcc.finder;f.stage='cleanup';f.active=true;f.result=ok?f.low:0;f.step++;cleanup=setTimeout(()=>{f.stage=ok?'found':'failed';f.active=false;},100);}
  await context.route('http://finder.test/**',async route=>{
   const req=route.request(),u=new URL(req.url()),data=new URLSearchParams(req.postData());
   if(u.pathname==='/api/status'){if(offline)return route.abort();result();return route.fulfill({json:state});}
   if(u.pathname.startsWith('/finder/')){
    requests++;if(reject){reject=false;return route.fulfill({status:409});}
    const f=state.dcc.finder;
    if(u.pathname==='/finder/start'){
     Object.assign(f,{stage:'verifyOn',active:true,step:f.step+1,first:+data.get('first'),last:+data.get('last'),low:+data.get('first'),high:+data.get('last'),result:0});readyAt=Date.now()+1100;
    } else if(u.pathname==='/finder/cancel'){if(f.active)finish(false);else Object.assign(f,{stage:'idle',result:0,step:f.step+1});}
    else {
     const answer=data.get('answer');if(answer==='retry'){f.step++;return route.fulfill({json:result()});}
     f.step++;
     if(f.stage==='verifyOn')f.stage='verifyOff';
     else if(f.stage==='verifyOff')f.stage=f.low===f.high?'confirmOn':'split';
     else if(f.stage==='split'){const m=Math.floor((f.low+f.high)/2);if(answer==='on')f.high=m;else f.low=m+1;f.stage=f.low===f.high?'confirmOn':'split';}
     else if(f.stage==='confirmOn')f.stage='confirmOff';
     else if(f.stage==='confirmOff')finish(answer==='off');
    }
    f.middle=Math.floor((f.low+f.high)/2);return route.fulfill({json:result()});
   }
   if(u.pathname==='/locos/save'){
    state.dcc.locos.push({id:2,address:+data.get('address'),name:data.get('name'),f0:false,bytes:'25 80 A5'});return route.fulfill({json:result()});
   }
   const filename=u.pathname==='/'?'index.html':u.pathname.slice(1);
   const asset=path.join(root,'web',filename);
   return route.fulfill({body:fs.existsSync(asset)?fs.readFileSync(asset):'',contentType:filename.endsWith('.html')?'text/html':filename.endsWith('.svg')?'image/svg+xml':'application/octet-stream'});
  });
  const page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.goto('http://finder.test/');await page.waitForFunction(()=>document.getElementById('connection-text').textContent==='Connected');
  assert(!await page.locator('#finder-panel').isVisible());await page.locator('#find-address').click();
  await page.locator('#finder-first').fill('90');await page.locator('#finder-last').fill('10');
  await page.locator('#finder-range button').click();assert.equal(requests,0);assert.match(await page.locator('#finder-message').textContent(),/range/);
  await page.locator('#finder-first').fill('1');await page.locator('#finder-last').fill('100');
  await page.locator('#finder-range button').click();await page.waitForFunction(()=>document.getElementById('finder-question').textContent.includes('First check'));
  assert(await page.locator('[data-finder-answer="on"]').isDisabled());assert(await page.locator('#add-loco').isDisabled());assert(await page.locator('.dcc-action').first().isDisabled());assert(await page.locator('#update-button').isDisabled());
  const second=await context.newPage();await second.goto('http://finder.test/');await second.waitForFunction(()=>!document.getElementById('finder-panel').hidden);assert(await second.locator('#add-loco').isDisabled());await second.close();
  await page.waitForFunction(()=>!document.querySelector('[data-finder-answer="on"]').disabled);
  await page.screenshot({path:path.join(out,'finder-check.png'),fullPage:true});
  async function answer(a){const step=state.dcc.finder.step;await page.locator(`[data-finder-answer="${a}"]`).click();await page.waitForFunction(step=>state.dcc.finder.step!==step,step);}
  await answer('on');await answer('off');
  const before=[state.dcc.finder.low,state.dcc.finder.high];await answer('retry');assert.deepEqual([state.dcc.finder.low,state.dcc.finder.high],before);
  reject=true;await page.locator('[data-finder-answer="on"]').click();await page.waitForFunction(()=>document.getElementById('finder-message').classList.contains('error'));assert.deepEqual([state.dcc.finder.low,state.dcc.finder.high],before);
  await page.screenshot({path:path.join(out,'finder-search.png'),fullPage:true});
  while(state.dcc.finder.stage==='split')await answer(37<=state.dcc.finder.middle?'on':'off');
  assert.equal(state.dcc.finder.low,37);await answer('on');await answer('off');
  await page.waitForTimeout(150);await page.evaluate(()=>refresh());assert.equal(state.dcc.finder.stage,'found');
  assert.match(await page.locator('#finder-question').textContent(),/37/);assert.equal(state.dcc.locos[0].f0,true);
  await page.screenshot({path:path.join(out,'finder-found.png'),fullPage:true});
  await page.locator('#finder-add').click();assert.equal(await page.locator('#loco-address').inputValue(),'37');
  await page.locator('#loco-name').fill('Found express');await page.locator('#save-loco').click();await page.waitForFunction(()=>!document.getElementById('loco-dialog').open);assert.equal(state.dcc.locos.length,2);
  assert.match(await page.locator('#finder-add').textContent(),/Already/);await page.locator('#finder-add').click();assert.equal(await page.locator('#loco-id').inputValue(),'2');await page.locator('#cancel-editor').click();
  await page.locator('#finder-again').click();await page.locator('#finder-first').fill('10');await page.locator('#finder-last').fill('45');await page.evaluate(()=>refresh());assert.equal(await page.locator('#finder-first').inputValue(),'10');
  await page.locator('[data-lang="es"]').click();assert.equal(await page.locator('#find-address').textContent(),'Buscar una dirección');await page.setViewportSize({width:320,height:1100});assert(!await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth));
  await page.locator('#finder-range button').click();await page.waitForFunction(()=>document.getElementById('finder-question').textContent.includes('Primera prueba'));
  await page.screenshot({path:path.join(out,'finder-mobile-es.png'),fullPage:true});
  offline=true;await page.evaluate(()=>refresh());assert(await page.locator('#finder-cancel').isDisabled());offline=false;await page.evaluate(()=>refresh());
  await page.locator('#finder-cancel').click();await page.waitForTimeout(150);await page.evaluate(()=>refresh());assert.equal(state.dcc.finder.stage,'failed');assert(await page.locator('#finder-answers').isHidden());assert(!await page.locator('#add-loco').isDisabled());
  await page.locator('#finder-close').click();await page.waitForFunction(()=>document.getElementById('finder-panel').hidden);assert(!await page.locator('#finder-panel').isVisible());assert.equal(state.dcc.finder.stage,'idle');
  assert.deepEqual(errors,[]);console.log('PASS: finder range validation, settling, roster/OTA locks, shared session, retries, stale-request recovery, binary flow to 37, confirmation, add/existing editor, cancellation/offline, draft preservation, EN/ES and 320px layout.');
  if(cleanup)clearTimeout(cleanup);
 } finally {await browser.close();}
})().catch(e=>{console.error(e);process.exit(1)});
