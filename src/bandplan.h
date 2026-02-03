#ifndef BANDPLAN_H
#define BANDPLAN_H

#include <stdint.h>

#define BANDPLAN_MAX_BANDS 64

typedef struct {
    char name[32];
    int64_t lower_bound;  // Hz
    int64_t upper_bound;  // Hz
} band_entry_t;

typedef struct {
    band_entry_t bands[BANDPLAN_MAX_BANDS];
    int count;
} bandplan_t;

// Load bandplan from JSON file
// Returns 0 on success, -1 on error
int bandplan_load(bandplan_t *plan, const char *filepath);

// Free bandplan resources (no-op for static array, but good practice)
void bandplan_free(bandplan_t *plan);

// Find bands that overlap with a frequency range
// Returns number of bands found, fills indices array with band indices
// indices must have space for at least max_results entries
int bandplan_find_visible(const bandplan_t *plan,
                          int64_t freq_start, int64_t freq_end,
                          int *indices, int max_results);

#endif // BANDPLAN_H
