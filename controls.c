#include "controls.h"

#include <stdlib.h>
#include <stdio.h>
//#include <threads.h>
//#include "c11threads/threads.h"

#define MTX_LOCK(code)                                                             \
do {                                                                               \
	success = pthread_mutex_lock(&game_control->mutex);                            \
	if(success != 0) { /* TODO */                                                  \
		fprintf(stderr, "ERROR: Failed to lock control mutex before writing.\n");  \
	}                                                                              \
	code;                                                                          \
	success = pthread_mutex_unlock(&game_control->mutex);                            \
	if(success != 0) {                                                             \
		fprintf(stderr, "ERROR: Failed to unlock control mutex after writing.\n"); \
	}                                                                              \
} while(0);

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

void destroy_game_control(GameControl *game_control) {
	pthread_mutex_destroy(&game_control->mutex); /* TODO: check failure */
}

static void key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	//fprintf(stderr, "Key Press\n");
	GameControl *game_control = (GameControl *)data;
	int success;
	switch(event->keyval) {
		case CREDIT:   MTX_LOCK(game_control->credit += 1;);       break;
		case P1_START: MTX_LOCK(game_control->player1.start = 1;); break;
		case P1_FIRE:  MTX_LOCK(game_control->player1.fire  = 1;); break;
		case P1_LEFT:  MTX_LOCK(game_control->player1.left  = 1;); break;
		case P1_RIGHT: MTX_LOCK(game_control->player1.right = 1;); break;
		case P2_START: MTX_LOCK(game_control->player2.start = 1;); break;
		case P2_FIRE:  MTX_LOCK(game_control->player2.fire  = 1;); break;
		case P2_LEFT:  MTX_LOCK(game_control->player2.left  = 1;); break;
		case P2_RIGHT: MTX_LOCK(game_control->player2.right = 1;); break;
		default: break;
	}
}

static void key_release_event(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	//fprintf(stderr, "Key Release\n");
	GameControl *game_control = (GameControl *)data;
	int success;
	switch(event->keyval) {
		case P1_START: MTX_LOCK(game_control->player1.start = 0;); break;
		case P1_FIRE:  MTX_LOCK(game_control->player1.fire  = 0;); break;
		case P1_LEFT:  MTX_LOCK(game_control->player1.left  = 0;); break;
		case P1_RIGHT: MTX_LOCK(game_control->player1.right = 0;); break;
		case P2_START: MTX_LOCK(game_control->player2.start = 0;); break;
		case P2_FIRE:  MTX_LOCK(game_control->player2.fire  = 0;); break;
		case P2_LEFT:  MTX_LOCK(game_control->player2.left  = 0;); break;
		case P2_RIGHT: MTX_LOCK(game_control->player2.right = 0;); break;
		default: break;
	}
}

void set_control_events(GtkWidget *widget, GameControl *game_control) {
	g_signal_connect(widget, "key-press-event", G_CALLBACK(key_press_event), game_control);
	g_signal_connect(widget, "key-release-event", G_CALLBACK(key_release_event), game_control);

	gtk_widget_set_events(widget, gtk_widget_get_events(widget) | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
}
