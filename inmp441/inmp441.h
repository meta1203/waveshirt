#pragma once
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "stdint.h"

class INMP441 {
private:
  struct {
    PIO pio;
    uint sm;
    uint offset;
  } pio;

public:
  INMP441(uint8_t pin_start, uint32_t sample_rate);
  ~INMP441();
  void read_audio_left(uint32_t* buf, size_t len);
  void read_audio_left(float* buf, size_t len);
  void read_audio_right(uint32_t* buf, size_t len);
  void read_audio_right(float* buf, size_t len);
  void read_audio_interleaved(uint32_t* buf, size_t len);
  void read_audio_interleaved(float* buf, size_t len);
};

