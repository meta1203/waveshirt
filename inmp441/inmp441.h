#pragma once
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pico/stdlib.h"
#include "stdint.h"

static const uint16_t i2s_read32_instructions[] = {
					//     .wrap_target
	0xe03e, //  0: set    x, 30           side 0b00
	0x4801, //  1: in     pins, 1         side 0b01
	0x0041, //  2: jmp    x--, 1          side 0b00
	0x5801, //  3: in     pins, 1         side 0b11
	0xf03e, //  4: set    x, 30           side 0b10
	0x5801, //  5: in     pins, 1         side 0b11
	0x1045, //  6: jmp    x--, 5          side 0b10
	0x4801, //  7: in     pins, 1         side 0b01
					//     .wrap
};

static const pio_program_t pio_i2s_read32 = {
	.instructions = i2s_read32_instructions,
	.length = sizeof(i2s_read32_instructions) / sizeof(uint16_t),
	.origin = -1,
};

class INMP441 {
private:
  struct {
    PIO pio;
    uint sm;
    uint offset;
		uint pin_start;
		uint sample_rate;
  } pio;

	struct {
		dma_channel_config config;
		uint8_t channel;
		void* write_pos;
		int32_t* ringbuf;
		size_t read_head;
		#if PICO_RP2040
		uint8_t chain_channel;
		#endif
	} dma;

	size_t buffered_sample_count;

	void init_pio(uint8_t install_to = 0xff);
	void init_dma();
	size_t get_write_head();

public:
  INMP441(
		uint8_t pin_start,
		uint32_t sample_rate,
		size_t buffered_sample_count = 8192,
		uint8_t target_pio = 0xff
	);
  ~INMP441();
  void read_audio_left(int32_t* buf, size_t len);
  void read_audio_left(float* buf, size_t len);
  void read_audio_right(int32_t* buf, size_t len);
  void read_audio_right(float* buf, size_t len);
  void read_audio_interleaved(int32_t* buf, size_t len);
  void read_audio_interleaved(float* buf, size_t len);
};

