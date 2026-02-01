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

### Phase 2: CAT Control (Complete)
- Frequency and mode display overlay on spectrum (transparent background)
- VFO A/B indicator in spectrum frame label
- CAT polling via serial port (/dev/ttyUSB0 @ 38400 baud)
- Kenwood TS-480 compatible IF; command for frequency, mode and VFO
- Supported modes: AM, LSB, USB, CW, FM, CW-R
- Real-time sync with radio tuning, mode and VFO changes

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

## Rotary Encoder (Raspberry Pi)

Optional GPIO rotary encoder support for hands-free parameter adjustment in Pi mode.

### Hardware Setup

| Pin | GPIO (BCM) | Function |
|-----|------------|----------|
| CLK | 17 | Rotation clock (A) |
| DT  | 27 | Rotation data (B) |
| SW  | 22 | Push button |

### Operation

- **Rotation**: Adjusts the active parameter (Ref Level or Range)
- **Button press**: Toggles between Ref Level and Range
- **Visual feedback**: Active parameter label highlighted in cyan
- **Step sizes**: 1 dB per detent for Ref, 5 dB for Range

### Notes

- Only enabled when running in Pi mode (`-p` flag)
- Requires libgpiod library (auto-detected at build time)
- Builds without encoder support if libgpiod is not available

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

- **Spectrum Frame**: Shows current VFO (VFO A or VFO B)
- **Ref**: Reference level (top of spectrum display in dB)
- **Range**: Dynamic range (dB span from ref to bottom)
- **Overlay**: Frequency and mode displayed centered on spectrum (cyan text)
- **Encoder highlight**: Active parameter label shown in cyan bold (Pi mode with encoder)

## Notes

- USB interfaces may require root access or udev rules for user access
- Serial port (/dev/ttyUSB0) requires dialout group membership or root access
- The FDM-DUO FPGA must be initialized before data streaming begins
- Sample rate correction values are read from device EEPROM
- Application auto-reconnects if device is disconnected
