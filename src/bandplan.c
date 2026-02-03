#include "bandplan.h"
#include <json-glib/json-glib.h>
#include <stdio.h>
#include <string.h>

int bandplan_load(bandplan_t *plan, const char *filepath) {
    if (!plan || !filepath) return -1;

    memset(plan, 0, sizeof(bandplan_t));

    JsonParser *parser = json_parser_new();
    GError *error = NULL;

    if (!json_parser_load_from_file(parser, filepath, &error)) {
        fprintf(stderr, "Bandplan: Failed to load %s: %s\n",
                filepath, error->message);
        g_error_free(error);
        g_object_unref(parser);
        return -1;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_ARRAY(root)) {
        fprintf(stderr, "Bandplan: Root is not an array\n");
        g_object_unref(parser);
        return -1;
    }

    JsonArray *array = json_node_get_array(root);
    guint len = json_array_get_length(array);

    for (guint i = 0; i < len && plan->count < BANDPLAN_MAX_BANDS; i++) {
        JsonNode *elem = json_array_get_element(array, i);
        if (!JSON_NODE_HOLDS_OBJECT(elem)) continue;

        JsonObject *obj = json_node_get_object(elem);

        // Get name
        const char *name = json_object_get_string_member(obj, "name");
        if (!name) continue;

        // Get bounds
        if (!json_object_has_member(obj, "lower_bound") ||
            !json_object_has_member(obj, "upper_bound")) {
            continue;
        }

        int64_t lower = json_object_get_int_member(obj, "lower_bound");
        int64_t upper = json_object_get_int_member(obj, "upper_bound");

        // Store band
        band_entry_t *band = &plan->bands[plan->count];
        strncpy(band->name, name, sizeof(band->name) - 1);
        band->name[sizeof(band->name) - 1] = '\0';
        band->lower_bound = lower;
        band->upper_bound = upper;
        plan->count++;
    }

    g_object_unref(parser);
    fprintf(stderr, "Bandplan: Loaded %d bands from %s\n", plan->count, filepath);
    return 0;
}

void bandplan_free(bandplan_t *plan) {
    if (plan) {
        plan->count = 0;
    }
}

int bandplan_find_visible(const bandplan_t *plan,
                          int64_t freq_start, int64_t freq_end,
                          int *indices, int max_results) {
    if (!plan || !indices || max_results <= 0) return 0;

    int found = 0;
    for (int i = 0; i < plan->count && found < max_results; i++) {
        const band_entry_t *band = &plan->bands[i];

        // Check if band overlaps with visible range
        // Overlap occurs when: band_start < visible_end AND band_end > visible_start
        if (band->lower_bound < freq_end && band->upper_bound > freq_start) {
            indices[found++] = i;
        }
    }

    return found;
}
