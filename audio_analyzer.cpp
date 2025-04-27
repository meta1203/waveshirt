#include "audio_analyzer.h"

inline double sum_buckets(double* data, size_t from, size_t to) {
	double sum = 0;
	for (size_t i = from; i < to; i++) {
		sum += data[i];
	}
	return sum / (1 + to - from);
}

audio_analyzer::audio_analyzer(size_t sample_frequency) {
	this->input_chunk.audio_data = (double*)malloc(AUDIO_SAMPLE_COUNT * sizeof(double));
	this->input_chunk.imaginary_data = (double*)malloc(AUDIO_SAMPLE_COUNT * sizeof(double));
	this->level_data = (double*)malloc(AUDIO_OUTPUT_BUCKETS * sizeof(double));

  this->sample_frequency = sample_frequency;
  sem_init(&(this->input_lock), 1, 1);
  sem_init(&(this->output_lock), 1, 1);
  this->input_chunk.read = true;
  this->fft = ArduinoFFT<double>(
    this->input_chunk.audio_data,
    this->input_chunk.imaginary_data,
    AUDIO_SAMPLE_COUNT,
    sample_frequency
  );
  this->upper_freq = sample_frequency / 4.0;
  this->freq_mult = pow(this->upper_freq / this->lower_freq, 1.0 / AUDIO_OUTPUT_BUCKETS);
  this->upper_bucket = (AUDIO_SAMPLE_COUNT / 2.0) * this->upper_freq / sample_frequency;
}

audio_analyzer::~audio_analyzer() {}

void audio_analyzer::write_audio_input(float* new_data) {
  sem_acquire_blocking(&(this->input_lock));
  for (size_t i = 0; i < AUDIO_SAMPLE_COUNT; i += 1) {
    this->input_chunk.audio_data[i] = new_data[i];
  }
  this->input_chunk.read = false;
  sem_release(&(this->input_lock));
}

void audio_analyzer::write_audio_input(uint32_t* new_data) {
  sem_acquire_blocking(&(this->input_lock));
  for (size_t i = 0; i < AUDIO_SAMPLE_COUNT; i += 1) {
    this->input_chunk.audio_data[i] = new_data[i];
  }
  this->input_chunk.read = false;
  sem_release(&(this->input_lock));
}

bool audio_analyzer::is_ready() {
  return !this->input_chunk.read;
}

void audio_analyzer::analyze_audio() {
  sem_acquire_blocking(&(this->output_lock));
  sem_acquire_blocking(&(this->input_lock));
	double* a_data = this->input_chunk.audio_data;
	
  this->input_chunk.read = true;
  this->fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  this->fft.compute(FFTDirection::Forward);
  this->fft.complexToMagnitude();

  // minor post processing of frequency data
  for (size_t i = 0; i < AUDIO_SAMPLE_COUNT; i += 1) {
    a_data[i] = abs(a_data[i]);
  }

  double lower = this->lower_freq / (this->sample_frequency / AUDIO_SAMPLE_COUNT);
  double upper = 0;
  size_t x = 0;
  while (lower < this->upper_bucket) {
    upper = lower * this->freq_mult;
    // add frequency buckets together, combining fewer buckets at lower frequencies and more buckets at higher frequencies
    double sum = sum_buckets(a_data, (size_t)round(lower), size_t(round(upper)));
    lower = upper;

    // further post processing of bucket data
    // sum = sum - noise_gate[x];
		sum = pow(sum, 1.0/3.0)*2;

    this->level_data[x] = sum;
    x += 1;
  }
  sem_release(&(this->output_lock));
  sem_release(&(this->input_lock));
}

void audio_analyzer::read_audio_output(float* output) {
  if (sem_try_acquire(&(this->output_lock))) {
    for (size_t i = 0; i < AUDIO_OUTPUT_BUCKETS; i += 1) {
      output[i] = this->level_data[i];
    }
    sem_release(&(this->output_lock));
  }
}