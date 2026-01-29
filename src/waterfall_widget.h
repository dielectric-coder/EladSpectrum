#ifndef WATERFALL_WIDGET_H
#define WATERFALL_WIDGET_H

#include <gtk/gtk.h>
#include "app_state.h"

G_BEGIN_DECLS

#define WATERFALL_TYPE_WIDGET (waterfall_widget_get_type())
G_DECLARE_FINAL_TYPE(WaterfallWidget, waterfall_widget, WATERFALL, WIDGET, GtkDrawingArea)

// Create a new waterfall widget
GtkWidget *waterfall_widget_new(void);

// Add a new spectrum line (thread-safe, copies data)
void waterfall_widget_add_line(WaterfallWidget *widget, const float *spectrum_db, int size);

// Set display range
void waterfall_widget_set_range(WaterfallWidget *widget, float min_db, float max_db);

// Clear waterfall history
void waterfall_widget_clear(WaterfallWidget *widget);

G_END_DECLS

#endif // WATERFALL_WIDGET_H
