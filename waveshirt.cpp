#define DEBUG 0

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
inline void print_bits_32(int32_t value);
inline void print_bits_double(double value);

int main() {
	stdio_init_all();

  ws2812_init();
  printf("sleeping for 2 seconds...\n");
  // sleep_ms(2000);
  printf("awoken from sleep!\n");

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
	mic = new INMP441(2, SAMPLING_FREQUENCY);
  int32_t* audio_data = (int32_t*)malloc(AUDIO_SAMPLE_COUNT * sizeof(int32_t));
  float* bucket_levels = (float*)malloc(AUDIO_SAMPLE_COUNT * sizeof(float));

  sleep_ms(20);
  printf("core 1: beginning data i/o...\n");

	uint16_t sample_counter = 0;

  while (true) {
    // read audio
    mic->read_audio_left(audio_data, AUDIO_SAMPLE_COUNT);
		sample_counter += 1;
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
		#if DEBUG
		printf("bucket %u: %lf\n", bucket, levels[bucket]);
		#endif
    float level = levels[bucket];
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

inline void print_bits_32(int32_t value) {
	for (int i = 31; i >= 0; i--) {
		printf("%c", (value & (1 << i)) ? '1' : '0');
	}
}

inline void print_bits_double(double value) {
	int64_t converted = *((int64_t*)&value);
	for (int i = sizeof(double); i >= 0; i--) {
		printf("%c", (converted & (1 << i)) ? '1' : '0');
	}
}