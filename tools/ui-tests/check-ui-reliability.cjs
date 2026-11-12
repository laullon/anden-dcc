const {chromium}=require('playwright'),assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../..');
const source=()=>fs.readFileSync(path.join(root,'firmware/esp13-ota/web/index.html'),'utf8');
(async()=>{
 const browser=await chromium.launch({headless:true,...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{})});
 const failures=[];
 async function fixture(){
  const context=await browser.newContext({viewport:{width:1180,height:1100}}),page=await context.newPage(),errors=[],calls=[],pending=[];
  page.on('pageerror',e=>errors.push(e.message));await page.clock.install();
  const state={appId:'ANDEN_ESP13_OTA_V1',version:'0.6.1',bootId:10,connected:true,configured:true,ssid:'Workshop',ip:'192.168.1.43',apIp:'192.168.13.1',apReady:true,mdnsReady:true,otaMaxBytes:458752,freeHeap:32000,storageReady:true,uno:{online:true,version:'0.6.0',trackSupported:true,trackActive:false,enableA:false,enableB:false,adcA:0,adcB:0,rxFrames:3,roundTripMs:8,bench:{supported:true,dccSupported:true,active:false,state:0,mode:0}},dcc:{ready:true,running:true,functionsSupported:true,packets:100,timingFaults:0,finder:{stage:'idle',active:false,first:1,last:100,step:0},locos:[{onDiorama:true,id:1,address:3,name:'Night express',type:'electric',f0:false,forward:true,functions:0,favorites:[]},{onDiorama:true,id:2,address:6,name:'Freight',type:'diesel',f0:false,forward:true,functions:0,favorites:[]}]}};
  const control={holdPost:null,holdStatus:false,holdAfterFavorite:false,holdAfterPower:false,statusRequests:0,pendingPost:null,pendingStatus:null};
  const hold=async(type)=>{let resolve;const wait=new Promise(r=>resolve=r);pending.push(resolve);control[type]=resolve;await wait;control[type]=null};
  await context.route('http://reliable.test/**',async route=>{const req=route.request(),url=new URL(req.url()),fields=new URLSearchParams(req.postData());
   if(url.pathname==='/api/status'){
    control.statusRequests++;const body=JSON.stringify(state);if(control.holdStatus){control.holdStatus=false;await hold('pendingStatus')}
    return route.fulfill({body,contentType:'application/json'}).catch(()=>{});
   }
   if(req.method()==='POST'){
    calls.push(url.pathname);
    if(control.holdPost===url.pathname)await hold('pendingPost');
    if(url.pathname==='/api/track/on'){state.uno.enableA=state.uno.trackActive=true;if(control.holdAfterPower)control.holdStatus=true;return route.fulfill({json:{lease:1}}).catch(()=>{})}
    if(url.pathname==='/api/track/off'){state.uno.enableA=state.uno.trackActive=false;if(control.holdAfterPower)control.holdStatus=true;return route.fulfill({json:{ok:true}}).catch(()=>{})}
    if(url.pathname==='/api/track/keep')return route.fulfill({json:{ok:true}}).catch(()=>{});
    if(url.pathname==='/dcc/function'||url.pathname==='/dcc/f0'){
     const loco=state.dcc.locos.find(l=>l.address===+fields.get('address')),n=url.pathname==='/dcc/f0'?0:+fields.get('function');
     if(fields.get('on')==='1')loco.functions|=1<<n;else loco.functions&=~(1<<n);loco.f0=!!(loco.functions&1);
    }else if(url.pathname==='/locos/favorite'){
     const loco=state.dcc.locos.find(l=>l.id===+fields.get('id'));loco.favorites=[{number:+fields.get('function'),name:fields.get('name')}];
     if(control.holdAfterFavorite)control.holdStatus=true;
    }else if(url.pathname==='/finder/start'){
     state.dcc.finder={stage:'verifyOn',active:true,first:1,last:100,low:1,high:100,step:1,waitMs:10000};state.dcc.ready=false;
    }else if(url.pathname==='/api/uno/ping')return route.fulfill({json:state.uno}).catch(()=>{});
    else if(url.pathname==='/setup/wifi')return route.fulfill({json:{message:'Saved'}}).catch(()=>{});
    else throw Error('Unexpected POST '+url.pathname);
    return route.fulfill({json:state.dcc}).catch(()=>{});
   }
   return route.fulfill({contentType:'text/html',body:source()});
  });
  await page.goto('http://reliable.test/');await page.waitForFunction(()=>document.querySelectorAll('.loco').length===2&&!statusRefreshing);
  await page.locator('#track-on').click();await page.waitForFunction(()=>!trackBusy&&!statusRefreshing&&state.uno.trackActive);
  const card=page.locator('.loco[data-id="1"]');
  return {context,page,card,state,control,calls,errors,async close(){for(const resolve of pending)resolve();await context.close()}};
 }
 const tests={
  async timeout(){
   const f=await fixture();try{
    await f.card.locator('summary').click();f.control.holdPost='/dcc/function';await f.card.locator('.function-test-toggle').click();
    await f.page.waitForFunction(()=>rosterBusy);assert.equal(await f.card.locator('.function-test-toggle').isDisabled(),true);
    await f.page.clock.fastForward(11000);
    await f.page.waitForFunction(()=>!rosterBusy,{},{timeout:2000});
    await f.page.waitForFunction(()=>document.getElementById('roster-message').classList.contains('error'),{},{timeout:2000});
    assert.equal(await f.card.locator('.function-test-toggle').isDisabled(),false,'Timed-out function request must release controls');
    assert.deepEqual(f.errors,[]);
   }finally{await f.close()}
  },
  async finder_exclusion(){
   const f=await fixture();try{
    await f.page.locator('#find-address').click();f.control.holdPost='/finder/start';await f.page.locator('#finder-range button').click();
    await f.page.waitForFunction(()=>finderBusy);
    assert.equal(await f.card.locator('.favorite-strip [data-function="0"]').isDisabled(),true,'Pending finder start must lock locomotive commands immediately');
    const before=f.calls.length;
    await f.page.evaluate(()=>{rosterCommand('/dcc/f0',{address:3,on:1});stackAction('ping')});await f.page.waitForTimeout(50);
    assert.equal(f.calls.length,before,'Shared command guards must reject roster/diagnostic commands during finder start');
    assert.deepEqual(f.errors,[]);
   }finally{await f.close()}
  },
  async finder_clock_changes(){
   const f=await fixture();try{
    await f.page.locator('#find-address').click();await f.page.locator('#finder-range button').click();
    await f.page.waitForFunction(()=>!finderBusy&&!statusRefreshing&&state.dcc.finder.active);
    const answer=f.page.locator('[data-finder-answer="on"]');assert(await answer.isDisabled());
    const before=await f.page.locator('#finder-wait').innerText();
    await f.page.clock.setSystemTime(new Date('2040-01-01T00:00:00Z'));await f.page.evaluate(()=>renderFinder());
    assert(await answer.isDisabled(),'A forward clock correction must not bypass the decoder settling interval');
    assert.equal(await f.page.locator('#finder-wait').innerText(),before);
    await f.page.clock.setSystemTime(new Date('2000-01-01T00:00:00Z'));await f.page.evaluate(()=>renderFinder());
    assert.equal(await f.page.locator('#finder-wait').innerText(),before,'A backward clock correction must not extend settling by years');
    assert.deepEqual(f.errors,[]);
   }finally{await f.close()}
  },
  async power_poll(){
   const f=await fixture();try{
    await f.page.locator('#track-off').click();await f.page.waitForFunction(()=>!trackTurningOff&&!statusRefreshing&&!state.uno.trackActive);
    f.control.holdPost='/api/track/on';await f.page.locator('#track-on').click();await f.page.waitForFunction(()=>trackBusy);
    const count=f.control.statusRequests;
    await f.page.evaluate(()=>{refreshStatus()});await f.page.waitForTimeout(30);
    assert.equal(f.control.statusRequests,count,'Background polls must not start while a power POST is pending');
    assert.deepEqual(f.errors,[]);
   }finally{await f.close()}
  },
  async power_confirmation(){
   const f=await fixture();try{
    await f.page.locator('#track-off').click();await f.page.waitForFunction(()=>!trackTurningOff&&!statusRefreshing&&!state.uno.trackActive);
    // Keep a pre-transition OFF snapshot in flight, then finish the ON request.
    f.control.holdStatus=true;await f.page.evaluate(()=>{refreshStatus()});await f.page.waitForTimeout(30);
    const stale=f.control.pendingStatus;assert.equal(typeof stale,'function');
    f.control.holdAfterPower=true;await f.page.locator('#track-on').click();
    await f.page.waitForTimeout(50);assert.equal(f.state.uno.trackActive,true);
    const fresh=f.control.pendingStatus;assert.equal(typeof fresh,'function');
    assert.notEqual(fresh,stale,'The acknowledged transition must obtain a fresh status independently of the stale snapshot');
    stale();await f.page.waitForTimeout(30);
    assert.notEqual(await f.page.locator('#track-label').innerText(),'Track is OFF','ON acknowledgment must not briefly render an old OFF state');
    assert.equal(await f.page.locator('#track-on').isVisible()&&!(await f.page.locator('#track-on').isDisabled()),false,'Duplicate Power on must stay unavailable during confirmation');
    fresh();await f.page.waitForFunction(()=>document.getElementById('track-label').textContent==='Track is ON'&&!statusRefreshing);
    // The reverse transition also must not display the old ON state after acknowledgment.
    await f.page.locator('#track-off').click();await f.page.waitForTimeout(50);assert.equal(f.state.uno.trackActive,false);
    assert.notEqual(await f.page.locator('#track-label').innerText(),'Track is ON');
    assert.equal(typeof f.control.pendingStatus,'function');f.control.pendingStatus();
    await f.page.waitForFunction(()=>document.getElementById('track-label').textContent==='Track is OFF'&&!statusRefreshing);
    assert.deepEqual(f.errors,[]);
   }finally{await f.close()}
  },
  async favorite_post_draft(){
   const f=await fixture();try{
    await f.card.locator('summary').click();await f.card.locator('.function-name').fill('Engine sound');f.control.holdPost='/locos/favorite';
    await f.card.locator('.pin-function').click();await f.page.waitForFunction(()=>rosterBusy);
    assert.equal(await f.card.locator('.function-name').isDisabled(),false,'Typing should remain available during favorite save');
    await f.card.locator('.function-name').fill('Newer POST draft');await f.card.locator('.function-select').selectOption('2');await f.card.locator('.function-name').fill('Horn draft');
    f.control.pendingPost();await f.page.waitForFunction(()=>!rosterBusy&&!statusRefreshing);await f.page.waitForTimeout(50);
    assert.equal(await f.card.locator('.function-select').inputValue(),'2');assert.equal(await f.card.locator('.function-name').inputValue(),'Horn draft');
    assert.deepEqual(f.state.dcc.locos[0].favorites,[{number:1,name:'Engine sound'}],'The request must retain its original function/name while the editor moves on');
    assert.deepEqual(f.errors,[]);
   }finally{await f.close()}
  },
  async favorite_draft(){
   const f=await fixture();try{
    await f.card.locator('summary').click();await f.card.locator('.function-name').fill('Engine sound');f.control.holdAfterFavorite=true;
    await f.card.locator('.pin-function').click();await f.page.waitForFunction(()=>!rosterBusy&&!document.querySelector('.function-name').disabled);
    // The command has acknowledged but its refresh is still in flight. Editing is available again.
    await f.page.waitForTimeout(50);assert.equal(typeof f.control.pendingStatus,'function','Fixture must hold the post-save status poll');
    await f.card.locator('.function-name').fill('New unsaved draft');f.control.pendingStatus();
    await f.page.waitForFunction(()=>!statusRefreshing);await f.page.waitForTimeout(50);
    assert.equal(await f.card.locator('.function-name').inputValue(),'New unsaved draft','A late save/poll continuation must not erase newer typing');
    assert.deepEqual(f.errors,[]);
   }finally{await f.close()}
  }
 };
 try{
  for(const [name,run]of Object.entries(tests)){
   if(process.env.RELIABILITY_CASE&&process.env.RELIABILITY_CASE!==name)continue;
   try{await run();console.log('PASS: UI reliability '+name)}catch(error){failures.push(name+': '+error.message);console.error('FAIL:',name,error)}
  }
 }finally{await browser.close()}
 assert.deepEqual(failures,[]);
})().catch(e=>{console.error(e);process.exit(1)});
