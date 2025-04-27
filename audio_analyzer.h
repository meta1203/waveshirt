#pragma once
#include "pico/sem.h"
#include "arduinoFFT/arduinoFFT.h"

#define AUDIO_SAMPLE_COUNT 4096
#define AUDIO_OUTPUT_BUCKETS 10

class audio_analyzer {
private:
  double sample_frequency;

  double lower_freq = 180.0;
  double upper_freq;
  double freq_mult;
  double upper_bucket;
  

  struct {
    double* audio_data;
    double* imaginary_data;
    bool read;
  } input_chunk;

  semaphore_t input_lock;
  double* level_data;
  semaphore_t output_lock;

  ArduinoFFT<double> fft;
public:
  audio_analyzer(size_t sample_frequency);
  ~audio_analyzer();
  void write_audio_input(float* new_data);
  void write_audio_input(uint32_t* new_data);
  bool is_ready();
  void analyze_audio();
  void read_audio_output(float* output);
};
