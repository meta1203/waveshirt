#pragma once
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pico/stdlib.h"
#include "stdint.h"

#define INMP441_BUF_SIZE 8192

class INMP441 {
private:
  struct {
    PIO pio;
    uint sm;
    uint offset;
  } pio;

	struct {
		dma_channel_config config;
		uint8_t channel;
		void* write_pos;
		int32_t* ringbuf;
		size_t read_head;
	} dma;

	size_t get_write_head();

public:
  INMP441(uint8_t pin_start, uint32_t sample_rate);
  ~INMP441();
  void read_audio_left(int32_t* buf, size_t len);
  void read_audio_left(float* buf, size_t len);
  void read_audio_right(int32_t* buf, size_t len);
  void read_audio_right(float* buf, size_t len);
  void read_audio_interleaved(int32_t* buf, size_t len);
  void read_audio_interleaved(float* buf, size_t len);
};

