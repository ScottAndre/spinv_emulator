#ifndef SPINV_DISPLAY
#define SPINV_DISPLAY

#include "emulator.h"

#include <gtk/gtk.h>

/* screen is rotated 90 degrees CCW, so it's actually 224*256 */
#define DISPLAY_WIDTH  256
#define DISPLAY_HEIGHT 224
#define VRAM_START_ADDRESS 0x2400 /* VRAM goes from 0x2400-3FFF, each bit mapping to one pixel */

/* prepares the display object for drawing. Call once before making any other calls to the display. */
void init_display(GtkApplication **app, GameState *game_state);

int run_display(GtkApplication *app, int argc, char **argv);

/* frees the memory associated with the display object. Use as part of cleanup. */
void close_display(GtkApplication *app);

#endif
