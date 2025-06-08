#include "audio_analyzer.h"

int32_t noise_gate[10] = {34, 26, 24, 12, 7, 4, 3, 2, 2, 2};

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
  sem_init(&(this->input_lock), 1, 2);
  sem_init(&(this->output_lock), 1, 2);
  this->input_chunk.read = true;
  this->fft = new ArduinoFFT<double>(
    this->input_chunk.audio_data,
    this->input_chunk.imaginary_data,
    AUDIO_SAMPLE_COUNT,
    sample_frequency
  );
  this->upper_freq = sample_frequency / 4.0;
  this->freq_mult = pow(this->upper_freq / this->lower_freq, 1.0 / (AUDIO_OUTPUT_BUCKETS+1));
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

void audio_analyzer::write_audio_input(int32_t* new_data) {
  // printf("audio_analyzer::write_audio_input: semcount %d\n", sem_available(&this->input_lock));
  // if (! sem_available(&this->input_lock)) return;
  sem_acquire_blocking(&(this->input_lock));
  for (size_t i = 0; i < AUDIO_SAMPLE_COUNT; i += 1) {
    this->input_chunk.audio_data[i] = new_data[i];
  }
  this->input_chunk.read = false;
  sem_release(&(this->input_lock));
}

void audio_analyzer::write_audio_input(audio_writethrough func) {
	sem_acquire_blocking(&(this->input_lock));
	func(this->input_chunk.audio_data, AUDIO_SAMPLE_COUNT);
	this->input_chunk.read = false;
	sem_release(&(this->input_lock));
}

bool audio_analyzer::is_ready() {
  return !this->input_chunk.read && sem_available(&this->output_lock) && sem_available(&this->input_lock);
}

void audio_analyzer::analyze_audio() {
  sem_acquire_blocking(&(this->output_lock));
  sem_acquire_blocking(&(this->input_lock));

  this->input_chunk.read = true;
  this->fft->windowing(FFTWindow::Hamming, FFTDirection::Forward);
  this->fft->compute(FFTDirection::Forward);
  this->fft->complexToMagnitude();

  // minor post processing of frequency data
	double* a_data = this->input_chunk.audio_data;
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

		// printf("bucket %d | sum: %f | ", x, sum);

    // further post processing of bucket data
    // sum = sum - noise_gate[x];
		sum = pow(sum, 1.0/3.0) / 1.5;
    this->level_data[x] = sum;
		// printf("level: %f\n", sum);

		lower = upper;
    x += 1;
  }
  sem_release(&(this->output_lock));

	// clear out imaginary data for next pass
	for (size_t i = 0; i < AUDIO_SAMPLE_COUNT; i += 1) {
		this->input_chunk.imaginary_data[i] = 0;
	}
	sem_release(&(this->input_lock));

  return;
}

void audio_analyzer::read_audio_output(float* output) {
	for (size_t i = 0; i < AUDIO_OUTPUT_BUCKETS; i += 1) {
		output[i] = this->level_data[i];
	}
}