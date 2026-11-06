#!/usr/bin/env python3
"""Exercise the real streaming OTA validator with complete and damaged images."""
from pathlib import Path
import os
import subprocess
import tempfile
from esp_image_fixture import APP_ID, make_image, seal_image

root = Path(__file__).resolve().parents[1]
firmware = root / 'firmware/esp13-ota'
with tempfile.TemporaryDirectory(prefix='anden-firmware-image-') as directory:
    folder = Path(directory)
    valid = make_image()
    (folder / 'valid.bin').write_bytes(valid)
    # Match the ID after an overlapping partial prefix, across each byte boundary.
    overlap = bytearray(valid)
    overlap[4120:4156] = b'ANDEN_ESP13_OTANDEN_ESP13_OTA_V1'.ljust(36, b'\0')
    (folder / 'overlap.bin').write_bytes(seal_image(overlap))
    wrong_app = bytearray(valid)
    wrong_app[4120:4120+len(APP_ID)] = b'x' * len(APP_ID)
    (folder / 'other.bin').write_bytes(seal_image(wrong_app))
    corrupt = bytearray(valid)
    corrupt[-1] ^= 1
    (folder / 'corrupt.bin').write_bytes(corrupt)
    (folder / 'short.bin').write_bytes(valid[:-1])
    (folder / 'extra.bin').write_bytes(valid+b'\0')
    for offset in (0, 3, 4096, 4099):
        wrong = bytearray(valid)
        wrong[offset] = 0
        (folder / f'header-{offset}.bin').write_bytes(seal_image(wrong))
    (folder / 'check.cpp').write_text(r'''
#include "FirmwareImage.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
std::vector<uint8_t> read(const char* path) {
  std::ifstream file(path, std::ios::binary);
  assert(file);
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
}
bool check(const std::vector<uint8_t>& data, size_t chunk) {
  FirmwareImage image;
  image.begin(FirmwareImage::LIMIT);
  size_t offset=0;
  while(offset<data.size()) {
    size_t count=std::min(chunk,data.size()-offset);
    if(!image.append(data.data()+offset,count)) return false;
    offset+=count;
  }
  bool valid=image.finish();
  assert(valid == (image.error()==nullptr));
  if(valid) assert(image.size()==data.size() && image.finish());
  return valid;
}
int main(int argc,char** argv) {
  assert(argc>=11);
  auto valid=read(argv[1]);
  for(size_t chunk : {1u,2u,7u,17u,511u,512u,1024u,4096u,5000u})
    assert(check(valid,chunk));
  auto overlap=read(argv[2]);
  for(size_t chunk=1;chunk<40;++chunk) assert(check(overlap,chunk));
  for(int i=3;i<11;++i) {
    auto invalid=read(argv[i]);
    for(size_t chunk : {1u,511u,4096u,5000u}) assert(!check(invalid,chunk));
  }
  // Every boundary inside headers, embedded size/CRC, and app identifier.
  for(size_t split=4088;split<4150;++split) {
    FirmwareImage image;image.begin(valid.size());
    assert(image.append(valid.data(),split));
    assert(image.append(valid.data()+split,valid.size()-split));
    assert(image.finish());
  }
  FirmwareImage image;image.begin(valid.size()-1);
  assert(!image.append(valid.data(),valid.size()) && image.error());
  image.begin(0);assert(!image.append(valid.data(),1) && !image.finish());
  image.begin(FirmwareImage::LIMIT);assert(image.append(nullptr,0));
  assert(!image.append(nullptr,1));
  image.begin(FirmwareImage::LIMIT);assert(image.append(valid.data(),valid.size()));
  assert(image.finish());assert(!image.append(valid.data(),1));
  // Optional real artifact: validate its entire image and metadata, not just a fixture.
  if(argc>11) assert(check(read(argv[11]),317));
}
''')
    binary = folder / 'check'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++11','-Wall','-Wextra','-Werror',
                    '-I',str(firmware),str(folder/'check.cpp'),str(firmware/'FirmwareImage.cpp'),
                    '-o',str(binary)],check=True)
    args = [str(binary),*[str(folder/name) for name in ['valid.bin','overlap.bin','other.bin',
            'corrupt.bin','short.bin','extra.bin','header-0.bin','header-3.bin',
            'header-4096.bin','header-4099.bin']]]
    artifact = root / 'build/esp13-ota/esp13-ota.ino.bin'
    if artifact.exists():
        args.append(str(artifact))
    subprocess.run(args,check=True)
print('PASS: streaming firmware profile/identity/size/CRC, split metadata/identifier, corruption/truncation, capacity, restart and real artifact validation.')
