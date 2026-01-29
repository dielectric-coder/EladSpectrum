#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdint.h>
#include <stdatomic.h>

#define FFT_SIZE 4096
#define WATERFALL_LINES 256
#define USB_BUFFER_SIZE (512 * 24)
#define DEFAULT_SAMPLE_RATE 192000

// IQ sample from FDM-DUO (24-bit samples packed as 3 bytes each)
typedef struct {
    float i;
    float q;
} iq_sample_t;

// Double buffer for thread-safe FFT data exchange
typedef struct {
    float spectrum_db[FFT_SIZE];
    atomic_int ready;
} spectrum_buffer_t;

// Application state shared between threads
typedef struct {
    // USB device state
    atomic_int device_connected;
    atomic_int streaming;

    // Double buffer: FFT writes to one, display reads from other
    spectrum_buffer_t buffers[2];
    atomic_int write_buffer;  // Index of buffer FFT is writing to

    // Display parameters
    float min_db;
    float max_db;
    int center_freq_hz;
    int sample_rate;

    // Waterfall history
    uint8_t waterfall_data[WATERFALL_LINES][FFT_SIZE];
    int waterfall_line;
} app_state_t;

#endif // APP_STATE_H
