/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ws2812.h"

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"

PIO pio;
uint sm;
uint offset;

uint32_t pixels[NUM_PIXELS];

static inline void put_pixel(uint32_t pixel_grb) { pio_sm_put_blocking(pio, sm, pixel_grb << 8u); }

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

static inline uint32_t urgbw_u32(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
	return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | ((uint32_t)(w) << 24) | (uint32_t)(b);
}

// Set the color of a single LED
void ws2812_set_led(uint16_t index, ws2812_color_t color) {
  pixels[index] = urgb_u32(color.red, color.green, color.blue);
}

// Update the LED strip to display the set colors
void ws2812_show() {
  for (uint16_t i = 0; i < NUM_PIXELS; i += 1) {
    put_pixel(pixels[i]);
  }
}

// Clear all LEDs (set them to off)
void ws2812_clear() {
  for (uint16_t i = 0; i < NUM_PIXELS; i += 1) {
    pixels[i] = 0;
  }
}

void pattern_snakes(uint len, uint t) {
	for (uint i = 0; i < len; ++i) {
		uint x = (i + (t >> 1)) % 64;
		if (x < 10)
			put_pixel(urgb_u32(0xff, 0, 0));
		else if (x >= 15 && x < 25)
			put_pixel(urgb_u32(0, 0xff, 0));
		else if (x >= 30 && x < 40)
			put_pixel(urgb_u32(0, 0, 0xff));
		else
			put_pixel(0);
	}
}

void pattern_random(uint len, uint t) {
	if (t % 8) return;
	for (uint i = 0; i < len; ++i) put_pixel(rand());
}

void pattern_sparkle(uint len, uint t) {
	if (t % 8) return;
	for (uint i = 0; i < len; ++i) put_pixel(rand() % 16 ? 0 : 0xffffffff);
}

void pattern_greys(uint len, uint t) {
	uint max = 100;  // let's not draw too much current!
	t %= max;
	for (uint i = 0; i < len; ++i) {
		put_pixel(t * 0x10101);
		if (++t >= max) t = 0;
	}
}

const struct {
	pattern pat;
	const char* name;
} pattern_table[] = {
		{pattern_snakes, "Snakes!"},
		{pattern_random, "Random data"},
		{pattern_sparkle, "Sparkles"},
		{pattern_greys, "Greys"},
};

// Initialize the WS2812 LED strip
void ws2812_init() {
	bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
	hard_assert(success);
	ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
}

void ws2812_test() {
  printf("WS2812 Smoke Test, using pin %d\n", WS2812_PIN);

  int t = 0;
	while (1) {
		int pat = rand() % count_of(pattern_table);
		int dir = (rand() >> 30) & 1 ? 1 : -1;
		puts(pattern_table[pat].name);
		puts(dir == 1 ? "(forward)" : "(backward)");
		for (int i = 0; i < 1000; ++i) {
			pattern_table[pat].pat(NUM_PIXELS, t);
			sleep_ms(10);
			t += dir;
		}
	}
}

int main_old() {
	// set_sys_clock_48();
	stdio_init_all();

	ws2812_init();

  ws2812_test();

	// This will free resources and unload our program
	pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);

  return 0;
}
