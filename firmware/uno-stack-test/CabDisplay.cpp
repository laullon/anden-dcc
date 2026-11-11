#include "CabDisplay.h"
#include <Arduino.h>
#include <Wire.h>
#include <avr/pgmspace.h>

namespace {
constexpr uint8_t OLED = 0x3C, WIDTH = 128, TEXT_WIDTH = 60, CHUNK = 24;

// Five columns per glyph, stored in flash. Render at 2x without a framebuffer.
const uint8_t FONT[][5] PROGMEM = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
};

void startPage(uint8_t page, uint8_t column) {
  Wire.beginTransmission(OLED);
  Wire.write(0x80); // One command follows each continuation control byte.
  Wire.write(0xB0 | page);
  Wire.write(0x80);
  Wire.write(column & 0x0F);
  Wire.write(0x80);
  Wire.write(0x10 | (column >> 4));
  Wire.write(0x40); // Remaining bytes are display data.
}

uint8_t textColumn(uint8_t glyph, uint8_t column, bool lower) {
  if (glyph >= sizeof(FONT) / sizeof(FONT[0]) || column >= 5)
    return 0;
  uint8_t source = pgm_read_byte(&FONT[glyph][column]);
  if (lower)
    source >>= 4;
  uint8_t pixels = 0;
  for (uint8_t row = 0; row < 4; ++row)
    if (source & (1 << row))
      pixels |= 3 << (row * 2);
  return pixels;
}
} // namespace

void CabDisplay::begin() {
  Wire.begin();
  Wire.setClock(400000);
  Wire.setWireTimeout(2000, true);
  // SSD1315/SSD1306, 64 rows, internal charge pump and page addressing.
  const uint8_t init[] = {0xAE, 0xD5, 0x80, 0xA8, 63,   0xD3, 0,    0x40, 0x8D,
                          0x14, 0x20, 2,    0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x8F,
                          0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0x2E};
  Wire.beginTransmission(OLED);
  Wire.write(uint8_t(0));
  Wire.write(init, sizeof(init));
  if (Wire.endTransmission())
    return; // An absent display has no effect on cab operation.

  for (uint8_t row = 0; row < 8; ++row) {
    for (uint8_t x = 0; x < WIDTH; x += CHUNK) {
      startPage(row, x);
      for (uint8_t i = 0; i < CHUNK && x + i < WIDTH; ++i)
        Wire.write(uint8_t(0));
      if (Wire.endTransmission())
        return;
    }
  }
  Wire.beginTransmission(OLED);
  Wire.write(uint8_t(0));
  Wire.write(0xAF);
  ready = Wire.endTransmission() == 0;
}

void CabDisplay::update(uint8_t nextAddress, uint8_t nextSpeed) {
  if (!ready)
    return;
  if (nextAddress != address || nextSpeed != speed) {
    address = nextAddress;
    speed = nextSpeed;
    page = column = 0;
  }
  const uint32_t now = millis();
  if (page >= 4 || uint32_t(now - lastWrite) < 5)
    return;
  lastWrite = now;

  // Send one bounded transaction per service call, including during movement.
  uint8_t glyphs[5] = {10, 255, 12, 12, 12}; // A ---
  if (page >= 2) {
    glyphs[0] = 11; // S 00
    glyphs[2] = speed / 10;
    glyphs[3] = speed % 10;
    glyphs[4] = 255;
  } else if (address) {
    glyphs[2] = address / 100;
    glyphs[3] = address / 10 % 10;
    glyphs[4] = address % 10;
  }
  // Address uses rows 0–15; speed uses rows 32–47 in the blue region.
  // Only the four text pages need refreshing after the full startup clear.
  startPage(page < 2 ? page : page + 2, column);
  for (uint8_t i = 0; i < CHUNK && column < TEXT_WIDTH; ++i, ++column)
    Wire.write(textColumn(glyphs[column / 12], column % 12 / 2, page & 1));
  if (Wire.endTransmission())
    ready = false;
  if (column == TEXT_WIDTH) {
    column = 0;
    ++page;
  }
}
