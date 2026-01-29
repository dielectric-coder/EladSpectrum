#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <stdbool.h>
#include <libusb-1.0/libusb.h>
#include "app_state.h"

#define ELAD_VENDOR_ID  0x1721
#define ELAD_PRODUCT_ID 0x061a
#define ELAD_RF_ENDPOINT 0x86

typedef struct usb_device usb_device_t;

// Callback for received IQ samples
typedef void (*usb_sample_callback_t)(const uint8_t *data, int length, void *user_data);

// Create USB device handler
usb_device_t *usb_device_new(void);

// Free USB device handler
void usb_device_free(usb_device_t *dev);

// Open and initialize the FDM-DUO device
// Returns 0 on success, negative on error
int usb_device_open(usb_device_t *dev);

// Close the device
void usb_device_close(usb_device_t *dev);

// Check if device is open
bool usb_device_is_open(usb_device_t *dev);

// Set the center frequency in Hz
int usb_device_set_frequency(usb_device_t *dev, long freq_hz);

// Start streaming RF data
int usb_device_start_streaming(usb_device_t *dev, usb_sample_callback_t callback, void *user_data);

// Stop streaming
void usb_device_stop_streaming(usb_device_t *dev);

// Process USB events (call from USB thread)
int usb_device_handle_events(usb_device_t *dev);

// Get device info strings
const char *usb_device_get_serial(usb_device_t *dev);
int usb_device_get_hw_version_major(usb_device_t *dev);
int usb_device_get_hw_version_minor(usb_device_t *dev);

// Read current frequency from radio (returns frequency in Hz, or -1 on error)
long usb_device_get_frequency(usb_device_t *dev);

// Radio modes
typedef enum {
    ELAD_MODE_UNKNOWN = 0,
    ELAD_MODE_AM = 1,
    ELAD_MODE_LSB = 2,
    ELAD_MODE_USB = 3,
    ELAD_MODE_CW = 4,
    ELAD_MODE_FM = 5,
    ELAD_MODE_CWR = 6
} elad_mode_t;

// Read current mode from radio (returns mode, or ELAD_MODE_UNKNOWN on error)
elad_mode_t usb_device_get_mode(usb_device_t *dev);

// Read frequency and mode together in a single USB transfer (more reliable)
int usb_device_get_freq_mode(usb_device_t *dev, long *freq_hz, elad_mode_t *mode);

// Get mode name string
const char *usb_device_mode_name(elad_mode_t mode);

#endif // USB_DEVICE_H
