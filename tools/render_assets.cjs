// Optional asset preview helper. Install tools/ui-tests dependencies first.
const {chromium}=require('./ui-tests/node_modules/playwright');
const fs=require('fs'),path=require('path');
const root=path.resolve(__dirname,'..');
const out=process.env.UI_ARTIFACTS||path.join(root,'build/ui');fs.mkdirSync(out,{recursive:true});
(async()=>{const browser=await chromium.launch({...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{}),headless:true});

for(const [name,w,h,target] of [['icon',512,512,'assets/icon-512.png'],['banner',1280,420,'assets/banner.png'],['power',1200,850,null],['control',1200,850,null],['protection',1200,850,null]]){
 const context=await browser.newContext({viewport:{width:w,height:h}});const page=await context.newPage();
 const source=path.join(root,name==='icon'||name==='banner'?'assets':'docs/hardware',name+'.svg');
 let svg=fs.readFileSync(source,'utf8');svg=svg.replace(/<svg /,'<svg style="width:100%;height:100%;display:block" ');
 await page.setContent('<!doctype html><html><body style="margin:0;width:'+w+'px;height:'+h+'px">'+svg+'</body></html>');
 const dest=target?path.join(root,target):path.join(out,'wiring-'+name+'.png');await page.evaluate(()=>new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve))));await page.screenshot({path:dest,omitBackground:name==='icon'});await context.close();
}
await browser.close();console.log('Rendered icon, banner and three wiring previews.');})().catch(e=>{console.error(e);process.exit(1)});
