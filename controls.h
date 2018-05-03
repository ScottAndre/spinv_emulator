#ifndef SPINV_CONTROLS
#define SPINV_CONTROLS

#include <gtk/gtk.h>
#include <pthread.h>
#include <stdint.h>

/* Key codes obtained from gdk/gdkkeysyms.h */

#define CREDIT GDK_KEY_c

#define P1_START GDK_KEY_Return
#define P1_FIRE GDK_KEY_space
#define P1_LEFT GDK_KEY_Left
#define P1_RIGHT GDK_KEY_Right

#define P2_START GDK_KEY_KP_Enter
#define P2_FIRE GDK_KEY_KP_0
#define P2_LEFT GDK_KEY_KP_4
#define P2_RIGHT GDK_KEY_KP_6

/* represents all controls for a single player */
typedef struct {
	uint8_t start:1;
	uint8_t fire:1;
	uint8_t left:1;
	uint8_t right:1;
	uint8_t padding:4;
} PlayerControl;

/* represents all controls available in the game */
typedef struct {
	uint8_t credit;
	PlayerControl player1;
	PlayerControl player2;
	pthread_mutex_t mutex; /* Controls are currently only set from one thread. Better to be safe anyway. */
} GameControl;

void init_game_control(GameControl *game_control);
void destroy_game_control(GameControl *game_control);

void set_control_events(GtkWidget *widget, GameControl *game_control);

#endif
