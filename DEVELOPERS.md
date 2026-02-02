# Developer Documentation

Comprehensive technical documentation for the Elad Spectrum Analyzer codebase.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Thread Model](#thread-model)
3. [Data Flow](#data-flow)
4. [Module Reference](#module-reference)
5. [Critical Sections](#critical-sections)
6. [Design Patterns](#design-patterns)
7. [Key Algorithms](#key-algorithms)
8. [Memory Management](#memory-management)
9. [Error Handling](#error-handling)

---

## Architecture Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         GTK4 Main Thread                            │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────┐  │
│  │  Spectrum   │  │  Waterfall  │  │   Control   │  │  Rotary   │  │
│  │   Widget    │  │   Widget    │  │     Bar     │  │  Encoder  │  │
│  └──────▲──────┘  └──────▲──────┘  └─────────────┘  └───────────┘  │
│         │                │                                          │
│         └────────┬───────┘                                          │
│                  │ spectrum_db[4096]                                │
│         ┌────────▼────────┐                                         │
│         │   app_data_t    │◄─────── CAT Control (serial polling)    │
│         │  (shared state) │                                         │
│         └────────▲────────┘                                         │
└──────────────────│──────────────────────────────────────────────────┘
                   │ GMutex protected
┌──────────────────│──────────────────────────────────────────────────┐
│                  │              USB Thread                          │
│         ┌────────┴────────┐                                         │
│         │  FFT Processor  │◄─────── USB async bulk transfers        │
│         │   (FFTW3)       │         from endpoint 0x86              │
│         └─────────────────┘                                         │
└─────────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility |
|-----------|---------------|
| `main.c` | Application lifecycle, GTK setup, thread coordination |
| `usb_device.c` | FDM-DUO USB protocol, async data streaming |
| `fft_processor.c` | IQ sample processing, FFT computation, spectrum averaging |
| `spectrum_widget.c` | Real-time spectrum display with Cairo |
| `waterfall_widget.c` | Scrolling waterfall display with direct pixel manipulation |
| `cat_control.c` | Serial CAT protocol for frequency/mode/filter queries |
| `rotary_encoder.c` | GPIO-based rotary encoder input (Raspberry Pi) |
| `settings.c` | Configuration persistence to INI file |

---

## Thread Model

### Thread Architecture

The application uses a **two-thread model**:

```
┌─────────────────────────────────────────┐
│            GTK Main Thread              │
│  - UI rendering (Cairo drawing)         │
│  - User input handling                  │
│  - CAT serial polling (every ~300ms)    │
│  - Display refresh timer (33ms / 30fps) │
│  - Settings auto-save timer             │
│  - Rotary encoder polling (5ms)         │
└─────────────────────────────────────────┘
                    │
                    │ GMutex (spectrum_mutex)
                    │ atomic operations
                    │
┌─────────────────────────────────────────┐
│              USB Thread                 │
│  - libusb event handling                │
│  - Async bulk transfer callbacks        │
│  - FFT processing                       │
│  - Device reconnection                  │
└─────────────────────────────────────────┘
```

### Thread Communication

| Mechanism | Purpose | Location |
|-----------|---------|----------|
| `GMutex spectrum_mutex` | Protects spectrum_db array during copy | `main.c:67` |
| `atomic_int spectrum_ready` | Signals new spectrum data available | `main.c:69` |
| `atomic_int running` | Signals thread shutdown | `main.c:63` |
| `atomic_int usb_connected` | USB connection status | `main.c:64` |

### Thread Lifecycle

```c
// Thread creation (main.c:755)
pthread_create(&app_data->usb_thread, NULL, usb_thread_func, app_data);

// Thread shutdown (main.c:499-503)
atomic_store(&app_data->running, 0);  // Signal stop
pthread_join(app_data->usb_thread, NULL);  // Wait for completion
```

---

## Data Flow

### USB Data Pipeline

```
FDM-DUO Hardware
      │
      │ USB Bulk Transfer (endpoint 0x86)
      │ 12288 bytes per transfer (1536 IQ samples)
      ▼
┌─────────────────┐
│ transfer_callback│  (usb_device.c:218)
│ (libusb async)   │
└────────┬────────┘
         │ Raw bytes
         ▼
┌─────────────────┐
│ usb_data_callback│  (main.c:90)
│                  │
└────────┬────────┘
         │ Raw bytes
         ▼
┌─────────────────┐
│ fft_processor_  │  (fft_processor.c:122)
│ process()       │
│  - Convert 32-bit IQ to float
│  - Apply Blackman-Harris window
│  - Execute FFTW
│  - FFT shift (DC to center)
│  - Convert to dB
│  - 3-frame averaging
└────────┬────────┘
         │ float spectrum_db[4096]
         ▼
┌─────────────────┐
│ Shared Buffer   │  (protected by GMutex)
│ app.spectrum_db │
└────────┬────────┘
         │ atomic_store(spectrum_ready, 1)
         ▼
┌─────────────────┐
│ refresh_display │  (main.c:153, GTK timer callback)
│                 │
│  - Copy spectrum under mutex
│  - Update spectrum widget
│  - Add waterfall line
└─────────────────┘
```

### Sample Format

```
USB Buffer Layout (12288 bytes):
┌────────────────────────────────────────────────────────────┐
│ Sample 0          │ Sample 1          │ ... │ Sample 1535 │
├───────┬───────────┼───────┬───────────┤     ├─────────────┤
│ I (4B)│ Q (4B)    │ I (4B)│ Q (4B)    │     │             │
│ int32 │ int32     │ int32 │ int32     │     │             │
│ LE    │ LE        │ LE    │ LE        │     │             │
└───────┴───────────┴───────┴───────────┴─────┴─────────────┘
```

### CAT Control Data Flow

```
Serial Port (/dev/ttyUSB0)
      │
      │ 38400 baud, 8N1
      ▼
┌─────────────────┐
│ cat_command()   │  (cat_control.c:105)
│  - Send: "IF;"  │
│  - Read response│
└────────┬────────┘
         │ "IF00014200000000000000000000000200;"
         ▼
┌─────────────────┐
│ Parse response  │  (cat_control.c:173-216)
│  - freq_hz      │  chars 2-12
│  - mode         │  char 29
│  - vfo          │  char 30
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Update UI       │  (main.c:219-251)
│  - Spectrum overlay
│  - Waterfall bandwidth lines
│  - VFO frame label
└─────────────────┘
```

---

## Module Reference

### usb_device.c

**Purpose**: Low-level USB communication with FDM-DUO hardware.

**Key Structures**:
```c
struct usb_device {
    libusb_context *ctx;
    libusb_device_handle *handle;
    struct libusb_transfer *transfers[2];  // Double-buffered async transfers
    uint8_t *transfer_buffers[2];
    usb_sample_callback_t callback;
    int streaming;
};
```

**Key Functions**:

| Function | Description |
|----------|-------------|
| `usb_device_open()` | Initialize device, read EEPROM, setup FIFO |
| `usb_device_start_streaming()` | Submit async bulk transfers |
| `usb_device_handle_events()` | Process libusb events (blocking) |
| `transfer_callback()` | Async callback, resubmits transfer |

**USB Control Transfers**:

| Request | wValue | wIndex | Description |
|---------|--------|--------|-------------|
| 0xFF | 0x0000 | 0x0000 | Read USB driver version |
| 0xA2 | 0x404C | 0x0151 | Read HW version |
| 0xA2 | 0x4000 | 0x0151 | Read serial number |
| 0xA2 | 0x4024 | 0x0151 | Read sample rate correction |
| 0xE1 | 0x0000 | 0xE900 | Stop FIFO |
| 0xE1 | 0x0001 | 0xE900 | Start FIFO |

### fft_processor.c

**Purpose**: Convert IQ samples to frequency-domain spectrum.

**Key Structures**:
```c
struct fft_processor {
    int fft_size;           // 4096
    double *fft_in;         // Interleaved I/Q input
    fftw_complex *fft_out;  // Complex output
    fftw_plan plan;         // FFTW execution plan
    double *window;         // Blackman-Harris coefficients
    float *spectrum_db;     // Output in dB
    float *spectrum_accum;  // Averaging accumulator
    int avg_count;          // Current average count (0-2)
};
```

**Processing Pipeline**:
1. Convert 32-bit signed int to float [-1.0, 1.0]
2. Apply Blackman-Harris window
3. Execute complex-to-complex FFT
4. FFT shift (move DC to center bin)
5. Calculate magnitude: `sqrt(re² + im²)`
6. Convert to dB: `20 * log10(mag)`
7. Accumulate for 3-frame averaging

### spectrum_widget.c

**Purpose**: Real-time spectrum display using GTK4/Cairo.

**Key Features**:
- GObject-based GTK widget
- Mutex-protected spectrum data
- Zoom/pan support (1x-16x)
- Frequency axis labels (adjusts to zoom)
- dB axis labels
- Tuned frequency marker (red line or arrow when off-screen)
- Mode/frequency overlay

**Drawing Order**:
1. Black background
2. Grid lines (gray)
3. dB labels (left margin)
4. Frequency labels (bottom margin)
5. Spectrum fill (cyan, 20% alpha)
6. Spectrum line (cyan)
7. Center frequency marker (red)
8. Overlay text (cyan on semi-transparent black)

### waterfall_widget.c

**Purpose**: Scrolling waterfall display with bandwidth indicators.

**Key Features**:
- Direct pixel manipulation (no Cairo for waterfall data)
- Cairo surface scrolling via `memmove()`
- Color gradient: black → blue → cyan → green → yellow → red
- Time axis labels
- Local/UTC time display
- Filter bandwidth lines (red/orange dashed)

**Rendering Technique**:
```c
// Direct pixel access for performance
unsigned char *data = cairo_image_surface_get_data(surface);
int stride = cairo_image_surface_get_stride(surface);

// Scroll down: move all rows down by 1
memmove(data + stride, data, stride * (height - 1));

// Write new line at row 0
uint32_t *row = (uint32_t *)data;
for (int x = 0; x < width; x++) {
    row[x] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}
```

### cat_control.c

**Purpose**: CAT (Computer Aided Transceiver) protocol over serial.

**Protocol**: Kenwood TS-480 compatible subset

**Commands Used**:

| Command | Response | Description |
|---------|----------|-------------|
| `IF;` | `IF...;` (38 chars) | Frequency, mode, VFO |
| `RF1;` | `RF1XX;` | LSB filter bandwidth |
| `RF2;` | `RF2XX;` | USB filter bandwidth |
| `RF3;` | `RF3XX;` | CW filter bandwidth |
| `RF4;` | `RF4XX;` | FM filter bandwidth |
| `RF5;` | `RF5XX;` | AM filter bandwidth |
| `RF7;` | `RF7XX;` | CW-R filter bandwidth |

**Filter Lookup Tables**:
- LSB/USB: 22 entries (1.6k - 6.0k, D300, D600, D1k)
- CW/CW-R: indices 7-16 (100&4 - 2.6k)
- AM: 8 entries (2.5k - 6.0k)
- FM: 3 entries (Narrow, Wide, Data)

### rotary_encoder.c

**Purpose**: GPIO-based rotary encoder input for Raspberry Pi.

**Hardware Interface**:
- Uses libgpiod for GPIO access
- Polling-based (5ms interval via GLib timer)
- Internal pull-up resistors enabled
- Button debounce: 200ms

**Rotation Detection**:
```c
// Detect rotation on CLK falling edge
if (clk_state != last_clk_state && clk_state == 0) {
    // Direction determined by DT state
    int direction = (dt_state != clk_state) ? 1 : -1;
    rotation_callback(direction, user_data);
}
```

### settings.c

**Purpose**: Persistent configuration storage.

**File Format**: Simple INI-style key=value
```ini
# EladSpectrum Settings
spectrum_ref=-30.0
spectrum_range=120.0
waterfall_ref=-30.0
waterfall_range=120.0
zoom_level=1
pan_offset=0
```

**Location**: `~/.config/elad-spectrum/settings.conf`

---

## Critical Sections

### 1. Spectrum Data Exchange

**Location**: `main.c:96-99` (write), `main.c:236-238` (read)

**Protected Resource**: `app.spectrum_db[4096]`

**Mutex**: `app.spectrum_mutex`

```c
// USB Thread (producer)
g_mutex_lock(&app_data->spectrum_mutex);
fft_processor_get_spectrum_db(app_data->fft, app_data->spectrum_db);
atomic_store(&app_data->spectrum_ready, 1);
g_mutex_unlock(&app_data->spectrum_mutex);

// GTK Thread (consumer)
if (atomic_exchange(&app_data->spectrum_ready, 0)) {
    g_mutex_lock(&app_data->spectrum_mutex);
    memcpy(spectrum_copy, app_data->spectrum_db, sizeof(spectrum_copy));
    g_mutex_unlock(&app_data->spectrum_mutex);
    // ... update widgets
}
```

### 2. Spectrum Widget Data

**Location**: `spectrum_widget.c:50,254` (draw), `spectrum_widget.c:294,305` (update)

**Protected Resource**: Widget's internal `spectrum_db` array

**Mutex**: `self->data_mutex`

### 3. Waterfall Widget Data

**Location**: `waterfall_widget.c:93,127` (draw), `waterfall_widget.c:282,339` (add_line)

**Protected Resource**: Cairo surface and zoom/pan state

**Mutex**: `self->data_mutex`

### 4. USB Transfer State

**Location**: `usb_device.c`

**Protected by**: Single-threaded access (USB thread only) + `streaming` flag

```c
// transfer_callback runs in USB thread context
if (dev->streaming) {
    libusb_submit_transfer(transfer);  // Resubmit
}
```

---

## Design Patterns

### 1. Double Buffering (USB Transfers)

```c
#define NUM_TRANSFERS 2
struct libusb_transfer *transfers[NUM_TRANSFERS];

// Both transfers active simultaneously
// When one completes, it's resubmitted while other is in-flight
```

### 2. Producer-Consumer (Spectrum Data)

- **Producer**: USB thread (via FFT processor)
- **Consumer**: GTK main thread (via refresh timer)
- **Buffer**: Single shared buffer with mutex
- **Signal**: Atomic flag `spectrum_ready`

### 3. Observer Pattern (GTK Adjustments)

```c
// Value changes propagate via callbacks
g_signal_connect(ref_adj, "value-changed", G_CALLBACK(on_spectrum_range_changed), app_data);
```

### 4. Opaque Pointer (Information Hiding)

All modules use opaque structs:
```c
// Header (public)
typedef struct usb_device usb_device_t;
usb_device_t *usb_device_new(void);

// Source (private)
struct usb_device {
    // Implementation details hidden
};
```

### 5. Callback Pattern (Event Handling)

```c
// USB data callback
typedef void (*usb_sample_callback_t)(const uint8_t *data, int length, void *user_data);

// Encoder rotation callback
typedef void (*encoder_rotation_callback_t)(int direction, void *user_data);
```

### 6. Debounced Auto-Save

```c
#define SETTINGS_SAVE_DELAY_MS 3000

static void schedule_settings_save(app_data_t *app_data) {
    if (app_data->save_timeout_id > 0) {
        g_source_remove(app_data->save_timeout_id);  // Cancel pending
    }
    app_data->save_timeout_id = g_timeout_add(SETTINGS_SAVE_DELAY_MS,
                                               save_settings_timeout, app_data);
}
```

---

## Key Algorithms

### 1. Blackman-Harris Window

**Location**: `fft_processor.c:33-44`

```c
// 4-term Blackman-Harris window for excellent sidelobe suppression
const double a0 = 0.35875, a1 = 0.48829, a2 = 0.14128, a3 = 0.01168;
for (int i = 0; i < size; i++) {
    double x = (double)i / (double)(size - 1);
    window[i] = a0 - a1*cos(2π*x) + a2*cos(4π*x) - a3*cos(6π*x);
}
```

### 2. FFT Shift

**Location**: `fft_processor.c:149-156`

```c
// Move DC (bin 0) to center of spectrum
int half = fft_size / 2;
for (int j = 0; j < fft_size; j++) {
    int src_idx = (j + half) % fft_size;
    // Use fft_out[src_idx] for bin j
}
```

### 3. FPGA Tuning Word Calculation

**Location**: `usb_device.c:173-194`

```c
// DDS tuning word for FPGA frequency synthesizer
long effective_rate = S_RATE + sample_rate_correction;  // 122.88 MHz ± correction
tuning_freq = freq_hz - (floor(freq_hz / effective_rate) * effective_rate);
tuning_word = (4294967296.0 * tuning_freq) / effective_rate;
```

### 4. Zoom/Pan Bin Calculation

**Location**: `spectrum_widget.c:125-132`

```c
int visible_bins = spectrum_size / zoom_level;
int max_pan = (spectrum_size - visible_bins) / 2;
int clamped_pan = clamp(pan_offset, -max_pan, max_pan);
int start_bin = (spectrum_size - visible_bins) / 2 + clamped_pan;
int end_bin = start_bin + visible_bins;
```

### 5. Color Gradient Mapping

**Location**: `waterfall_widget.c:39-76`

```c
// Piecewise linear gradient: black→blue→cyan→green→yellow→red
if (normalized < 0.2f) {
    // Black to blue
    *b = (uint8_t)(normalized / 0.2f * 255);
} else if (normalized < 0.4f) {
    // Blue to cyan
    *g = (uint8_t)((normalized - 0.2f) / 0.2f * 255);
    *b = 255;
}
// ... continued for other segments
```

### 6. Bandwidth Line Positioning

**Location**: `waterfall_widget.c:140-181`

```c
// Mode-aware positioning
int center_bin = spectrum_size / 2 + offset_bins;  // Apply data mode offset
int bw_bins = (bandwidth_hz * spectrum_size) / sample_rate;

switch (mode) {
    case USB:  line_bins[0] = center_bin + bw_bins; break;      // Upper edge only
    case LSB:  line_bins[0] = center_bin - bw_bins; break;      // Lower edge only
    default:   // Symmetric around center
        line_bins[0] = center_bin - bw_bins/2;
        line_bins[1] = center_bin + bw_bins/2;
}
```

---

## Memory Management

### Allocation Strategy

| Module | Allocation | Lifetime |
|--------|------------|----------|
| `usb_device` | `calloc()` | Application lifetime |
| `fft_processor` | `fftw_malloc()` for aligned data | Application lifetime |
| `spectrum_widget` | `g_malloc()` for spectrum copy | Widget lifetime |
| `waterfall_widget` | `cairo_image_surface_create()` | Widget lifetime |
| `settings` | Stack + `malloc()` for paths | Function scope |

### Resource Cleanup

All modules follow a consistent pattern:
```c
// Creation
xxx_t *xxx_new(void);

// Destruction
void xxx_free(xxx_t *xxx);
```

GTK widgets use GObject reference counting and `finalize()` methods.

---

## Error Handling

### Strategy

- **USB errors**: Log and attempt reconnection
- **Serial errors**: Log and continue (CAT is non-critical)
- **Memory errors**: Return NULL, caller checks
- **GPIO errors**: Log and disable encoder support

### Error Propagation

```c
// Return codes
int usb_device_open(usb_device_t *dev);  // 0 = success, -1 = error

// Null checks
if (!fft_processor_process(...)) {
    // Handle error or incomplete data
}
```

### Reconnection Logic

**Location**: `main.c:109-142`

```c
while (running) {
    if (!usb_device_is_open(usb)) {
        if (usb_device_open(usb) == 0) {
            usb_device_start_streaming(usb, callback, data);
        } else {
            usleep(1000000);  // Retry after 1 second
            continue;
        }
    }
    // ... handle events
}
```

---

## Build Configuration

### Meson Options

```meson
# Conditional libgpiod support
gpiod_dep = dependency('libgpiod', required: false)
if gpiod_dep.found()
    add_project_arguments('-DHAVE_GPIOD', language: 'c')
endif
```

### Compiler Flags

- `-Wall -Wextra` for warnings
- `-O2` for release optimization
- `-g` for debug symbols

---

## Testing Recommendations

### Unit Testing Candidates

1. `parse_bandwidth_hz()` - Pure function, easy to test
2. `db_to_color()` - Pure function for color mapping
3. `generate_window()` - Verify window coefficients
4. Filter lookup tables - Verify all indices map correctly

### Integration Testing

1. USB device detection and streaming
2. CAT command/response parsing
3. Settings save/load round-trip

### Manual Testing Checklist

- [ ] USB device connects and streams
- [ ] Spectrum updates at ~30 fps
- [ ] Waterfall scrolls smoothly
- [ ] CAT frequency/mode updates
- [ ] Zoom/pan works correctly
- [ ] Bandwidth lines appear for all modes
- [ ] Settings persist across restarts
- [ ] Rotary encoders respond (Pi mode)
