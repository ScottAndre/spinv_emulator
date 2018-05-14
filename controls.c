#include "controls.h"

#include <stdlib.h>
#include <stdio.h>
//#include <threads.h>
//#include "c11threads/threads.h"

#define SET_CONTROL(control, value)                                                    \
do {                                                                                   \
	if(control != value) {                                                             \
		success = pthread_mutex_lock(&game_control->mutex);                            \
		if(success != 0) { /* TODO */                                                  \
			fprintf(stderr, "ERROR: Failed to lock control mutex before writing.\n");  \
		}                                                                              \
		control = value;                                                               \
		success = pthread_mutex_unlock(&game_control->mutex);                          \
		if(success != 0) { /* TODO */                                                  \
			fprintf(stderr, "ERROR: Failed to unlock control mutex after writing.\n"); \
		}                                                                              \
	}                                                                                  \
} while(0);

/* TODO DELETE */
static uint8_t *mem;

void init_game_control(GameControl *game_control) {
	game_control->credit = 0;
	game_control->player1.start = 0;
	game_control->player1.fire  = 0;
	game_control->player1.left  = 0;
	game_control->player1.right = 0;
	game_control->player2.start = 0;
	game_control->player2.fire  = 0;
	game_control->player2.left  = 0;
	game_control->player2.right = 0;
	int success = pthread_mutex_init(&game_control->mutex, NULL);
	if(success != 0) { /* TODO: check for individual error codes */
		fprintf(stderr, "ERROR: Failed to initialize control mutex.");
		exit(EXIT_FAILURE);
	}
}

/* TODO DELETE */
void set_control_debug_memory_pointer(uint8_t *memory) {
	mem = memory;
}

void destroy_game_control(GameControl *game_control) {
	pthread_mutex_destroy(&game_control->mutex); /* TODO: check failure */
}

static void dump_vram() {
	int vram_start = 0x2400;
	int vram_end = 0x4000;
	int i, j;
	for(i = vram_start; i < vram_end; i = i + 0x20) {
		for(j = 0x00; j < 0x20; j++) {
			uint8_t byte = mem[i + j];
			fprintf(stdout, "%d%d%d%d%d%d%d%d ", byte & 0x1, (byte >> 1) & 0x1, (byte >> 2) & 0x1, (byte >> 3) & 0x1, (byte >> 4) & 0x1, (byte >> 5) & 0x1, (byte >> 6) & 0x1, (byte >> 7) & 0x1);
		}
		fprintf(stdout, "\n");
	}
}

static void key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	GameControl *game_control = (GameControl *)data;
	int success;
	switch(event->keyval) {
		case CREDIT:   SET_CONTROL(game_control->credit, 1);        break;
		case P1_START: SET_CONTROL(game_control->player1.start, 1); break;
		case P1_FIRE:  SET_CONTROL(game_control->player1.fire,  1); break;
		case P1_LEFT:  SET_CONTROL(game_control->player1.left,  1); break;
		case P1_RIGHT: SET_CONTROL(game_control->player1.right, 1); break;
		case P2_START: SET_CONTROL(game_control->player2.start, 1); break;
		case P2_FIRE:  SET_CONTROL(game_control->player2.fire,  1); break;
		case P2_LEFT:  SET_CONTROL(game_control->player2.left,  1); break;
		case P2_RIGHT: SET_CONTROL(game_control->player2.right, 1); break;
		case DUMP_VRAM: dump_vram(); break;
		default: break;
	}
}

static void key_release_event(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	GameControl *game_control = (GameControl *)data;
	int success;
	switch(event->keyval) {
		case CREDIT:   SET_CONTROL(game_control->credit, 0);        break;
		case P1_START: SET_CONTROL(game_control->player1.start, 0); break;
		case P1_FIRE:  SET_CONTROL(game_control->player1.fire,  0); break;
		case P1_LEFT:  SET_CONTROL(game_control->player1.left,  0); break;
		case P1_RIGHT: SET_CONTROL(game_control->player1.right, 0); break;
		case P2_START: SET_CONTROL(game_control->player2.start, 0); break;
		case P2_FIRE:  SET_CONTROL(game_control->player2.fire,  0); break;
		case P2_LEFT:  SET_CONTROL(game_control->player2.left,  0); break;
		case P2_RIGHT: SET_CONTROL(game_control->player2.right, 0); break;
		default: break;
	}
}

void set_control_events(GtkWidget *widget, GameControl *game_control) {
	g_signal_connect(widget, "key-press-event", G_CALLBACK(key_press_event), game_control);
	g_signal_connect(widget, "key-release-event", G_CALLBACK(key_release_event), game_control);

	gtk_widget_set_events(widget, gtk_widget_get_events(widget) | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
}
