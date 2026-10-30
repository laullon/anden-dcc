#include "DccSignal.h"
#include "dcc_tx.pio.h"
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/pio.h>
#include <hardware/sync.h>

namespace DccSignal {
namespace {
PIO engine = nullptr;
int machine = -1, dataChannel = -1, reloadChannel = -1;
uint offset = 0;
bool active = false;
// The DMA read ring must be aligned to its 512-byte size. Aligned word stores
// let updates replace complete packets without tearing a packet mid-frame.
alignas(512) volatile uint32_t packets[AddressFinder::WORDS] = {};
uint32_t ringTransfers = AddressFinder::WORDS;

void start() {
  pio_sm_set_enabled(engine, machine, false);
  pio_sm_config config = dcc_tx_program_get_default_config(offset);
  sm_config_set_set_pins(&config, PIN, 1);
  sm_config_set_out_shift(&config, false, false, 32);
  sm_config_set_clkdiv(&config, float(clock_get_hz(clk_sys)) / 500000.0f);
  pio_sm_init(engine, machine, offset, &config);
  pio_sm_set_pins_with_mask(engine, machine, 0, 1u << PIN);
  pio_sm_set_consecutive_pindirs(engine, machine, PIN, 1, true);

  // A streams 128 words, wrapping its read pointer. B reloads A's transfer
  // count and triggers it again. Both run without CPU/HTTP/flash callbacks.
  auto data = dma_channel_get_default_config(dataChannel);
  channel_config_set_transfer_data_size(&data, DMA_SIZE_32);
  channel_config_set_read_increment(&data, true);
  channel_config_set_write_increment(&data, false);
  channel_config_set_ring(&data, false, 9);
  channel_config_set_dreq(&data, pio_get_dreq(engine, machine, true));
  channel_config_set_chain_to(&data, reloadChannel);
  auto reload = dma_channel_get_default_config(reloadChannel);
  channel_config_set_transfer_data_size(&reload, DMA_SIZE_32);
  channel_config_set_read_increment(&reload, false);
  channel_config_set_write_increment(&reload, false);
  dma_channel_configure(reloadChannel, &reload,
      &dma_hw->ch[dataChannel].al1_transfer_count_trig, &ringTransfers, 1, false);
  dma_channel_configure(dataChannel, &data, &engine->txf[machine], packets,
      ringTransfers, true);
  pio_sm_set_enabled(engine, machine, true);
  active = true;
}
}

bool begin(const LocoRoster& roster) {
  if (ready()) return true;
  dataChannel = dma_claim_unused_channel(false);
  reloadChannel = dma_claim_unused_channel(false);
  if (dataChannel < 0 || reloadChannel < 0) {
    if (dataChannel >= 0) dma_channel_unclaim(dataChannel);
    if (reloadChannel >= 0) dma_channel_unclaim(reloadChannel);
    return false;
  }
  PIO candidates[] = {pio1, pio0};
  for (PIO candidate : candidates) {
    if (!pio_can_add_program(candidate, &dcc_tx_program)) continue;
    int freeMachine = pio_claim_unused_sm(candidate, false);
    if (freeMachine < 0) continue;
    engine = candidate;
    machine = freeMachine;
    offset = pio_add_program(engine, &dcc_tx_program);
    pio_gpio_init(engine, PIN);
    return setRoster(roster, true);
  }
  dma_channel_unclaim(dataChannel);
  dma_channel_unclaim(reloadChannel);
  return false;
}

bool setRoster(const LocoRoster& roster, bool resume) {
  if (!ready()) return false;
  uint32_t words[LocoRoster::CAPACITY];
  roster.schedule(words);
  for (unsigned i = 0; i < AddressFinder::WORDS; ++i) packets[i] = words[i % LocoRoster::CAPACITY];
  __dmb();
  if (!active && resume) start();
  return true;
}

bool setFinder(const AddressFinder& finder) {
  if (!ready()) return false;
  uint32_t words[AddressFinder::WORDS];
  finder.schedule(words);
  for (unsigned i=0; i<AddressFinder::WORDS; ++i) packets[i]=words[i];
  __dmb();
  if (!active) start();
  return true;
}

void stop() {
  if (!ready()) return;
  // Disable both before aborting so neither chain can restart the other.
  pio_sm_set_enabled(engine, machine, false);
  hw_clear_bits(&dma_hw->ch[dataChannel].ctrl_trig, DMA_CH0_CTRL_TRIG_EN_BITS);
  hw_clear_bits(&dma_hw->ch[reloadChannel].ctrl_trig, DMA_CH0_CTRL_TRIG_EN_BITS);
  dma_channel_abort(dataChannel);
  dma_channel_abort(reloadChannel);
  pio_sm_clear_fifos(engine, machine);
  pio_sm_set_pins_with_mask(engine, machine, 0, 1u << PIN);
  active = false;
}

bool ready() { return engine != nullptr; }
bool running() { return active; }
}
