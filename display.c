#include "display.h"
#include "controls.h"

#include <stdint.h>
#include <stdio.h>

#define DRAW_TOP 1
#define DRAW_BOTTOM 0

typedef struct {
	GtkWidget *screen;
	uint8_t *memory;
	Interrupt *interrupts;
} RefreshData;

static cairo_surface_t *surface = NULL;
/* this data must be global because it must remain available for the screen refresh callback after the activate function exits, but it cannot be initialized outside of the activate function (I believe) */
static guint timeout_id;
static RefreshData refresh_data;

static gdouble display_scale;

static gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
	if(surface) {
		cairo_surface_destroy(surface);
	}

	/* WIDTH and HEIGHT are reversed here, since the display is rotated 90 degrees in the cabinet */
	gdouble xscale = ((gdouble)event->width)/DISPLAY_HEIGHT;
	gdouble yscale = ((gdouble)event->height)/DISPLAY_WIDTH;
	display_scale = xscale < yscale ? xscale : yscale;

	surface = gdk_window_create_similar_surface(gtk_widget_get_window(widget), CAIRO_CONTENT_COLOR, DISPLAY_HEIGHT * display_scale, DISPLAY_WIDTH * display_scale);

	return TRUE;
}

static gboolean draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	//fprintf(stderr, "Draw\n");
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_paint(cr);

	return FALSE;
}

/* TODO: rotate CCW 90 degrees */
/* TODO: allow the display to scale if the window is resized - look up cairo transforms */
static gboolean refresh(gpointer data) {
	//fprintf(stderr, "Refresh\n");
	RefreshData *rd = (RefreshData *)data;

	cairo_t *cr = cairo_create(surface);

	static int draw_side = DRAW_TOP;
	int line_start, line_end;
	if(draw_side == DRAW_TOP) {
		line_start = 0;
		line_end = DISPLAY_HEIGHT/2;
	}
	else { /* draw_side == DRAW_BOTTOM */
		line_start = DISPLAY_HEIGHT/2;
		line_end = DISPLAY_HEIGHT;
	}

	int i, j, b;
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
	cairo_set_line_width(cr, display_scale);
	for(j = line_start; j < line_end; j++) {
		//fprintf(stderr, "Drawing line %d\n", j);
		uint8_t previous_bit = 0;
		cairo_set_source_rgb(cr, 0, 0, 0); /* Initialize the new line - we'll assume the first pixel will be black. If it's not, no big deal */
		//cairo_move_to(cr, 0, j); /* Begin a new line */
		cairo_move_to(cr, j * display_scale, (DISPLAY_WIDTH - 1) * display_scale); /* rotated, scaled */

		for(i = 0; i < DISPLAY_WIDTH/8; i++) {
			uint8_t byte = rd->memory[VRAM_START_ADDRESS + j*(DISPLAY_WIDTH/8) + i];

			for(b = 0; b < 7; b++) {
				uint8_t bit = (byte >> b) & 0x1;
				if(bit != previous_bit) {
					if(i != 0 || b != 0) { /* no need to draw if the first bit is different from our presupposition */
						//cairo_line_to(cr, i*8 + b - 1, j);
						cairo_line_to(cr, j * display_scale, (DISPLAY_WIDTH - (i*8 + b)) * display_scale); /* rotated, scaled */
						cairo_stroke(cr);
					}
					if(bit == 0) {
						cairo_set_source_rgb(cr, 0, 0, 0);
					} else {
						cairo_set_source_rgb(cr, 1, 1, 1);
					}
					//cairo_move_to(cr, i*8 + b, j);
					cairo_move_to(cr, j * display_scale, (DISPLAY_WIDTH - (i*8 + b) - 1) * display_scale); /* rotated, scaled */
					previous_bit = bit;
				}
			}
		}

		//cairo_line_to(cr, i*8 - 1, j); /* Finish drawing the line */
		cairo_line_to(cr, j * display_scale, 0); /* rotated, scaled */
		cairo_stroke(cr);
	}

	if(draw_side == DRAW_TOP) {
		draw_side = DRAW_BOTTOM;
		trigger_hblank(rd->interrupts);
	}
	else { /* draw_side == DRAW_BOTTOM */
		draw_side = DRAW_TOP;
		trigger_vblank(rd->interrupts);
	}

	gtk_widget_queue_draw(rd->screen);
	//fprintf(stderr, "Finished drawing\n");

	return TRUE; /* do not cancel the timeout */
}

static void close_window() {
	if(surface) {
		cairo_surface_destroy(surface);
	}
}

static void activate(GtkApplication *app, gpointer user_data) {
	GameState *game_state = (GameState *)user_data;

	GtkWidget *window;
	GtkWidget *game_screen;

	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Space Invaders");

	g_signal_connect(window, "destroy", G_CALLBACK(close_window), NULL);

	gtk_container_set_border_width(GTK_CONTAINER(window), 0);

	game_screen = gtk_drawing_area_new();
	/* WIDTH and HEIGHT are reversed due to the screen being rotated in the cabinet, in case that wasn't abundantly clear by now */
	gtk_widget_set_size_request(game_screen, DISPLAY_HEIGHT, DISPLAY_WIDTH);
	gtk_container_add(GTK_CONTAINER(window), game_screen);
	display_scale = 1.0; /* initial window scale is 1:1 with original Space Invaders display */

	g_signal_connect(game_screen, "draw", G_CALLBACK(draw), NULL);
	g_signal_connect(game_screen, "configure_event", G_CALLBACK(configure_event), NULL);
	// size-allocate is triggered when the window is resized. Maybe redraw the screen according to the new resolution?
	//g_signal_connect(game_screen, "size-allocate", G_CALLBACK(size_allocate), NULL);

	set_control_events(window, game_state->game_control);

	gtk_widget_show_all(window);

	/* set a timeout to check memory a draw the screen 60 times per second (actually 120, top half -> hblank, bottom half -> vblank) */
	refresh_data.screen = game_screen;
	refresh_data.memory = game_state->memory;
	refresh_data.interrupts = game_state->interrupts;
	guint interval = (guint)((1.0/120.0)*1000); /* interval is given in terms of milliseconds */
	timeout_id = g_timeout_add(interval, refresh, &refresh_data);
}

void init_display(GtkApplication **app, GameState *game_state) {
	*app = gtk_application_new("spinvemu.emulator", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(*app, "activate", G_CALLBACK(activate), game_state);
}

int run_display(GtkApplication *app, int argc, char **argv) {
	int status = g_application_run(G_APPLICATION(app), 1, argv); /* The application doesn't really need to know any command-line arguments, and gets pissy if there's a file name present and it doesn't have the HANDLES_OPEN flag set */
	return status;
}

void close_display(GtkApplication *app) {
	g_source_remove(timeout_id);
	g_object_unref(app);
}
