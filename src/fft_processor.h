#ifndef FFT_PROCESSOR_H
#define FFT_PROCESSOR_H

#include "app_state.h"
#include <stdbool.h>

typedef struct fft_processor fft_processor_t;

// Create FFT processor with given FFT size
fft_processor_t *fft_processor_new(int fft_size);

// Free FFT processor
void fft_processor_free(fft_processor_t *fft);

// Process raw USB data (24-bit IQ samples) and compute FFT
// Returns true if a new spectrum is ready
bool fft_processor_process(fft_processor_t *fft, const uint8_t *usb_data, int length);

// Get the computed spectrum in dB (call after process returns true)
// Output array must be at least fft_size elements
void fft_processor_get_spectrum_db(fft_processor_t *fft, float *output);

// Get FFT size
int fft_processor_get_size(fft_processor_t *fft);

// Get RSSI (peak power in center passband) in dB
float fft_processor_get_rssi(fft_processor_t *fft);

#endif // FFT_PROCESSOR_H
