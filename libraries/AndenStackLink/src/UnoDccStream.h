#pragma once
#include "DccFunctions.h"
#include <stdint.h>

// Main code prepares the inactive bank only while no publication is pending.
// The ISR takes a pending bank only at a complete packet boundary.
class UnoDccStream {
public:
  static constexpr uint8_t CAPACITY = 8, BITS = 44;
  static constexpr uint16_t ONE = 116, ZERO = 200; // Timer1 /8, 0.5 us per tick.
  // Legacy bitmaps and prebuilt function bytes share storage; neither mode reads
  // the other view. Encode functions in the main loop, outside the timed ISR.
  struct Bank {
    union {
      uint8_t packets[CAPACITY * 2][7];
      uint8_t functionPackets[CAPACITY][DccFunctions::GROUPS][4];
    };
    uint8_t sizes[CAPACITY * 2], addresses[CAPACITY], count, mask, slots, first, last, onLow,
        onHigh;
    bool functionMode;
    uint16_t signature;
  };
  Bank banks[2] = {};
  volatile uint8_t active = 0, pending = 255;
  volatile uint32_t packets = 0;
  static uint16_t signature(uint8_t count, const uint8_t* addresses, uint8_t mask);

  // Call commit() with interrupts disabled after prepare() succeeds.
  bool prepare(uint8_t count, const uint8_t* addresses, uint8_t mask);
  static uint16_t controlSignature(uint8_t count, const uint8_t* addresses, uint8_t mask,
                                   uint8_t directions, const uint8_t* speeds);

  bool prepareControl(uint8_t count, const uint8_t* addresses, uint8_t mask, uint8_t directions,
                      const uint8_t* speeds);
  static uint16_t functionSignature(uint8_t count, const uint8_t* addresses, uint8_t directions,
                                    const uint32_t* functions);
  bool prepareFunctions(uint8_t count, const uint8_t* addresses, uint8_t directions,
                        const uint32_t* functions);
  bool prepareFinder(uint8_t first, uint8_t last, uint8_t onLow, uint8_t onHigh);
  void startStopped() { startupPending = true; }
  void commit() { pending = active ^ 1; }
  bool atPacketStart() const { return position == 0 && !second; }
  uint16_t nextHalfTicks();

private:
  static bool packetBit(uint8_t position, uint8_t address, uint8_t instruction, uint8_t checksum);
  static bool stopBit(uint8_t position, uint8_t address);
  static void encode(uint8_t* out, uint8_t& size, const uint8_t* bytes, uint8_t count);
  uint8_t functionPacket[4] = {0}, functionBytes = 3;
  volatile bool startupPending = false;
  uint8_t startupRemaining = 0, position = 0, slot = 0;
  uint16_t duration = ONE;
  bool second = false;
};
