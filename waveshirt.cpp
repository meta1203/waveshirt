#define DEBUG 1

#include <stdio.h>

#include "hardware/pio.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/audio_i2s.h"

#include "ws2812.h"
#include "inmp441/inmp441.h"
#include "audio_analyzer.h"

#define SAMPLING_FREQUENCY 48000

audio_analyzer* aa;
INMP441* mic;

void io_thread();
void write_levels(float* levels);

int main() {
	stdio_init_all();

  ws2812_init();
  printf("sleeping for 2 seconds...\n");
  // sleep_ms(2000);
  printf("awoken from sleep!\n");

  aa = new audio_analyzer(SAMPLING_FREQUENCY);

  multicore_launch_core1(io_thread);
  // multicore_fifo_pop_blocking();
  // ws2812_test();

  printf("core 0: beginning fft analysis...\n");
	while (true) {
    // compute fft
    while (!aa->is_ready()) tight_loop_contents();
    aa->analyze_audio();
	}
}

// handle audio in and ws2812 out
void io_thread() {
	mic = new INMP441(3, SAMPLING_FREQUENCY);
  float* audio_data = (float*)malloc(AUDIO_SAMPLE_COUNT * sizeof(float));
  float* bucket_levels = (float*)malloc(AUDIO_SAMPLE_COUNT * sizeof(float));

  // sleep_ms(20);
  printf("core 1: beginning data i/o...\n");

  while (true) {
    // read audio
    mic->read_audio_left(audio_data, AUDIO_SAMPLE_COUNT);
    aa->write_audio_input(audio_data);

    // read bucket values from fft processor
    aa->read_audio_output(bucket_levels);

    // write buckets to LEDs
    write_levels(bucket_levels);
    ws2812_show();
  }
}

const uint32_t max_height = NUM_PIXELS / AUDIO_OUTPUT_BUCKETS;
void write_levels(float* levels) {
  for (uint16_t bucket = 0; bucket < AUDIO_OUTPUT_BUCKETS; bucket += 1) {
		// printf("bucket %u: %lf\n", bucket, levels[bucket]);
    float level = levels[bucket];
    for (uint32_t height = 0; height < max_height; height += 1) {
      ws2812_color_t color = {0x0, 0x0, 0x0};
      if (height <= level) {
        if (height+1 >= level) color = {0xff, 0xff, 0xff}; // white (top)
        else if (height < max_height*0.6) color = {0x00, 0xff, 0x00}; // green
        else if (height < max_height*0.90) color = {0xff, 0xff, 0x00}; // yellow 
        else color = {0xff, 0x00, 0x00}; // red
      }
      ws2812_set_led(bucket*max_height + height, color);
    }
  }
}