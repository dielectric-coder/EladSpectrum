#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <glib.h>

// Default GPIO pin assignments (BCM numbering)
#define ENCODER_CLK_PIN  17   // Rotation clock (A)
#define ENCODER_DT_PIN   27   // Rotation data (B)
#define ENCODER_SW_PIN   22   // Push button

// Active parameter selection
typedef enum {
    ENCODER_PARAM_REF = 0,    // Reference level
    ENCODER_PARAM_RANGE = 1   // Dynamic range
} encoder_param_t;

// Callback function type for rotation events
// direction: +1 for clockwise, -1 for counter-clockwise
typedef void (*encoder_rotation_callback_t)(int direction, void *user_data);

// Callback function type for button press events
typedef void (*encoder_button_callback_t)(void *user_data);

// Opaque encoder handle
typedef struct rotary_encoder rotary_encoder_t;

// Create and initialize rotary encoder
// Returns NULL on failure (e.g., GPIO not available)
rotary_encoder_t *rotary_encoder_new(void);

// Free encoder resources
void rotary_encoder_free(rotary_encoder_t *encoder);

// Set rotation callback
void rotary_encoder_set_rotation_callback(rotary_encoder_t *encoder,
                                          encoder_rotation_callback_t callback,
                                          void *user_data);

// Set button press callback
void rotary_encoder_set_button_callback(rotary_encoder_t *encoder,
                                        encoder_button_callback_t callback,
                                        void *user_data);

// Get currently active parameter
encoder_param_t rotary_encoder_get_active_param(rotary_encoder_t *encoder);

// Set active parameter
void rotary_encoder_set_active_param(rotary_encoder_t *encoder, encoder_param_t param);

// Toggle active parameter (called internally on button press)
void rotary_encoder_toggle_param(rotary_encoder_t *encoder);

// Start polling (adds GLib timeout source)
// Returns source ID, or 0 on failure
guint rotary_encoder_start_polling(rotary_encoder_t *encoder);

// Stop polling
void rotary_encoder_stop_polling(rotary_encoder_t *encoder);

#endif // ROTARY_ENCODER_H
