const {chromium}=require('playwright');
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../..'),out=process.env.UI_ARTIFACTS||path.join(root,'build/ui');
fs.mkdirSync(out,{recursive:true});
const state={name:'anden-esp13',appId:'ANDEN_ESP13_OTA_V1',version:'0.2.3',connected:true,configured:true,ssid:'Workshop',ip:'192.168.1.43',apIp:'192.168.13.1',apReady:true,mdnsReady:true,otaMaxBytes:458752,freeHeap:40000,storageReady:true,
 uno:{online:true,version:'0.2.0',enableA:false,enableB:false,adcA:0,adcB:0,rxFrames:5,roundTripMs:8},
 dcc:{ready:true,running:true,pin:10,packets:100,timingFaults:0,locos:[{id:1,address:3,name:'Morning express',type:'electric',f0:true,bytes:['03','90','93']},{id:2,address:4,name:'Depot diesel',type:'diesel',f0:false,bytes:['04','80','84']}]}};
function luminance(color){
 const rgb=color.match(/[\d.]+/g).slice(0,3).map(Number).map(v=>{v/=255;return v<=.04045?v/12.92:((v+.055)/1.055)**2.4});
 return rgb[0]*.2126+rgb[1]*.7152+rgb[2]*.0722;
}
(async()=>{const browser=await chromium.launch({...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{}),headless:true});
try{
 const context=await browser.newContext({viewport:{width:1180,height:1100},colorScheme:'light'}),errors=[],writes=[];let offline=false;
 async function route(route){const request=route.request(),url=new URL(request.url());
  if(request.method()!=='GET')writes.push(url.pathname);
  if(url.pathname==='/api/status')return offline?route.abort():route.fulfill({json:state});
  return route.fulfill({contentType:'text/html',body:fs.readFileSync(path.join(root,'experiments/esp13-ota/web/index.html'),'utf8')});
 }
 await context.route('http://themes.test/**',route);
 const page=await context.newPage();page.on('pageerror',e=>errors.push(e.message));
 const choice=value=>page.locator('#theme [data-theme-choice="'+value+'"]');
 const selected=async()=>{assert.equal(await page.locator('#theme [aria-pressed=true]').count(),1);return page.locator('#theme [aria-pressed=true]').getAttribute('data-theme-choice')};
 const theme=async expected=>{await page.waitForFunction(value=>document.documentElement.dataset.theme===value,expected)};
 await page.goto('http://themes.test/');await page.waitForFunction(()=>document.querySelectorAll('.loco').length===2);
 await theme('light');assert.equal(await selected(),'system');
 assert.equal(await page.locator('#theme select').count(),0);assert.equal(await page.getByRole('group',{name:'Theme',exact:true}).count(),1);assert.equal(await page.locator('#theme button').count(),3);
 await page.emulateMedia({colorScheme:'dark'});await theme('dark');
 await choice('light').click();await theme('light');await page.reload();await theme('light');
 assert.equal(await selected(),'light');
 await choice('dark').focus();await page.keyboard.press('Enter');await page.emulateMedia({colorScheme:'light'});await theme('dark');await page.reload();await theme('dark');
 assert.equal(await selected(),'dark');
 await choice('system').click();await theme('light');assert.equal(await page.evaluate(()=>localStorage.getItem('anden-esp13-theme')),null);
 await page.emulateMedia({colorScheme:'dark'});await theme('dark');
 await page.locator('.edit-loco').first().click();await page.locator('#loco-name').fill('An unfinished edit');
 await page.emulateMedia({colorScheme:'light'});await theme('light');assert.equal(await page.locator('#loco-name').inputValue(),'An unfinished edit');await page.locator('#cancel-loco').click();
 for(const value of ['light','dark']){
  await choice(value).click();await theme(value);await page.setViewportSize({width:1180,height:1100});
  assert.equal(await page.evaluate(()=>getComputedStyle(document.documentElement).colorScheme),value);
  assert.equal(await page.locator('meta[name="theme-color"]').getAttribute('content'),value==='dark'?'#0b121a':'#f6f3ec');
  // Check actual rendered foreground/background pairs, including both F0 states.
  for(const selector of ['h1','.intro','.loco-name','.loco-kind','.loco-bytes','#dcc-info','.light-pair button[aria-pressed=true]','.light-pair button[aria-pressed=false]','.theme-picker button[aria-pressed=true]','.theme-picker button[aria-pressed=false]','a#lan-link']){
   const colors=await page.locator(selector).first().evaluate(el=>{
    const color=getComputedStyle(el).color;let bg='rgba(0, 0, 0, 0)';
    for(let node=el;node;node=node.parentElement){bg=getComputedStyle(node).backgroundColor;if(bg!=='rgba(0, 0, 0, 0)')break}
    return {color,bg};
   });
   const a=luminance(colors.color),b=luminance(colors.bg),ratio=(Math.max(a,b)+.05)/(Math.min(a,b)+.05);
   assert(ratio>=4.5,value+' '+selector+' contrast '+ratio.toFixed(2));
  }
  await page.locator('#setup>summary').click();assert(await page.locator('#ssid-input').isVisible());
  await page.screenshot({path:path.join(out,'esp13-theme-'+value+'.png'),fullPage:true});
  await page.locator('#setup>summary').click();
  for(const width of [390,320]){
   await page.setViewportSize({width,height:1100});assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
   for(const mode of ['system','light','dark']){const box=await choice(mode).boundingBox();assert(box.height>=44&&box.x>=0&&box.x+box.width<=width)}
   await page.locator('.edit-loco').first().click();assert.equal(await page.locator('#loco-dialog').evaluate(el=>el.scrollWidth>el.clientWidth),false);await page.locator('#cancel-loco').click();
  }
  await page.screenshot({path:path.join(out,'esp13-theme-'+value+'-mobile.png'),fullPage:true});
 }
 offline=true;await page.evaluate(()=>refreshStatus());assert.equal(await page.locator('#version').innerText(),'Offline');
 await choice('light').click();await theme('light');assert.equal(await choice('light').isDisabled(),false);offline=false;
 // Invalid preferences and restricted storage must fall back safely to the OS.
 await page.evaluate(()=>localStorage.setItem('anden-esp13-theme','invalid'));await page.emulateMedia({colorScheme:'dark'});await page.reload();await theme('dark');assert.equal(await selected(),'system');
 const restricted=await browser.newContext({colorScheme:'light'});await restricted.route('http://themes.test/**',route);
 await restricted.addInitScript(()=>Object.defineProperty(window,'localStorage',{get(){throw new DOMException('Storage blocked','SecurityError')}}));
 const other=await restricted.newPage();other.on('pageerror',e=>errors.push(e.message));await other.goto('http://themes.test/');
 await other.locator('#theme [data-theme-choice=dark]').click();assert.equal(await other.evaluate(()=>document.documentElement.dataset.theme),'dark');
 await other.reload();assert.equal(await other.evaluate(()=>document.documentElement.dataset.theme),'light');
 assert.deepEqual(writes,[]);assert.deepEqual(errors,[]);
 console.log('PASS: ESP theme buttons/pressed states/keyboard, light/dark palettes and text contrast, OS following/live changes, saved overrides/reload, editable drafts, 320px dialog/layout, offline control, invalid/blocked storage and zero controller writes.');
}finally{await browser.close()}})().catch(e=>{console.error(e);process.exit(1)});
