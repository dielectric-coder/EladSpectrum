# Elad Spectrum User Guide

A spectrum analyzer and waterfall display for the Elad FDM-DUO transceiver.

## Table of Contents

1. [Introduction](#introduction)
2. [Requirements](#requirements)
3. [Installation](#installation)
4. [Getting Started](#getting-started)
5. [Understanding the Display](#understanding-the-display)
6. [Controls](#controls)
7. [Raspberry Pi Mode](#raspberry-pi-mode)
8. [Troubleshooting](#troubleshooting)

---

## Introduction

Elad Spectrum provides a real-time spectrum analyzer and waterfall display for your Elad FDM-DUO Software Defined Radio. It shows:

- **Spectrum Display**: Real-time signal strength across 192 kHz bandwidth
- **Waterfall Display**: Signal history over time with color-coded intensity
- **Radio Status**: Current frequency, mode, filter bandwidth, and VFO
- **Filter Visualization**: Bandwidth lines showing your receiver's passband

The application connects to your FDM-DUO via USB for spectrum data and serial port for radio control information.

---

## Requirements

### Hardware

- Elad FDM-DUO transceiver
- USB connection to computer
- Serial port connection (typically `/dev/ttyUSB0`)

### Software Dependencies

**Debian/Ubuntu:**
```bash
sudo apt install libgtk-4-1 libusb-1.0-0 libfftw3-3
```

**For Raspberry Pi (optional encoder support):**
```bash
sudo apt install libgpiod2
```

### Permissions

You need access to USB and serial devices:

```bash
# Add yourself to the dialout group (for serial port)
sudo usermod -a -G dialout $USER

# Create udev rule for FDM-DUO USB access (optional)
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1721", ATTR{idProduct}=="061a", MODE="0666"' | \
    sudo tee /etc/udev/rules.d/99-elad-fdm-duo.rules
sudo udevadm control --reload-rules

# Log out and back in for group changes to take effect
```

---

## Installation

### From Source

```bash
# Install build dependencies
sudo apt install libgtk-4-dev libusb-1.0-0-dev libfftw3-dev meson ninja-build

# Build
meson setup build
meson compile -C build

# Run
./build/elad-spectrum
```

---

## Getting Started

### Basic Startup

1. Connect your FDM-DUO to your computer via USB
2. Turn on the FDM-DUO
3. Run the application:
   ```bash
   ./build/elad-spectrum
   ```

### Command-Line Options

| Option | Description |
|--------|-------------|
| `-f, --fullscreen` | Start in fullscreen mode |
| `-p, --pi` | Raspberry Pi mode (800x480, dark theme, encoder support) |
| `-h, --help` | Show help message |

### Examples

```bash
# Normal windowed mode (1024x768)
./build/elad-spectrum

# Fullscreen
./build/elad-spectrum -f

# Raspberry Pi with 5" LCD
./build/elad-spectrum -p -f
```

---

## Understanding the Display

### Main Window Layout

```
┌─────────────────────────────────────────────────────────┐
│ VFO A                                                   │
│ ┌─────────────────────────────────────────────────────┐ │
│ │        14.200000 MHz  USB 2.4k                      │ │
│ │ +0 ─┬─────────────────────┬─────────────────────┬── │ │
│ │     │                     │                     │   │ │
│ │-30 ─┤        ╱╲    ╱╲    │╱╲                   │   │ │
│ │     │       ╱  ╲  ╱  ╲  ╱│  ╲                  │   │ │
│ │-60 ─┤      ╱    ╲╱    ╲╱ │   ╲    ╱╲          │   │ │
│ │     │     ╱              │    ╲  ╱  ╲         │   │ │
│ │-90 ─┤    ╱               │     ╲╱    ╲        │   │ │
│ │     │───╱────────────────│──────────────╲─────│   │ │
│ │-120─┴────────────────────┴─────────────────────┴── │ │
│ │     14.100    14.150    14.200    14.250    14.300 │ │
│ └─────────────────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────────────────┐ │
│ │ LOCAL 14:32:15                      UTC 12:32:15   │ │
│ │ 0s ─░░░▒▒▓▓██│██▓▓▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │ │
│ │     ░░░▒▒▓▓██│██▓▓▒▒░░░░░░░░░░░▒▒░░░░░░░░░░░░░░░░░ │ │
│ │ 5s ─░░░▒▒▓▓██│██▓▓▒▒░░░░░░░░░░▒▒▓▓▒▒░░░░░░░░░░░░░░ │ │
│ │     ░░░▒▒▓▓██│██▓▓▒▒░░░░░░░░░░░▒▒░░░░░░░░░░░░░░░░░ │ │
│ │ 10s─░░░▒▒▓▓██│██▓▓▒▒░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │ │
│ └─────────────────────────────────────────────────────┘ │
│                                                         │
│            ● Ref [-30] Rng [120]                        │
└─────────────────────────────────────────────────────────┘
```

### Display Elements

#### Status Indicator
- **Green circle (●)**: Connected and receiving data
- **Gray circle (○)**: Disconnected or not streaming
- **Red circle (●)**: Error state

#### Spectrum Display (Top)

| Element | Description |
|---------|-------------|
| Frame label | Shows current VFO (VFO A or VFO B) |
| Overlay text | Frequency, mode, and filter bandwidth (cyan) |
| Cyan trace | Real-time signal strength |
| Red vertical line | Tuned frequency marker |
| Left axis | Signal strength in dB |
| Bottom axis | Frequency in MHz |
| Grid | 10x10 reference grid |

#### Waterfall Display (Bottom)

| Element | Description |
|---------|-------------|
| Time labels | Time since signal was received (left side) |
| LOCAL/UTC | Current time display (top corners) |
| Colors | Signal strength: black (weak) → blue → cyan → green → yellow → red (strong) |
| Red dashed lines | Filter bandwidth edges |
| Orange dashed lines | CW resonator filter (100 Hz modes) |

#### Bandwidth Lines

The dashed vertical lines on the waterfall show your filter passband:

| Mode | Line Display |
|------|--------------|
| USB | Single line at upper filter edge |
| LSB | Single line at lower filter edge |
| CW, AM, FM | Two symmetric lines around center |
| Data modes (D300, D600, D1k) | Two lines centered at +1500 Hz offset |
| CW Resonator (100&1-4) | Orange lines, 100 Hz bandwidth |

### Color Scale

The waterfall uses this color gradient for signal strength:

```
Weak                                              Strong
 │                                                    │
 ▼                                                    ▼
Black → Blue → Cyan → Green → Yellow → Red
```

---

## Controls

### Desktop Mode Controls

| Control | Function |
|---------|----------|
| **Ref** spinner | Reference level (top of display) in dB |
| **Rng** spinner | Dynamic range (display span) in dB |

**Typical Settings:**
- Strong signals: Ref = -20 dB, Range = 80 dB
- Weak signals: Ref = -40 dB, Range = 100 dB
- Very weak signals: Ref = -50 dB, Range = 120 dB

### Settings Persistence

Your display settings are automatically saved:
- **Auto-save**: 3 seconds after any change
- **On exit**: When you close the window
- **Location**: `~/.config/elad-spectrum/settings.conf`

Saved settings include:
- Spectrum reference and range
- Waterfall reference and range
- Zoom level and pan offset (Pi mode)

---

## Raspberry Pi Mode

Pi mode (`-p` flag) is optimized for embedded displays like the official 5" LCD.

### Display Differences

- Window size: 800x480 pixels
- Dark theme (black background)
- Compact control bar showing all parameters:
  ```
  ● SP.REF -30  SP.RNG 120  WF.REF -30  WF.RNG 120  SPAN 192  OFS +0
  ```

### Rotary Encoder Setup

Pi mode supports two rotary encoders for hands-free control.

#### Wiring

| Encoder | Function | GPIO Pin |
|---------|----------|----------|
| Encoder 1 | CLK (rotation) | GPIO 17 |
| Encoder 1 | DT (direction) | GPIO 27 |
| Encoder 1 | SW (button) | GPIO 22 |
| Encoder 2 | CLK (rotation) | GPIO 5 |
| Encoder 2 | DT (direction) | GPIO 6 |
| Encoder 2 | SW (button) | GPIO 13 |

Connect encoder common pins to ground. Internal pull-ups are enabled.

#### Encoder 1 - Parameter Control

| Action | Effect |
|--------|--------|
| **Button press** | Cycle through parameters (highlighted in cyan) |
| **Rotate** | Adjust selected parameter |

Parameters cycle: SP.REF → SP.RNG → WF.REF → WF.RNG → (repeat)

- Reference levels: ±1 dB per click
- Range values: ±5 dB per click

#### Encoder 2 - Zoom/Pan Control

| Action | Effect |
|--------|--------|
| **Button press** | Toggle between Zoom and Pan mode |
| **Rotate (Zoom mode)** | Change zoom level (1x ↔ 16x) |
| **Rotate (Pan mode)** | Move view left/right |

**Zoom Levels:**

| Zoom | Span | Resolution |
|------|------|------------|
| 1x | 192 kHz | 46.9 Hz/bin |
| 2x | 96 kHz | 23.4 Hz/bin |
| 4x | 48 kHz | 11.7 Hz/bin |
| 8x | 24 kHz | 5.9 Hz/bin |
| 16x | 12 kHz | 2.9 Hz/bin |

The active mode (SPAN or OFS) is highlighted in cyan.

---

## Troubleshooting

### Connection Issues

#### "Cannot open FDM-DUO device"

**Cause**: USB permission denied or device not connected.

**Solutions**:
1. Check USB connection
2. Verify FDM-DUO is powered on
3. Run with sudo (temporary): `sudo ./build/elad-spectrum`
4. Set up udev rules (permanent):
   ```bash
   echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1721", ATTR{idProduct}=="061a", MODE="0666"' | \
       sudo tee /etc/udev/rules.d/99-elad-fdm-duo.rules
   sudo udevadm control --reload-rules
   ```
5. Unplug and replug the USB cable

#### "CAT: Cannot open /dev/ttyUSB0"

**Cause**: Serial port permission denied or wrong device.

**Solutions**:
1. Add yourself to dialout group:
   ```bash
   sudo usermod -a -G dialout $USER
   # Log out and back in
   ```
2. Check if device exists: `ls -la /dev/ttyUSB*`
3. The FDM-DUO serial port may be on a different device (ttyUSB1, etc.)

#### Gray status indicator (not connecting)

**Cause**: Application can't find or communicate with FDM-DUO.

**Solutions**:
1. Ensure FDM-DUO firmware is up to date
2. Try unplugging other USB devices
3. Use a shorter or higher-quality USB cable
4. Try a different USB port (USB 2.0 recommended)

### Display Issues

#### No spectrum displayed

**Possible causes**:
- USB streaming not started (check status indicator)
- FDM-DUO in standby mode
- Antenna not connected

**Solutions**:
1. Wait a few seconds after connection
2. Check that FDM-DUO front panel shows activity
3. Verify antenna connection

#### Frequency/mode not updating

**Cause**: CAT serial connection not working.

**Solutions**:
1. Check serial cable connection
2. Verify correct serial port
3. CAT control is optional - spectrum will still work

#### Waterfall appears frozen

**Cause**: Display range may not match signal levels.

**Solutions**:
1. Adjust **Ref** (reference level) to match your signals
2. Increase **Rng** (range) for more dynamic range
3. Check that signals are present on the spectrum display

### Raspberry Pi Issues

#### Encoders not responding

**Possible causes**:
- Not running in Pi mode
- GPIO permission denied
- Wiring incorrect

**Solutions**:
1. Ensure you're using `-p` flag
2. Check GPIO permissions (may need to run as root first time)
3. Verify wiring matches the GPIO table above
4. Check encoder common is connected to ground

#### Display too large for screen

**Solution**: Use both `-p` and `-f` flags:
```bash
./build/elad-spectrum -p -f
```

### Performance Issues

#### High CPU usage

**Normal behavior**: The application processes ~15 FFT frames per second.

**If excessive**:
1. Close other applications
2. On Raspberry Pi, ensure GPU memory is set appropriately
3. Check for thermal throttling

#### Choppy waterfall

**Cause**: System unable to maintain display refresh rate.

**Solutions**:
1. Close other applications
2. Reduce window size
3. On Pi, use fullscreen mode for better performance

---

## Keyboard Shortcuts

Currently, all interaction is via the control bar and optional rotary encoders. Future versions may add keyboard shortcuts.

---

## Technical Specifications

| Parameter | Value |
|-----------|-------|
| Sample rate | 192 kHz |
| FFT size | 4096 points |
| Frequency resolution | 46.9 Hz/bin |
| Spectrum averaging | 3 frames |
| Update rate | ~15.6 Hz |
| Display refresh | 30 fps |

---

## Getting Help

- **Issues**: [GitHub Issues](https://github.com/dielectric-coder/EladSpectrum/issues)
- **Source**: [GitHub Repository](https://github.com/dielectric-coder/EladSpectrum)
