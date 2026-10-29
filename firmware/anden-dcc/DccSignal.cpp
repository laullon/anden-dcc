#include "DccSignal.h"
#include "DccPacket.h"
#include "dcc_tx.pio.h"
#include <hardware/clocks.h>
#include <hardware/pio.h>

namespace DccSignal {
namespace {
PIO engine = nullptr;
int machine = -1;
uint offset = 0;
bool active = false;
uint8_t locomotive = 3;
bool lights = false;

void start(uint32_t packet) {
  pio_sm_set_enabled(engine, machine, false);
  pio_sm_config config = dcc_tx_program_get_default_config(offset);
  sm_config_set_set_pins(&config, PIN, 1);
  sm_config_set_out_shift(&config, false, false, 32); // MSB first, manual pull.
  sm_config_set_clkdiv(&config, float(clock_get_hz(clk_sys)) / 500000.0f);
  pio_sm_init(engine, machine, offset, &config);
  pio_sm_set_pins_with_mask(engine, machine, 0, 1u << PIN);
  pio_sm_set_consecutive_pindirs(engine, machine, PIN, 1, true);
  pio_sm_put(engine, machine, packet);
  pio_sm_set_enabled(engine, machine, true);
  active = true;
}
}

bool begin() {
  if (ready()) return true;
  // Wi-Fi has already initialized. Claim resources rather than assuming its PIO.
  PIO candidates[] = {pio1, pio0};
  for (PIO candidate : candidates) {
    if (!pio_can_add_program(candidate, &dcc_tx_program)) continue;
    int freeMachine = pio_claim_unused_sm(candidate, false);
    if (freeMachine < 0) continue;
    engine = candidate;
    machine = freeMachine;
    offset = pio_add_program(engine, &dcc_tx_program);
    pio_gpio_init(engine, PIN);
    start(DccPacket::word(locomotive, lights));
    return true;
  }
  return false;
}

bool setF0(uint8_t target, bool enabled) {
  if (!ready() || target < 1 || target > 127) return false;
  uint32_t packet = DccPacket::word(target, enabled);
  if (active) {
    if (pio_sm_is_tx_fifo_full(engine, machine)) return false;
    pio_sm_put(engine, machine, packet);
  } else start(packet);
  locomotive = target;
  lights = enabled;
  return true;
}

void stop() {
  if (!ready()) return;
  pio_sm_set_enabled(engine, machine, false);
  pio_sm_clear_fifos(engine, machine);
  pio_sm_set_pins_with_mask(engine, machine, 0, 1u << PIN);
  active = false;
}

bool ready() { return engine != nullptr; }
bool running() { return active; }
uint8_t address() { return locomotive; }
bool f0() { return lights; }
}
