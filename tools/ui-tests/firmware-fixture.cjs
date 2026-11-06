// Independent bitwise reference for the ESP8266 Arduino core image metadata.
module.exports=function firmwareImage(){
 const bytes=Buffer.alloc(4176);Buffer.from([0xe9,2,3,0x20]).copy(bytes,0);Buffer.from([0xe9,5,3,0x20]).copy(bytes,4096);
 bytes.write('ANDEN_ESP13_OTA_V1',4120);
 let crc=0xffffffff;
 for(const byte of bytes){crc^=byte<<24;for(let bit=0;bit<8;bit++)crc=crc&0x80000000?(crc<<1)^0x04c11db7:crc<<1;}
 bytes.writeUInt32LE(bytes.length,4112);bytes.writeUInt32LE(crc>>>0,4116);return bytes;
};
