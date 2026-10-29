// Rasterize the small SVG mark into a classic 16/32px Windows ICO fallback.
// Uses the project's optional UI-tool dependencies, not the firmware build.
const {chromium}=require('./ui-tests/node_modules/playwright');
const fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'..');
function dib(size,rgba){
 const maskStride=Math.ceil(size/32)*4;
 const result=Buffer.alloc(40+size*size*4+maskStride*size);
 result.writeUInt32LE(40,0);result.writeInt32LE(size,4);result.writeInt32LE(size*2,8);
 result.writeUInt16LE(1,12);result.writeUInt16LE(32,14);result.writeUInt32LE(size*size*4,20);
 for(let y=0;y<size;y++)for(let x=0;x<size;x++){
  const src=(y*size+x)*4,dst=40+((size-1-y)*size+x)*4;
  result[dst]=rgba[src+2];result[dst+1]=rgba[src+1];result[dst+2]=rgba[src];result[dst+3]=rgba[src+3];
  if(rgba[src+3]===0)result[40+size*size*4+(size-1-y)*maskStride+(x>>3)]|=128>>(x&7);
 }
 return result;
}
(async()=>{
 const browser=await chromium.launch({...(process.env.CHROME_PATH?{executablePath:process.env.CHROME_PATH}:{}),headless:true});
 try{
  const page=await browser.newPage();
  const svg=fs.readFileSync(path.join(root,'web/favicon.svg'),'utf8'),frames=[];
  for(const size of [16,32]){
   const data=await page.evaluate(async({svg,size})=>{
    const img=new Image();img.src='data:image/svg+xml,'+encodeURIComponent(svg);await img.decode();
    const canvas=document.createElement('canvas');canvas.width=canvas.height=size;
    const context=canvas.getContext('2d');context.drawImage(img,0,0,size,size);
    return {rgba:Array.from(context.getImageData(0,0,size,size).data),png:canvas.toDataURL('image/png').split(',')[1]};
   },{svg,size});
   frames.push({size,bytes:dib(size,data.rgba)});
   if(process.env.FAVICON_PREVIEW_DIR){fs.mkdirSync(process.env.FAVICON_PREVIEW_DIR,{recursive:true});fs.writeFileSync(path.join(process.env.FAVICON_PREVIEW_DIR,'favicon-'+size+'.png'),Buffer.from(data.png,'base64'));}
  }
  const directory=Buffer.alloc(6+frames.length*16);directory.writeUInt16LE(1,2);directory.writeUInt16LE(frames.length,4);
  let offset=directory.length;
  frames.forEach(({size,bytes},index)=>{const pos=6+index*16;directory[pos]=directory[pos+1]=size;directory.writeUInt16LE(1,pos+4);directory.writeUInt16LE(32,pos+6);directory.writeUInt32LE(bytes.length,pos+8);directory.writeUInt32LE(offset,pos+12);offset+=bytes.length;});
  fs.writeFileSync(path.join(root,'web/favicon.ico'),Buffer.concat([directory,...frames.map(f=>f.bytes)]));
  console.log('Generated favicon.ico with 16px and 32px RGBA images.');
 }finally{await browser.close();}
})().catch(error=>{console.error(error);process.exit(1)});
