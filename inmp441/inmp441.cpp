#include "inmp441.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "inmp441.pio.h"

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
  uint32_t poll_rate = sample_rate * 2 * 32;
  printf("look: %f MHz\n", poll_rate / 1000000.0);
  printf("clk: %u Hz\n", clock_get_hz(clk_sys));
  inmp441_program_init(
    this->pio.pio,
    this->pio.sm,
    this->pio.offset,
    pin_start,
    poll_rate
  );
}

INMP441::~INMP441() {
  pio_remove_program_and_unclaim_sm(
    &inmp441_program,
    this->pio.pio,
    this->pio.sm,
    this->pio.offset
  );
}

void INMP441::read_audio_left(uint32_t* buf, size_t len) {
  for (size_t i = 0; i < len; i += 1) {
    // read and store left channel word
    buf[i] = pio_sm_get_blocking(
      this->pio.pio,
      this->pio.sm
    );
    // if (buf[i]) printf("%x ", buf[i]);
    // read and discard right channel word
    pio_sm_get_blocking(
      this->pio.pio,
      this->pio.sm
    );
  }
  // printf("\n");
}

void INMP441::read_audio_left(float* buf, size_t len) {
  for (size_t i = 0; i < len; i += 1) {
    // read and store left channel word
    buf[i] = (int32_t)(pio_sm_get_blocking(
      this->pio.pio,
      this->pio.sm
    ));
    // read and discard right channel word
    pio_sm_get_blocking(
      this->pio.pio,
      this->pio.sm
    );
  }
  // printf("\n");
}

void INMP441::read_audio_right(uint32_t* buf, size_t len) {
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

void INMP441::read_audio_interleaved(uint32_t* buf, size_t len) {
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
