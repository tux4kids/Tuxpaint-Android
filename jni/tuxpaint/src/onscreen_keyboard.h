#include "wchar.h"
#include "stdio.h"
#include "SDL.h"
#include "SDL_ttf.h"
/* after file:///usr/share/doc/libsdl1.2-dev/docs/html/sdlkey.html

Table 8-2. SDL modifier definitions

SDL Modifier	Meaning
KMOD_NONE	No modifiers applicable
KMOD_NUM	Numlock is down
KMOD_CAPS	Capslock is down
KMOD_LCTRL	Left Control is down
KMOD_RCTRL	Right Control is down
KMOD_RSHIFT	Right Shift is down
KMOD_LSHIFT	Left Shift is down
KMOD_RALT	Right Alt is down
KMOD_LALT	Left Alt is down
KMOD_CTRL	A Control key is down
KMOD_SHIFT	A Shift key is down
KMOD_ALT	An Alt key is down

*/



typedef struct osk_keymap
{
  int keycode;                
  int disable_caps;   /* If caps lock should affect this key */
  char * plain;       /* The default Xkeysym for the keycode */
  char * caps;        /* If CapsLock or Shift + key */
  char * altgr;       /* If AltGr + key */
  char * shiftaltgr;  /* If AltGr + Shift + key */
} osk_keymap;

typedef struct osk_key
{
  int keycode;                 /* The code associated to this key. If 0, then it is an empty key. */
  int row;
  int x;
  int y;
  float width;                 /* The width in buttons */
  char *plain_label;        /* The text that will show the key */
  char *top_label;          /* The text that will show the key above the plain label. */
  char *altgr_label;        /* The text that will show the key at the right of the plain label */
  char *shift_altgr_label;  /* The text that will show the key when shift and altgr are activated */
  int shiftcaps;               /* If the value of the key should be shifted when capslock is active */
  int stick;                   /* If the key currently affects the others */
} osk_key;

typedef struct osk_composenode
{
  wchar_t * keysym;
  wchar_t * result;
  int size;                         /* How many childs are there. */
  struct osk_composenode ** childs;
  //  struct osk_composenode **parent;
} osk_composenode;

typedef struct keysymdefs
{
  char * mnemo;
  int keysym;
  int unicode;
} keysymdefs;

typedef struct osk_layout
{
  char *name;
  int *rows;
  int width;
  int height;
  char * fontpath;
  osk_key **keys;
  osk_keymap *keymap;
  osk_composenode * composemap;
  keysymdefs * keysymdefs;
  unsigned int sizeofkeysymdefs;
  SDL_Color bgcolor;
  SDL_Color fgcolor;
} osk_layout;

typedef struct osk_keymodifiers
{
  osk_key shift;
  osk_key altgr;
  osk_key compose;
  osk_key dead;
} osk_keymodifiers;

typedef struct osk_kmdf
{
  osk_key * shift;
  osk_key * altgr;
  osk_key * compose;
  osk_key * dead;
  osk_key * dead2;
  osk_key * dead3;
  osk_key * dead4;
} osk_kmdf;

typedef struct osk_keyboard
{
  char * name;                   /* The name of the keyboard */
  char * keyboard_list;          /* The names of the keyboards allowed from this one */
  SDL_Surface *surface;          /* The surface containing the keyboard */
  SDL_Surface *button_up;        /* The surfaces containing the buttons */
  SDL_Surface *button_down;
  SDL_Surface *button_off;
  SDL_Surface *button_nav;
  SDL_Surface *button_hold;
  SDL_Surface *oskdel;           /* The surfaces containing some symbols for the buttons, delete arrow */
  SDL_Surface *osktab;           /* Tab arrows */
  SDL_Surface *oskenter;         /* Return hook/arrow */
  SDL_Surface *oskcapslock;      /* CapsLock */
  SDL_Surface *oskshift;         /* Shift */
  int changed;                   /* If the surface has been modified (painted)  */
  SDL_Rect rect;                 /* The rectangle that has changed */
  int recreated;                 /* If the surface has been deleted and newly created */ 
  int modifiers;                 /* The state of Alt, CTRL, Shift, CapsLock, AltGr keys */
  osk_keymodifiers keymodifiers; /* A shortcurt to find the place of the pressed modifiers */
  osk_kmdf kmdf;
  osk_layout *layout;            /* The layout struct */ 
  char *layout_name[256];        /* The layout name */
  TTF_Font * osk_fonty;          /* Font */
  int disable_change;            /* If true, stay with the first layout found */
  wchar_t * key[256];            /* The text of the key */
  int keycode;                   /* The unicode code corresponding to the key */
  wchar_t * composed;            /* The unicode char found after a sequence of key presses */
  int composed_type;             /* 1 if the value stored in composed is yet the unicode value */
  osk_composenode * composing;   /* The node in the middle of a compose sequence */
  osk_key * last_key_pressed;    /* The last key pressed */
} on_screen_keyboard;

struct osk_keyboard *osk_create(char *layout_name, SDL_Surface *canvas, SDL_Surface *button_up, SDL_Surface *button_down, SDL_Surface *button_off, SDL_Surface *button_nav, SDL_Surface *button_hold, SDL_Surface *oskdel, SDL_Surface *osktab, SDL_Surface *oskenter, SDL_Surface *oskcapslock, SDL_Surface *oskshift, int disable_change);

struct osk_layout *osk_load_layout(char *layout_name);

void osk_get_layout_data(char *layout_name, int *layout_w, int *layout_h, char * layout_buttons, char *layout_labels, char *layout_keycodes);
void osk_reset(on_screen_keyboard *osk);
struct osk_keyboard * osk_clicked(on_screen_keyboard *keyboard, int x, int y);
void osk_released(on_screen_keyboard *osk);
void osk_hover(on_screen_keyboard *keyboard, int x, int y);
void osk_free(on_screen_keyboard *osk);
void osk_change_layout(on_screen_keyboard *osk);
