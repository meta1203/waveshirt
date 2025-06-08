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
INMP441<double>* mic;

uint8_t final_divisor = 10;

void io_thread();
void write_levels(float* levels);

int main() {
	stdio_init_all();

  ws2812_init();
  aa = new audio_analyzer(SAMPLING_FREQUENCY);

  multicore_launch_core1(io_thread);

  printf("core 0: beginning fft analysis...\n");
	while (true) {
    // compute fft
    while (!aa->is_ready()) tight_loop_contents();
    aa->analyze_audio();
	}
}

// handle audio in and ws2812 out
void io_thread() {
	mic = new INMP441<double>(2, SAMPLING_FREQUENCY, AUDIO_SAMPLE_COUNT);
  float* bucket_levels = (float*)malloc(AUDIO_SAMPLE_COUNT * sizeof(float));

  sleep_ms(20);
  printf("core 1: beginning data i/o...\n");

	uint16_t sample_counter = 0;

  while (true) {
    // read audio
		aa->write_audio_input([](double* buff, size_t len) {
			mic->read_audio_left(buff, len);
		});
		sample_counter += 1;

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
		float level = levels[bucket] / final_divisor;
		#if DEBUG
		printf("bucket %u: %lf\n", bucket, level);
		#endif
		level = MAX(0, MIN(100.0, level));
    for (uint32_t height = 0; height < max_height; height += 1) {
      ws2812_color_t color = {0x0, 0x0, 0x0};
      if (height <= level) {
        if (height+1 >= level) color = {0xff, 0xff, 0xff}; // white (top)
        else if (height < max_height*0.6) color = {0x00, 0xff, 0x00}; // green
        else if (height < max_height*0.85) color = {0xff, 0xff, 0x00}; // yellow
        else color = {0xff, 0x00, 0x00}; // red
      }
      ws2812_set_led(bucket*max_height + height, color);
    }
  }
}