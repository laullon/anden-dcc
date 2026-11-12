#include "CabDisplay.h"
#include <Arduino.h>
#include "CabI2c.h"
#include "UnoDcc.h"
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

void CabDisplay::update(uint8_t nextAddress, uint8_t nextSpeed, const char* nextName) {
  if (!ready)
    return;
  if (nextAddress != address || nextSpeed != speed ||
      strncmp(nextName, name, CabLabel::LENGTH)) {
    address = nextAddress;
    speed = nextSpeed;
    strncpy(name, nextName, CabLabel::LENGTH);
    name[CabLabel::LENGTH] = 0;
    page = column = 0;
  }
  const uint32_t now = millis();
  if (page >= 8 || uint32_t(now - lastWrite) < 5)
    return;
  lastWrite = now;

  const char* label = address ? (*name ? name : "LOCOMOTIVE") : "SELECT A LOCO";
  const uint8_t labelScale = strlen(label) <= 10 ? 2 : 1;
  const uint8_t labelStart = (WIDTH - strlen(label) * 6 * labelScale) / 2;
  char number[4];
  snprintf(number, sizeof(number), "%u", unsigned(speed));
  const uint8_t numberStart = (WIDTH - strlen(number) * 24) / 2;
  const uint8_t level = speed > UnoDcc::MAX_CRAWL_STEP ? UnoDcc::MAX_CRAWL_STEP : speed;
  const uint8_t filled = uint16_t(level) * 106 / UnoDcc::MAX_CRAWL_STEP;

  // Build one bounded transaction. Polling its transfer leaves Timer1 free.
  // A redraw clears every column, so shorter names cannot leave old pixels.
  uint8_t bytes[CabI2c::MAX_BYTES];
  pageHeader(bytes, page, column);
  uint8_t count = 7;
  for (uint8_t i = 0; i < CHUNK && column < WIDTH; ++i, ++column) {
    uint8_t pixels = 0;
    if (page < 2 && column >= labelStart) {
      if (labelScale == 2) {
        pixels = textColumn(label, column - labelStart, 2, page);
      } else {
        // Centre a long, single-height name vertically in the yellow band.
        const uint16_t text = uint16_t(textColumn(label, column - labelStart, 1)) << 4;
        pixels = text >> (page * 8);
      }
    } else if (page >= 2 && page <= 5 && column >= numberStart) {
      pixels = textColumn(number, column - numberStart, 4, page - 2);
    } else if (page >= 6 && column >= 8 && column < 120) {
      // One rounded gauge, with third markers matching the three crawl steps.
      // The outline stays visible at zero; only the commanded portion fills.
      const uint8_t x = column - 8;
      uint16_t bar = 0x2010;
      if (x == 0 || x == 111)
        bar = 0x0780;
      else if (x == 1 || x == 110)
        bar = 0x1860;
      if (x >= 3 && x < 3 + filled)
        bar |= 0x0FC0;
      if (x == 37 || x == 74)
        bar |= 0x1020;
      pixels = bar >> ((page - 6) * 8);
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
