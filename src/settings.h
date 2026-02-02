#ifndef SETTINGS_H
#define SETTINGS_H

// Application settings that persist between sessions
typedef struct {
    double spectrum_ref;
    double spectrum_range;
    double waterfall_ref;
    double waterfall_range;
    int zoom_level;
    int pan_offset;
} app_settings_t;

// Load settings from config file (~/.config/elad-spectrum/settings.conf)
// If file doesn't exist, settings are initialized to defaults
void settings_load(app_settings_t *settings);

// Save settings to config file
// Creates directory if it doesn't exist
void settings_save(const app_settings_t *settings);

// Initialize settings to default values
void settings_init_defaults(app_settings_t *settings);

#endif // SETTINGS_H
