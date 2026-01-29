#ifndef CAT_CONTROL_H
#define CAT_CONTROL_H

#include <stdbool.h>
#include "usb_device.h"  // For elad_mode_t

typedef struct cat_control cat_control_t;

// Create CAT control handler
cat_control_t *cat_control_new(void);

// Free CAT control handler
void cat_control_free(cat_control_t *cat);

// Open serial port (e.g., "/dev/ttyUSB0")
int cat_control_open(cat_control_t *cat, const char *device);

// Close serial port
void cat_control_close(cat_control_t *cat);

// Check if port is open
bool cat_control_is_open(cat_control_t *cat);

// Read frequency, mode and VFO via CAT commands (single poll)
// vfo: 0=VFO A, 1=VFO B
// Returns 0 on success, -1 on error
int cat_control_get_freq_mode(cat_control_t *cat, long *freq_hz, elad_mode_t *mode, int *vfo);

#endif // CAT_CONTROL_H
