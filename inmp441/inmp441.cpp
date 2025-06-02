#include "inmp441.h"

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "inmp441.pio.h"
#include <math.h>

#define SIGNAL_DIVISOR (256*256)

INMP441::INMP441(uint8_t pin_start, uint32_t sample_rate) {
  bool success = pio_claim_free_sm_and_add_program_for_gpio_range(
    &inmp441_program,
    &this->pio.pio,
    &this->pio.sm,
    &this->pio.offset,
    pin_start,
    3,
    true
  );
  hard_assert(success);
  inmp441_program_init(
    this->pio.pio,
    this->pio.sm,
    this->pio.offset,
    pin_start,
    sample_rate
  );
	// this->dma.ringbuf = (int32_t*)calloc(INMP441_BUF_SIZE, sizeof(int32_t));
	this->dma.ringbuf = (int32_t*)aligned_alloc(INMP441_BUF_SIZE, INMP441_BUF_SIZE * sizeof(int32_t));
	for (size_t i = 0; i < INMP441_BUF_SIZE; i += 1) {
		this->dma.ringbuf[i] = 0;
	}
	this->dma.channel = dma_claim_unused_channel(true);
	this->dma.config = dma_channel_get_default_config(this->dma.channel);
	channel_config_set_read_increment(&this->dma.config, false);
	channel_config_set_write_increment(&this->dma.config, true);
	channel_config_set_ring(&this->dma.config, true, log2(INMP441_BUF_SIZE));
	channel_config_set_dreq(&this->dma.config, pio_get_dreq(this->pio.pio, this->pio.sm, false));
	#if PICO_RP2040
	// RP2040-specific code (using 2-channel chained dma)
	int dma_chain = dma_claim_unused_channel(true);
	channel_config_set_chain_to(
		&this->dma.config,
		dma_chain
	);

	dma_channel_configure(
		this->dma.channel,
		&this->dma.config,
		this->dma.ringbuf,
		&this->pio.pio->rxf[this->pio.sm],
		INMP441_BUF_SIZE,
		true
	);

	dma_channel_config chain_config = dma_channel_get_default_config(dma_chain);
	channel_config_set_transfer_data_size(&chain_config, DMA_SIZE_32);
	channel_config_set_read_increment(&chain_config, false);
  channel_config_set_write_increment(&chain_config, false);
	channel_config_set_chain_to(&chain_config, this->dma.channel);
	dma_channel_configure(
		dma_chain,
		&chain_config,
		&(dma_channel_hw_addr(this->dma.channel)->write_addr),
		&this->dma.ringbuf,
		1,
		true
	);
	#else
	// rp2350+ code (single-channel dma chaining)
	dma_channel_configure(
		this->dma.channel,
		&this->dma.config,
		this->dma.ringbuf,
		&this->pio.pio->rxf[this->pio.sm],
		(0x1 << 28) | INMP441_BUF_SIZE,
		true
	);
	#endif
	this->dma.read_head = 0;
}

INMP441::~INMP441() {
	dma_channel_cleanup(this->dma.channel);
	free(this->dma.ringbuf);
  pio_remove_program_and_unclaim_sm(
    &inmp441_program,
    this->pio.pio,
    this->pio.sm,
    this->pio.offset
  );
}

size_t INMP441::get_write_head() {
	dma_channel_hw_addr(this->dma.channel)->transfer_count;
	return dma_channel_hw_addr(this->dma.channel)->write_addr - (size_t)this->dma.ringbuf;
}

void INMP441::read_audio_left(int32_t* buf, size_t len) {
	size_t buf_pos = this->dma.read_head;
	if (buf_pos & 0b1 == 1) buf_pos += 1;
  for (size_t i = 0; i < len; i += 1) {
		buf[i] = this->dma.ringbuf[(buf_pos+i*2) % INMP441_BUF_SIZE] / SIGNAL_DIVISOR;
  }
	this->dma.read_head = (this->dma.read_head + (len*2)) % INMP441_BUF_SIZE;
  // printf("\n");
}

void INMP441::read_audio_left(float* buf, size_t len) {
  size_t buf_pos = this->dma.read_head;
	if (buf_pos & 0b1 == 1) buf_pos += 1;
  for (size_t i = 0; i < len; i += 1) {
		buf[i] = this->dma.ringbuf[(buf_pos+i*2) % INMP441_BUF_SIZE] / SIGNAL_DIVISOR;
  }
	this->dma.read_head = (this->dma.read_head + (len*2)) % INMP441_BUF_SIZE;
  // printf("\n");
}

void INMP441::read_audio_right(int32_t* buf, size_t len) {
  for (size_t i = 0; i < len; i += 1) {
    // read and discard left channel word
    pio_sm_get_blocking(
      this->pio.pio,
      this->pio.sm
    );
    // read and store right channel word
    buf[i] = (int32_t)pio_sm_get_blocking(
      this->pio.pio,
      this->pio.sm
    );
  }
}

void INMP441::read_audio_right(float* buf, size_t len) {
  for (size_t i = 0; i < len; i += 1) {
    // read and discard left channel word
    pio_sm_get_blocking(
      this->pio.pio,
      this->pio.sm
    );
    // read and store right channel word
    buf[i] = pio_sm_get_blocking(
      this->pio.pio,
      this->pio.sm
    );
  }
}

void INMP441::read_audio_interleaved(int32_t* buf, size_t len) {
  for (size_t i = 0; i < len*2; i += 2) {
    // read and store left channel word
    buf[i] = ((int32_t)pio_sm_get_blocking(
      this->pio.pio,
      this->pio.sm
    )) / 256;
    // read and store right channel word
    buf[i+1] = ((int32_t)pio_sm_get_blocking(
      this->pio.pio,
      this->pio.sm
    )) / 256;
  }
}

void INMP441::read_audio_interleaved(float* buf, size_t len) {
  for (size_t i = 0; i < len*2; i += 2) {
    // read and store left channel word
    buf[i] = pio_sm_get_blocking(
      this->pio.pio,
      this->pio.sm
    );
    // read and store right channel word
    buf[i+1] = pio_sm_get_blocking(
      this->pio.pio,
      this->pio.sm
    );
  }
}
