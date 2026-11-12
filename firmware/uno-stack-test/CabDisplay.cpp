#include "CabDisplay.h"
#include <Arduino.h>
#include "CabI2c.h"
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>

namespace {
constexpr uint8_t OLED = 0x3C, WIDTH = 128, CHUNK = 24;

// ASCII space through Z, five columns per glyph, held in flash.
const uint8_t FONT[][5] PROGMEM = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // "'"
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 'Z'
};

void pageHeader(uint8_t* bytes, uint8_t page, uint8_t column) {
  bytes[0] = 0x80; // One command follows each continuation control byte.
  bytes[1] = 0xB0 | page;
  bytes[2] = 0x80;
  bytes[3] = column & 0x0F;
  bytes[4] = 0x80;
  bytes[5] = 0x10 | (column >> 4);
  bytes[6] = 0x40; // Remaining bytes are display data.
}

// Produce one column of a scaled text page without allocating a framebuffer.
uint8_t textColumn(const char* text, uint8_t x, uint8_t scale, uint8_t part = 0) {
  const uint8_t index = x / (6 * scale);
  if (index >= strlen(text))
    return 0;
  const uint8_t column = x / scale % 6;
  if (column == 5)
    return 0;
  char c = text[index];
  if (c < ' ' || c > 'Z')
    c = '?';
  const uint8_t source = pgm_read_byte(&FONT[c - ' '][column]);
  uint8_t pixels = 0;
  for (uint8_t bit = 0; bit < 8; ++bit) {
    const uint8_t row = (part * 8 + bit) / scale;
    if (row < 7 && (source & (1 << row)))
      pixels |= 1 << bit;
  }
  return pixels;
}
} // namespace

void CabDisplay::begin() {
  CabI2c::begin();
  // SSD1315/SSD1306, 64 rows, internal charge pump and page addressing.
  const uint8_t init[] = {0, 0xAE, 0xD5, 0x80, 0xA8, 63,   0xD3, 0,    0x40, 0x8D,
                          0x14, 0x20, 2,    0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x8F,
                          0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0x2E};
  if (!CabI2c::write(OLED, init, sizeof(init)))
    return; // An absent display has no effect on cab operation.

  for (uint8_t row = 0; row < 8; ++row) {
    for (uint8_t x = 0; x < WIDTH; x += CHUNK) {
      uint8_t bytes[CabI2c::MAX_BYTES] = {};
      pageHeader(bytes, row, x);
      const uint8_t count = WIDTH - x < CHUNK ? WIDTH - x : CHUNK;
      if (!CabI2c::write(OLED, bytes, 7 + count))
        return;
    }
  }
  const uint8_t on[] = {0, 0xAF};
  ready = CabI2c::write(OLED, on, sizeof(on));
}

void CabDisplay::update(uint8_t nextAddress, uint8_t nextSpeed, const char* nextName,
                        bool nextPowered) {
  if (!ready)
    return;
  if (nextAddress != address || nextSpeed != speed || nextPowered != powered ||
      strncmp(nextName, name, CabLabel::LENGTH)) {
    address = nextAddress;
    speed = nextSpeed;
    powered = nextPowered;
    strncpy(name, nextName, CabLabel::LENGTH);
    name[CabLabel::LENGTH] = 0;
    page = column = 0;
  }
  const uint32_t now = millis();
  if (page >= 8 || uint32_t(now - lastWrite) < 5)
    return;
  lastWrite = now;

  char number[6];
  if (address)
    snprintf(number, sizeof(number), "A %03u", unsigned(address));
  else
    strcpy(number, "A ---");
  const char* label = address ? (*name ? name : "LOCOMOTIVE") : "SELECT A LOCO";
  const uint8_t labelScale = strlen(label) <= 10 ? 2 : 1;
  const uint8_t labelStart = (WIDTH - strlen(label) * 6 * labelScale) / 2;

  // Build one bounded transaction. Polling its transfer leaves Timer1 free.
  // A redraw clears every column, so shorter names cannot leave old pixels.
  uint8_t bytes[CabI2c::MAX_BYTES];
  pageHeader(bytes, page, column);
  uint8_t count = 7;
  for (uint8_t i = 0; i < CHUNK && column < WIDTH; ++i, ++column) {
    uint8_t pixels = 0;
    if (page < 2) {
      pixels = textColumn(number, column, 2, page);
      if (page == 0 && column >= 104)
        pixels |= textColumn(powered ? "ON" : "OFF", column - 104, 1);
    } else if (page == 2 || (page == 3 && labelScale == 2)) {
      if (column >= labelStart)
        pixels = textColumn(label, column - labelStart, labelScale, page - 2);
    } else if (page == 3 && column >= 8 && column < 120) {
      pixels = 0x08; // Divider separates the name from the throttle.
    } else if (page >= 4 && page <= 6) {
      char step[3] = {char('0' + speed / 10), char('0' + speed % 10), 0};
      if (column >= 46 && column < 82)
        pixels = textColumn(step, column - 46, 3, page - 4);
      if (page == 5 && column >= 8 && column < 32)
        pixels |= textColumn("STEP", column - 8, 1);
      if (page == 5 && column >= 94)
        pixels |= textColumn(speed ? "MOVE" : "STOP", column - 94, 1);
    } else if (page == 7 && column >= 13 && column < 115) {
      const uint8_t segment = (column - 13) / 36;
      const uint8_t x = (column - 13) % 36;
      if (x < 30)
        pixels = speed > segment || x == 0 || x == 29 ? 0x3C : 0x24;
    }
    bytes[count++] = pixels;
  }
  if (!CabI2c::write(OLED, bytes, count))
    ready = false;
  if (column == WIDTH) {
    column = 0;
    ++page;
  }
}
