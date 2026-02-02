# Elad Spectrum

Elad FDM-DUO Transceiver SDR Spectrum Analyzer application.

## Project Overview

This project provides a spectrum analyzer application for the Elad FDM-DUO Software Defined Radio transceiver. The FDM-DUO exposes three USB interfaces:

1. **Sound Card** - Audio I/O for transmit/receive audio
2. **Serial Port** - CAT (Computer Aided Transceiver) control commands
3. **Sampling Data Port** - Raw sampled RF data from the antenna (typically 192 KHz bandwidth)

## Technology Stack

- **Language**: C
- **GUI Framework**: GTK4
- **FFT Library**: FFTW3
- **USB Library**: libusb-1.0
- **GPIO Library**: libgpiod (optional, for Raspberry Pi rotary encoder)
- **Build System**: Meson

## Current Status

### Phase 1: Spectrum Display (Complete)
- USB device handling with async bulk transfers from endpoint 0x86
- 4096-point FFT with Blackman-Harris window using FFTW3
- 3-frame spectrum averaging for noise reduction
- Live spectrum analyzer with cyan trace and grid
- Axis labels outside plot area (dB on left, frequency on bottom)
- Waterfall display with direct pixel rendering and time axis (no scaling artifacts)
- Adjustable reference level (default -30 dB) and dynamic range (default 120 dB)
- Red tuned frequency marker line (follows zoom/pan, shows arrow when off-screen)

### Phase 2: CAT Control (Complete)
- Frequency, mode and filter bandwidth display overlay on spectrum (transparent background)
- VFO A/B indicator in spectrum frame label
- CAT polling via serial port (/dev/ttyUSB0 @ 38400 baud)
- Kenwood TS-480 compatible IF; command for frequency, mode and VFO
- RF command for filter bandwidth with mode-specific lookup tables
- Supported modes: AM, LSB, USB, CW, FM, CW-R
- Real-time sync with radio tuning, mode, VFO and filter changes

### Phase 3: Audio Interface (Planned)
- Interface with the USB sound card
- Handle audio routing for RX/TX

## Project Structure

```
EladSpectrum/
├── meson.build              # Build configuration
├── CLAUDE.md                # Project documentation
├── src/
│   ├── main.c               # GTK4 application entry point
│   ├── app_state.h          # Shared state structures and constants
│   ├── usb_device.c/h       # USB communication with FDM-DUO
│   ├── cat_control.c/h      # CAT serial control (frequency/mode polling)
│   ├── fft_processor.c/h    # FFT computation with FFTW3
│   ├── spectrum_widget.c/h  # Spectrum display GtkDrawingArea
│   ├── waterfall_widget.c/h # Waterfall display GtkDrawingArea
│   └── rotary_encoder.c/h   # GPIO rotary encoder (Pi only, optional)
└── examples/                # Reference implementations
    ├── main.c               # Direct USB communication example
    ├── elad-server.c        # Network server with UDP/TCP
    ├── serial-send-ui.c     # GTK3 serial terminal
    └── elad-gqrx.c          # Wrapper for gqrx SDR
```

## USB Protocol Details

- **Vendor ID**: 0x1721
- **Product ID**: 0x061a
- **Sampling Data Endpoint**: 0x86 (bulk transfer)
- **Sample Format**: 32-bit signed integer IQ pairs (8 bytes per sample)
- **Buffer Size**: 12288 bytes per transfer (1536 IQ samples)
- **Sample Rates**: 192, 384, 768, 1536, 3072, 6144 kS/s

## FFT Settings

- **FFT Size**: 4096 samples
- **Window**: Blackman-Harris
- **Frequency Resolution**: 46.9 Hz per bin (at 192 kHz sample rate)
- **Averaging**: 3 frames
- **Update Rate**: ~15.6 lines/second (192000 / 4096 / 3)

## Dual Rotary Encoders (Raspberry Pi)

Optional dual GPIO rotary encoder support for hands-free parameter adjustment in Pi mode.

### Hardware Setup

| Encoder | Pin | GPIO (BCM) | Function |
|---------|-----|------------|----------|
| 1 | CLK | 17 | Rotation clock (A) |
| 1 | DT  | 27 | Rotation data (B) |
| 1 | SW  | 22 | Push button |
| 2 | CLK | 5  | Rotation clock (A) |
| 2 | DT  | 6  | Rotation data (B) |
| 2 | SW  | 13 | Push button |

### Encoder 1 - Parameter Control

Controls display parameters with independent spectrum/waterfall settings.

- **Button press**: Cycles active parameter:
  - Spectrum Ref → Spectrum Range → Waterfall Ref → Waterfall Range → (repeat)
- **Rotation**: Adjusts active parameter
  - Ref levels: ±1 dB per detent
  - Range: ±5 dB per detent
- **Visual feedback**: Active parameter name displayed in cyan

### Encoder 2 - Zoom/Pan Control

Controls horizontal zoom and panning with mode toggle.

- **Button press**: Toggles between Zoom mode and Pan mode
- **Rotation in Zoom mode**: Changes zoom level
  - Clockwise: zoom in (1x → 2x → 4x)
  - Counter-clockwise: zoom out (4x → 2x → 1x)
  - Pan resets to center when zoom changes
- **Rotation in Pan mode**: Translates spectrum/waterfall horizontally
  - Clockwise: pan right, Counter-clockwise: pan left
  - Only effective when zoom > 1x
- **Visual feedback**: Mode and zoom level displayed in cyan ("Zoom 2x" or "Pan 2x")

### Zoom Levels

| Zoom | Frequency Span | Resolution |
|------|----------------|------------|
| 1x | 192 kHz (full) | 46.9 Hz/bin |
| 2x | 96 kHz (center) | 23.4 Hz/bin |
| 4x | 48 kHz (center) | 11.7 Hz/bin |

### Notes

- Only enabled when running in Pi mode (`-p` flag)
- Requires libgpiod library (auto-detected at build time)
- Builds without encoder support if libgpiod is not available
- Spectrum and waterfall have independent ref/range settings

## Dependencies

```bash
# Debian/Ubuntu
sudo apt install libgtk-4-dev libusb-1.0-0-dev libfftw3-dev meson ninja-build

# Raspberry Pi (optional, for rotary encoder support)
sudo apt install libgpiod-dev
```

## Build

```bash
meson setup build
meson compile -C build
```

## Run

```bash
# Normal windowed mode (1024x768)
./build/elad-spectrum

# Fullscreen mode
./build/elad-spectrum -f

# Raspberry Pi 5" LCD (800x480)
./build/elad-spectrum -p

# Pi fullscreen (recommended for embedded use)
./build/elad-spectrum -p -f

# Help
./build/elad-spectrum -h
```

## Command-line Options

| Option | Description |
|--------|-------------|
| `-f, --fullscreen` | Start in fullscreen mode |
| `-p, --pi` | Set window size to 800x480 (5" LCD), enable rotary encoder |
| `-h, --help` | Show help message |

## UI Controls

- **Status Indicator**: Colored circle showing connection state
  - Green (●): USB device connected and streaming
  - Gray (○): Disconnected or not streaming
  - Red (●): Error state
- **Spectrum Frame**: Shows current VFO (VFO A or VFO B)
- **Ref**: Spectrum reference level (top of spectrum display in dB)
- **Range**: Spectrum dynamic range (dB span from ref to bottom)
- **Overlay**: Frequency, mode and filter bandwidth displayed centered on spectrum (cyan text)
- **Frequency Marker**: Red vertical line at tuned frequency position (from CAT)
  - Follows zoom and pan movements
  - Shows red arrow at edge when tuned frequency is panned off-screen
- **Active Param**: Currently selected parameter for encoder 1 (Pi mode)
- **Zoom**: Current zoom level shown in cyan (Pi mode with encoder)

### Pi Mode UI

In Pi mode (`-p` flag), the control bar uses a single spinbutton that displays the value of the currently active parameter (controlled by encoder 1). This saves horizontal space on small displays. The spinbutton updates automatically when:
- The active parameter is changed via encoder button press
- The parameter value is adjusted via encoder rotation

## Notes

- USB interfaces may require root access or udev rules for user access
- Serial port (/dev/ttyUSB0) requires dialout group membership or root access
- The FDM-DUO FPGA must be initialized before data streaming begins
- Sample rate correction values are read from device EEPROM
- Application auto-reconnects if device is disconnected
