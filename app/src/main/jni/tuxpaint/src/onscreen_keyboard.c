/*
  onscreen_keyboard.c

  Copyright (c) 2011-2023
  https://tuxpaint.org/

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  (See COPYING.txt)

  Last modified: March 23, 2023
*/

#include "debug.h"
#include "onscreen_keyboard.h"

#ifdef DEBUG
#define OSK_DEBUG
#define DEBUG_OSK_COMPOSEMAP
#endif

#include "SDL2_rotozoom.h"

#if !defined(_SDL2_rotozoom_h)
#error "---------------------------------------------------"
#error "If you installed SDL_gfx from a package, be sure"
#error "to get the development package, as well!"
#error "(e.g., 'libsdl-gfx1.2-devel.rpm')"
#error "---------------------------------------------------"
#endif

static SDL_Color def_bgcolor = { 255, 255, 255, 255 };
static SDL_Color def_fgcolor = { 0, 0, 0, 0 };

static void load_hlayout(osk_layout * layout, char *layout_name);
static void load_keymap(osk_layout * layout, char *keymap_name);
static void load_composemap(osk_layout * layout, char *composemap_name);

static int is_blank_or_comment(char *line);

/* static int isw_blank_or_comment(wchar_t *line); */


static void keybd_prepare(on_screen_keyboard * keyboard);

static void draw_key(osk_key key, on_screen_keyboard * keyboard, int hot);

static void label_key(osk_key key, on_screen_keyboard * keyboard);
static void draw_keyboard(on_screen_keyboard * keyboard);
static osk_key *find_key(on_screen_keyboard * keyboard, int x, int y);
static void set_key(osk_key * orig, osk_key * dest, int firsttime);
static void load_keysymdefs(osk_layout * layout, char *keysymdefs_name);
static struct osk_layout *load_layout(on_screen_keyboard * keyboard, char *layout_name);

#ifdef DEBUG_OSK_COMPOSEMAP
static void print_composemap(osk_composenode * composemap, char *sp);
#endif

#ifdef WIN32
#include <windows.h>
#define mbstowcs(wtok, tok, size) MultiByteToWideChar(CP_UTF8,0,tok,-1,wtok,size)
#define wcstombs(tok, wtok, size) WideCharToMultiByte(CP_UTF8,0,wtok,-1,tok,size,NULL,NULL)
#endif

static SDL_Surface *SDL_DisplayFormatAlpha(SDL_Surface * surface)
{
  SDL_Surface *tmp;

  tmp = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
  return (tmp);
}

struct osk_keyboard *osk_create(char *layout_name, SDL_Surface * canvas,
                                SDL_Surface * BLANK_button_up,
                                SDL_Surface * BLANK_button_down,
                                SDL_Surface * BLANK_button_off,
                                SDL_Surface * BLANK_button_nav,
                                SDL_Surface * BLANK_button_hold,
                                SDL_Surface * BLANK_oskdel,
                                SDL_Surface * BLANK_osktab,
                                SDL_Surface * BLANK_oskenter,
                                SDL_Surface * BLANK_oskcapslock, SDL_Surface * BLANK_oskshift, int disable_change)
{
  SDL_Surface *surface;
  SDL_Surface *button_up, *button_down;
  SDL_Surface *button_off, *button_nav;
  SDL_Surface *button_hold;
  SDL_Surface *oskdel, *osktab, *oskenter;
  SDL_Surface *oskcapslock, *oskshift;
  osk_layout *layout;
  on_screen_keyboard *keyboard;
  int layout_avail_width, layout_avail_height;

  keyboard = malloc(sizeof(on_screen_keyboard));

  keyboard->osk_fonty = NULL;

  keyboard->disable_change = disable_change;
  layout = load_layout(keyboard, layout_name);
  if (!layout)
  {
    fprintf(stderr, "Error trying to load the required layout %s\n", layout_name);
    layout = load_layout(keyboard, strdup("default.layout"));
    if (!layout)
    {
      fprintf(stderr, "Error trying to load the default layout\n");
      return NULL;
    }
    fprintf(stderr, "Loaded the default layout instead.\n");
  }

#ifdef OSK_DEBUG
  printf("w %i, h %i\n", layout->width, layout->height);
#endif

  layout_avail_width = (canvas->w * 0.9);
  layout_avail_height = (canvas->h * 0.5);

  if (layout->width * BLANK_button_up->w >= layout_avail_width ||       /* Don't allow it to be > 90% of the width of the canvas */
      layout->height * BLANK_button_up->h >= layout_avail_height
      /* Don't allow it to be > 50% of the height of the canvas */ )
  {
    /* Full-size buttons too large, resize to fit */
    float max_w, max_h;
    float scale_w, scale_h;

#ifdef OSK_DEBUG
    printf
      ("%d x %d layout of %d x %d buttons won't fit within %d x %d pixel area...\n",
       layout->width, layout->height, BLANK_button_up->w, BLANK_button_up->h, layout_avail_width, layout_avail_height);
#endif

    max_w = (float)layout_avail_width / (float)layout->width;
    max_h = (float)layout_avail_height / (float)layout->height;

#ifdef OSK_DEBUG
    printf("...want (%d / %d) x (%d x %d) = %.2f x %.2f buttons...\n",
           layout_avail_width, layout->width, layout_avail_height, layout->height, max_w, max_h);
#endif

    if (max_w > max_h)
      max_w = max_h;
    if (max_h > max_w)
      max_h = max_w;

    scale_w = (float)max_w / (float)BLANK_button_up->w;
    scale_h = (float)max_h / (float)BLANK_button_up->h;

#ifdef OSK_DEBUG
    printf("...so scaling by w=%.2f & h=%.2f\n", scale_w, scale_h);
#endif

    button_up = zoomSurface(BLANK_button_up, scale_w, scale_h, 1);
    button_down = zoomSurface(BLANK_button_down, scale_w, scale_h, 1);
    button_off = zoomSurface(BLANK_button_off, scale_w, scale_h, 1);
    button_nav = zoomSurface(BLANK_button_nav, scale_w, scale_h, 1);
    button_hold = zoomSurface(BLANK_button_hold, scale_w, scale_h, 1);
    oskdel = zoomSurface(BLANK_oskdel, scale_w, scale_h, 1);
    osktab = zoomSurface(BLANK_osktab, scale_w, scale_h, 1);
    oskenter = zoomSurface(BLANK_oskenter, scale_w, scale_h, 1);
    oskcapslock = zoomSurface(BLANK_oskcapslock, scale_w, scale_h, 1);
    oskshift = zoomSurface(BLANK_oskshift, scale_w, scale_h, 1);
  }
  else
  {
    button_up = SDL_DisplayFormatAlpha(BLANK_button_up);
    button_down = SDL_DisplayFormatAlpha(BLANK_button_down);
    button_off = SDL_DisplayFormatAlpha(BLANK_button_off);
    button_nav = SDL_DisplayFormatAlpha(BLANK_button_nav);
    button_hold = SDL_DisplayFormatAlpha(BLANK_button_hold);
    oskdel = SDL_DisplayFormatAlpha(BLANK_oskdel);
    osktab = SDL_DisplayFormatAlpha(BLANK_osktab);
    oskenter = SDL_DisplayFormatAlpha(BLANK_oskenter);
    oskcapslock = SDL_DisplayFormatAlpha(BLANK_oskcapslock);
    oskshift = SDL_DisplayFormatAlpha(BLANK_oskshift);
  }

  surface = SDL_CreateRGBSurface(canvas->flags,
                                 layout->width * button_up->w,
                                 layout->height * button_up->h,
                                 canvas->format->BitsPerPixel,
                                 canvas->format->Rmask, canvas->format->Gmask, canvas->format->Bmask, 0);
  if (!surface)
  {
    fprintf(stderr, "Error creating the onscreen keyboard surface\n");
    return NULL;
  }
  //  keyboard->name = layout_name;
  keyboard->canvas_ptr = canvas;
  keyboard->layout = layout;
  keyboard->surface = surface;
  keyboard->rect.x = 0;
  keyboard->rect.y = 0;
  keyboard->rect.w = keyboard->surface->w;
  keyboard->rect.h = keyboard->surface->h;
  keyboard->button_up = button_up;
  keyboard->button_down = button_down;
  keyboard->button_off = button_off;
  keyboard->button_nav = button_nav;
  keyboard->button_hold = button_hold;
  keyboard->oskdel = oskdel;
  keyboard->osktab = osktab;
  keyboard->oskenter = oskenter;
  keyboard->oskcapslock = oskcapslock;
  keyboard->oskshift = oskshift;
  keyboard->composing = layout->composemap;
  keyboard->composed = NULL;
  keyboard->last_key_pressed = NULL;
  keyboard->modifiers = 0;

  set_key(NULL, &keyboard->keymodifiers.shift, 1);
  set_key(NULL, &keyboard->keymodifiers.altgr, 1);
  set_key(NULL, &keyboard->keymodifiers.compose, 1);
  set_key(NULL, &keyboard->keymodifiers.dead, 1);

  keyboard->kmdf.shift = NULL;
  keyboard->kmdf.altgr = NULL;
  keyboard->kmdf.dead = NULL;
  keyboard->kmdf.dead2 = NULL;
  keyboard->kmdf.dead3 = NULL;
  keyboard->kmdf.dead4 = NULL;

  keyboard->BLANK_button_up = BLANK_button_up;
  keyboard->BLANK_button_down = BLANK_button_down;
  keyboard->BLANK_button_off = BLANK_button_off;
  keyboard->BLANK_button_nav = BLANK_button_nav;
  keyboard->BLANK_button_hold = BLANK_button_hold;
  keyboard->BLANK_oskdel = BLANK_oskdel;
  keyboard->BLANK_osktab = BLANK_osktab;
  keyboard->BLANK_oskenter = BLANK_oskenter;
  keyboard->BLANK_oskcapslock = BLANK_oskcapslock;
  keyboard->BLANK_oskshift = BLANK_oskshift;

  SDL_FillRect(surface, NULL,
               SDL_MapRGB(surface->format, keyboard->layout->bgcolor.r,
                          keyboard->layout->bgcolor.g, keyboard->layout->bgcolor.b));

  keybd_prepare(keyboard);

  draw_keyboard(keyboard);
  return keyboard;
}

static struct osk_layout *load_layout(on_screen_keyboard * keyboard, char *layout_name)
{
  FILE *fi;
  int hlayout_loaded;
  char *line;
  char *filename;
  char *key, *value;
  osk_layout *layout;
  char * __attribute__((unused)) tmp_ptr;

  layout = malloc(sizeof(osk_layout));
  layout->name = NULL;
  layout->rows = NULL;
  layout->width = 0;
  layout->height = 0;
  layout->fontpath = NULL;
  layout->keys = NULL;
  layout->keymap = NULL;
  layout->composemap = NULL;
  layout->keysymdefs = NULL;
  layout->sizeofkeysymdefs = 0;
  layout->bgcolor = def_bgcolor;
  layout->fgcolor = def_fgcolor;

  hlayout_loaded = 0;
#ifdef OSK_DEBUG
  printf("load_layout %s\n", layout_name);
#endif
  filename = malloc(sizeof(char) * 255);
  if (layout_name != NULL)
  {
    keyboard->name = strdup(layout_name);
    /* Try full path */
    fi = fopen(layout_name, "r");
    if (fi == NULL)
    {
      /* Try with DATA_PREFIX */

      snprintf(filename, 255, "%sosk/%s", DATA_PREFIX, layout_name);
      fi = fopen(filename, "r");
      if (fi == NULL)
      {
        fprintf(stderr, "Can't open either %s nor %s\n", layout_name, filename);
        /* Fallback to default */
        snprintf(filename, 255, "%sosk/default.layout", DATA_PREFIX);
        fi = fopen(filename, "r");
        keyboard->name = strdup("default.layout");
      }
    }
  }
  else
  {
    snprintf(filename, 255, "%sosk/default.layout", DATA_PREFIX);
    fi = fopen(filename, "r");
    keyboard->name = strdup("default.layout");
  }

  free(filename);
  if (fi == NULL)
  {
    fprintf(stderr, "Can't load the on screen keyboard layout\n");
    return NULL;
  }


  line = malloc(sizeof(char) * 1024);
  key = malloc(sizeof(char) * 255);
  value = malloc(sizeof(char) * 255);

  while (!feof(fi))
  {
    tmp_ptr = fgets(line, 1023, fi);

    if (is_blank_or_comment(line))
      continue;

    sscanf(line, "%s %s", key, value);
    if (strcmp("layout", key) == 0 && !hlayout_loaded)
    {
#ifdef OSK_DEBUG
      printf("layout found: %s\n", value);
#endif

      load_hlayout(layout, value);
      hlayout_loaded = 1;
    }
    else if (strncmp("keymap", key, 6) == 0)
    {
#ifdef OSK_DEBUG
      printf("keymap found: %s\n", value);
#endif
      load_keymap(layout, value);
    }
    else if (strncmp("composemap", key, 10) == 0)
    {
#ifdef OSK_DEBUG
      printf("composemap found: %s\n", value);
#endif
      load_composemap(layout, value);
    }
    else if (strncmp("keysymdefs", key, 10) == 0)
    {
      load_keysymdefs(layout, value);
    }
    else if (strncmp("keyboardlist", key, 12) == 0)
    {
      strcpy(value, &line[13]);
      keyboard->keyboard_list = strdup(value);
    }

#ifdef OSK_DEBUG
    printf("key %s, value %s\n", key, value);
#endif
    key[0] = '\0';
    value[0] = '\0';
  }



  free(key);
  free(value);
  free(line);
  fclose(fi);
  return layout;
}

/* A hlayout contains the definitions of the keyboard as seen in the screen.
   Things like the number of rows of the keyboard, the font used to render the keys,
   the width of the keys, and a code that matches each key like in real hardware keyboards */
void load_hlayout(osk_layout * layout, char *hlayout_name)
{
  int width, height;
  int key_number, line_number;
  int keycode, shiftcaps;
  int allocated, have_fontpath;
  int i;
  int r, g, b;
  int key_width, key_width_decimal;
  char *filename;
  char *line;
  char *key, *fontpath;
  char *plain_label, *top_label, *altgr_label, *shift_altgr_label;
  FILE *fi;
  char * __attribute__((unused)) tmp_ptr;

  key_number = line_number = 0;
  width = height = 0;
  allocated = 0;
  have_fontpath = 0;

  filename = malloc(sizeof(char) * 255);

  /* Try full path */
  fi = fopen(hlayout_name, "r");
  if (fi == NULL)
  {
    /* Try with DATA_PREFIX */

    snprintf(filename, 255, "%sosk/%s", DATA_PREFIX, hlayout_name);
    fi = fopen(filename, "r");
    if (fi == NULL)
    {
      fprintf(stderr, "Can't open either %s nor %s\n", hlayout_name, filename);
      layout->keys = NULL;
      free(filename);
      return;
    }
  }

  free(filename);

  line = malloc(sizeof(char) * 1024);
  key = malloc(sizeof(char) * 255);
  fontpath = malloc(sizeof(char) * 255);
  r = g = b = 256;

  layout->fgcolor.r = def_fgcolor.r;
  layout->fgcolor.g = def_fgcolor.g;
  layout->fgcolor.b = def_fgcolor.b;

  layout->bgcolor.r = def_bgcolor.r;
  layout->bgcolor.g = def_bgcolor.g;
  layout->bgcolor.b = def_bgcolor.b;


  while (!feof(fi))
  {
    if (width && height && !allocated)
    {
      layout->keys = malloc(height * sizeof(osk_key *));
      layout->keys[0] = malloc(width * sizeof(osk_key));

      for (i = 0; i < width; i++)
      {
        layout->keys[0][i].width = 0;
        layout->keys[0][i].plain_label = NULL;
        layout->keys[line_number][i].top_label = NULL;
        layout->keys[line_number][i].altgr_label = NULL;
        layout->keys[line_number][i].shift_altgr_label = NULL;
      }
      layout->width = width;
      layout->height = height;

#ifdef OSK_DEBUG
      printf("w %i, h %i\n", layout->width, layout->height);
#endif
      allocated = 1;
    }

    tmp_ptr = fgets(line, 1023, fi);

    if (is_blank_or_comment(line))
      continue;

    if (strncmp(line, "WIDTH", 5) == 0)
      sscanf(line, "%s %i", key, &width);

    else if (strncmp(line, "HEIGHT", 5) == 0)
      sscanf(line, "%s %i", key, &height);

    else if (strncmp(line, "FONTPATH", 8) == 0)
    {
#ifdef OSK_DEBUG
      printf("linefont %s\n", line);
#endif
      sscanf(line, "%s %s", key, fontpath);
      if (!is_blank_or_comment(fontpath))
        have_fontpath = 1;
    }
    else if (strncmp(line, "FGCOLOR", 5) == 0)
    {
#ifdef OSK_DEBUG
      printf("linefont %s\n", line);
#endif
      sscanf(line, "%s %i %i %i", key, &r, &g, &b);
      if (r > 0 && r < 256 && g > 0 && g < 256 && b > 0 && b < 256)
      {
        layout->fgcolor.r = r;
        layout->fgcolor.g = g;
        layout->fgcolor.b = b;
        r = g = b = 256;
      }
    }
    else if (strncmp(line, "BGCOLOR", 5) == 0)
    {
#ifdef OSK_DEBUG
      printf("linefont %s\n", line);
#endif
      sscanf(line, "%s %i %i %i", key, &r, &g, &b);
      if (r > 0 && r < 256 && g > 0 && g < 256 && b > 0 && b < 256)
      {
        layout->bgcolor.r = r;
        layout->bgcolor.g = g;
        layout->bgcolor.b = b;
        r = g = b = 256;
      }
    }
    else if (strncmp(line, "NEWLINE", 7) == 0)
    {
      line_number++;
      key_number = 0;
      layout->keys[line_number] = malloc(width * sizeof(osk_key));
      for (i = 0; i < width; i++)
      {
        layout->keys[line_number][i].width = 0;
        layout->keys[line_number][i].plain_label = NULL;
        layout->keys[line_number][i].top_label = NULL;
        layout->keys[line_number][i].altgr_label = NULL;
        layout->keys[line_number][i].shift_altgr_label = NULL;
      }
    }
    else if (width && height && allocated && strncmp(line, "KEY ", 4) == 0 && key_number < width)
    {
      plain_label = malloc(sizeof(char) * 64);
      top_label = malloc(sizeof(char) * 64);
      altgr_label = malloc(sizeof(char) * 64);
      shift_altgr_label = malloc(sizeof(char) * 64);

      sscanf(line,
             "%s %i %i.%i %s %s %s %s %i",
             key,
             &keycode,
             &key_width, &key_width_decimal, plain_label, top_label, altgr_label, shift_altgr_label, &shiftcaps);
      layout->keys[line_number][key_number].keycode = keycode;
      layout->keys[line_number][key_number].width = (float)0.1 *key_width_decimal + key_width;

      layout->keys[line_number][key_number].plain_label = plain_label;
      layout->keys[line_number][key_number].top_label = top_label;
      layout->keys[line_number][key_number].altgr_label = altgr_label;
      layout->keys[line_number][key_number].shift_altgr_label = shift_altgr_label;
      layout->keys[line_number][key_number].shiftcaps = shiftcaps;
      layout->keys[line_number][key_number].stick = 0;
      key_number++;
    }
  }

  if (have_fontpath)
    layout->fontpath = fontpath;
  else
  {
    free(fontpath);
    layout->fontpath = NULL;
  }

  free(line);
  free(key);
  fclose(fi);
  /* int j; */
  /* for(i = 0; i<= line_number; i++) */
  /* { */
  /*   printf("Line %i\n", i); */
  /*   for (j =0; j < width; j++) */
  /*   { */
  /*     printf("        %i,    \n ", j); */
  /*                 if(layout.keys[i][j].width) */
  /*    printf("keycode %d, width %f, plain %ls, caps %ls\n", */
  /*           layout.keys[i][j].keycode, */
  /*           layout.keys[i][j].width, */
  /*           layout.keys[i][j].plain_label, */
  /*           layout.keys[i][j].caps_label); */
  /*   } */
  /* } */
}


/* A keymap contains the keysyms (X keysym mnemonics) associated to each keycode in the hlayout.*/
void load_keymap(osk_layout * layout, char *keymap_name)
{
  int i, keycode, readed;
  char *filename;
  char *ksname1, *ksname2, *ksname3, *ksname4;
  char *line;
  FILE *fi;
  char * __attribute__((unused)) tmp_ptr;

  filename = malloc(sizeof(char) * 255);

  /* Try full path */
  fi = fopen(keymap_name, "r");
  if (fi == NULL)
  {
    /* Try with DATA_PREFIX */

    snprintf(filename, 255, "%sosk/%s", DATA_PREFIX, keymap_name);
    fi = fopen(filename, "r");
    if (fi == NULL)
    {
      fprintf(stderr, "Can't open either %s nor %s\n", keymap_name, filename);
      layout->keys = NULL;
      free(filename);
      return;
    }
  }

  free(filename);

  line = malloc(sizeof(char) * 1024);
  layout->keymap = malloc(256 * sizeof(osk_keymap));

  for (i = 0; i < 256; i++)
  {
    layout->keymap[i].plain = NULL;
    layout->keymap[i].caps = NULL;
    layout->keymap[i].altgr = NULL;
    layout->keymap[i].shiftaltgr = NULL;
  }


  while (!feof(fi))
  {
    tmp_ptr = fgets(line, 1023, fi);

    if (is_blank_or_comment(line))
      continue;

    ksname1 = malloc(sizeof(char) * 64);
    ksname2 = malloc(sizeof(char) * 64);
    ksname3 = malloc(sizeof(char) * 64);
    ksname4 = malloc(sizeof(char) * 64);
    ksname1[0] = '\0';
    ksname2[0] = '\0';
    ksname3[0] = '\0';
    ksname4[0] = '\0';

    /* FIXME: Why is the us-intl keymap duplicating the two first entries of every keycode? */
    /* And why is the arabic keymap using the 5th and 6th entries as plain/shifted keys? */
    readed = sscanf(line, "keycode %i = %s %s %s %s", &keycode, ksname1, ksname2, ksname3, ksname4);

    if (readed == 5 && keycode > 8 && keycode < 256)
    {
      layout->keymap[keycode].plain = ksname1;
      layout->keymap[keycode].caps = ksname2;
      layout->keymap[keycode].altgr = ksname3;
      layout->keymap[keycode].shiftaltgr = ksname4;
    }
    else
    {
      free(ksname1);
      free(ksname2);
      free(ksname3);
      free(ksname4);
      layout->keymap[keycode].plain = NULL;
      layout->keymap[keycode].caps = NULL;
      layout->keymap[keycode].altgr = NULL;
      layout->keymap[keycode].shiftaltgr = NULL;
    }
  }

  free(line);
  fclose(fi);
  /* int i; */
  /* for (i = 0; i < 256; i++) */
  /* { */
  /*   if (layout.keymap[i].plain) */
  /*     printf("%i,  %i, %i, %i, %i\n", i,   */
  /*         layout.keymap[i].plain, */
  /*         layout.keymap[i].caps, */
  /*         layout.keymap[i].altgr, */
  /*         layout.keymap[i].shiftaltgr); */
  /* } */



}

/* Scans a line of keysyms and result and classifies them. */
static void gettokens(char *line, char *delim, char **pointer, osk_composenode * composenode, osk_layout * layout)
{
  int i;
  char *tok;
  wchar_t *result, *wtok;
  osk_composenode *auxnode;

  wtok = malloc(sizeof(wchar_t) * 255);

  tok = strdup(strtok_r(line, delim, pointer));

  if (!tok)
    return;

  if (tok[0] == ':')            /* End of precompose keysyms, next will be the result in UTF-8. */
  {
    free(tok);
    tok = strdup(strtok_r(line, ": \"\t", pointer));

    mbstowcs(wtok, tok, 255);

    result = wcsdup(wtok);
    /* printf("->%ls<-\n", wtok); */
    free(wtok);
    free(tok);
    composenode->result = result;
    return;
  }
  else
  {
    if (composenode->size == 0)
    {
      composenode->size = 1;
      auxnode = malloc(sizeof(osk_composenode));
      composenode->childs = malloc(sizeof(osk_composenode *));
      composenode->childs[0] = auxnode;
      mbstowcs(wtok, tok, 254); /* <<< CRASH */
      composenode->childs[0]->keysym = wcsdup(wtok);
      composenode->childs[0]->result = NULL;
      composenode->childs[0]->size = 0;

      /* printf("size %d, keysym %ls => ", composenode->size, composenode->childs[0]->keysym); */

      gettokens(NULL, delim, pointer, composenode->childs[0], layout);
      free(wtok);
      free(tok);
      return;
    }
    else
    {
      for (i = 0; i < composenode->size; i++)
      {
        mbstowcs(wtok, tok, 255);
        if (wcscmp(composenode->childs[i]->keysym, wtok) == 0)
        {

          /* printf("Size %d, keysym %ls =>", composenode->size, composenode->childs[i]->keysym); */

          gettokens(NULL, delim, pointer, composenode->childs[i], layout);
          free(tok);
          free(wtok);
          return;
        }
      }
    }

    composenode->size = composenode->size + 1;
    composenode->childs = realloc(composenode->childs, composenode->size * sizeof(osk_composenode *));

    mbstowcs(wtok, tok, 255);
    auxnode = malloc(sizeof(osk_composenode));
    composenode->childs[composenode->size - 1] = auxnode;       //malloc(sizeof(osk_composenode));
    composenode->childs[composenode->size - 1]->keysym = wtok;
    composenode->childs[composenode->size - 1]->result = NULL;
    composenode->childs[composenode->size - 1]->size = 0;

    /* printf("size %d, keysym %ls =>", composenode->size, composenode->childs[composenode->size - 1]->keysym); */

    gettokens(NULL, delim, pointer, composenode->childs[composenode->size - 1], layout);
    free(tok);
    return;
  }
}


/* A compose map contains the sequences of keysyms (X keysym mnemonics) needed to generate another keysym.
   The last in the sequence is the result, the others will be searched in the order they appear. 
   They will be classified in a multiway tree.*/
static void load_composemap(osk_layout * layout, char *composemap_name)
{
  char *filename;
  char **pointer;
  char *line;
  FILE *fi;
  char * __attribute__((unused)) tmp_ptr;

  pointer = malloc(sizeof(wchar_t *));
  filename = malloc(sizeof(char) * 255);

  /* Try full path */
  fi = fopen(composemap_name, "r");
  if (fi == NULL)
  {
    /* Try with DATA_PREFIX */

    snprintf(filename, 255, "%sosk/%s", DATA_PREFIX, composemap_name);
    fi = fopen(filename, "r");
    if (fi == NULL)
    {
      fprintf(stderr, "Can't open either %s nor %s\n", composemap_name, filename);
      layout->keys = NULL;
      free(filename);
      return;
    }
  }

  free(filename);

  layout->composemap = malloc(sizeof(osk_composenode));
  layout->composemap[0].keysym = NULL;
  layout->composemap[0].result = NULL;

  layout->composemap->size = 0;
  line = malloc(1024 * sizeof(char));

  while (!feof(fi))
  {
    tmp_ptr = fgets(line, 1023, fi);

    if (is_blank_or_comment(line))
      continue;

    gettokens(line, (char *)">< \t", pointer, layout->composemap, layout);
  }

  fclose(fi);
  free(line);
  free(pointer);
#ifdef DEBUG_OSK_COMPOSEMAP
  print_composemap(layout->composemap, NULL);
#endif
}

#ifdef DEBUG_OSK_COMPOSEMAP
static void print_composemap(osk_composenode * composemap, char *sp)
{
  int i;
  char *space;

  space = malloc(sizeof(char) * 255);

#ifdef OSK_DEBUG
  printf("%ls, ", composemap->keysym);
  printf("%d ==> ", composemap->size);
#endif
  if (composemap->size == 0)
  {
#ifdef OSK_DEBUG
    printf("result %ls\n", composemap->result);
#endif
    return;
  }
  if (sp)
  {
    sprintf(space, "%s\t", sp);
  }
  else
  {
    sprintf(space, " ");
  }
#ifdef OSK_DEBUG
  printf("%s", space);
#endif

  for (i = 0; i < composemap->size; i++)
  {
    print_composemap(composemap->childs[i], space);
    //      free(space);
  }
  /* for (i = 0; i < composemap->size; i++) */
  /*   { */
  /*     printf("aaa %ls, ", composemap->keysym); */
  /*     printf("%d ==> ", composemap->size); */
  /*     printf("%ls, ", composemap->childs[i]->keysym); */
  /*     printf("childs %d ==> ", composemap->childs[i]->size); */

  /*     if (composemap->childs[i]->size == 0) */
  /*            printf("result %ls\n", composemap->childs[i]->result); */
  /*     else */
  /*            print_composemap(composemap->childs[i], space); */
  /*     //      free(space); */
  /*   } */
}
#endif

/* This parses the contents of keysymdef.h from the source of xorg.
   Therefore, if somebody wants to provide custom keysymdefs, he has to follow its syntax. */
static void load_keysymdefs(osk_layout * layout, char *keysymdefs_name)
{
  int i;
  char *filename;
  char *line;
  FILE *fi;
  char * __attribute__((unused)) tmp_ptr;

  filename = malloc(sizeof(char) * 255);

  /* Try full path */
  fi = fopen(keysymdefs_name, "r");
  if (fi == NULL)
  {
    /* Try with DATA_PREFIX */

    snprintf(filename, 255, "%sosk/%s", DATA_PREFIX, keysymdefs_name);
    fi = fopen(filename, "r");
    if (fi == NULL)
    {
      fprintf(stderr, "Can't open either %s nor %s\n", keysymdefs_name, filename);
      layout->keysymdefs = NULL;
      free(filename);
      return;
    }
  }

  free(filename);

  layout->keysymdefs = malloc(sizeof(keysymdefs));
  layout->keysymdefs[0].unicode = 0;
  i = 0;
  line = malloc(1024 * sizeof(wchar_t));

  while (!feof(fi))
  {
    tmp_ptr = fgets(line, 1023, fi);
    if (strncmp("#define XK_", line, 11) != 0)
      continue;

    layout->sizeofkeysymdefs = i;
    layout->keysymdefs = realloc(layout->keysymdefs, sizeof(keysymdefs) * (i + 1));

    /* Some keysyms doesn't correspond to any unicode value, ej. BackSpace */
    layout->keysymdefs[i].unicode = 0;
    layout->keysymdefs[i].mnemo = malloc(sizeof(char) * 128);
    sscanf(line, "#define XK_%s %x /* U+%x",
           layout->keysymdefs[i].mnemo, &layout->keysymdefs[i].keysym, &layout->keysymdefs[i].unicode);
    i++;
  }

  fclose(fi);
  free(line);
}

/* /\* Return the mnemonic string of a x keysym as defined in the source of xorg in keysymdef.h *\/ */
/* static char * keysym2mnemo(int keysym, on_screen_keyboard * keyboard) */
/* { */
/*   unsigned int i; */
/*   for (i = 0; i < keyboard->layout->sizeofkeysymdefs ;i++) */
/*     if (keysym == keyboard->layout->keysymdefs[i].keysym) */
/*       return(keyboard->layout->keysymdefs[i].mnemo); */

/*   /\* For the purpose of onscreen keyboard we don't need the conversion to strings in the form U0000 *\/ */
/*   return(NULL); */
/* } */

/* Returns the x keysym corresponding to a mnemonic string */
static int mnemo2keysym(char *mnemo, on_screen_keyboard * keyboard)
{
  unsigned int i;

  for (i = 0; i < keyboard->layout->sizeofkeysymdefs; i++)
  {
    if (strcmp(mnemo, keyboard->layout->keysymdefs[i].mnemo) == 0)
      return (keyboard->layout->keysymdefs[i].keysym);
  }
  i = 0;

  /* Perhaps the mnemo is in UXXXX format? */
  if (sscanf(mnemo, "U%x", &i))
    return (i | 0x01000000);

  /* Or maybe mnemo is already a keysym? */
  if (sscanf(mnemo, "0x%x", &i))
    return (i);

  return (0);
}

/* Returns the unicode value of a x keysym if any, otherwise returns 0 */
static int keysym2unicode(int keysym, on_screen_keyboard * keyboard)
{
  unsigned int i;

  /* Credits for the conversion from xkeysyms to unicode values, code taken from the source code of xterm, file keysym2ucs.c.
   *Author: Markus G. Kuhn <mkuhn@acm.org>, University of Cambridge, April 2001
   *
   * Special thanks to Richard Verhoeven <river@win.tue.nl> for preparing
   * an initial draft of the mapping table.
   * 
   * This software is in the public domain. Share and enjoy!
   */
  /* first check for Latin-1 characters (1:1 mapping) */
  if ((keysym >= 0x0020 && keysym <= 0x007e) || (keysym >= 0x00a0 && keysym <= 0x00ff))
    return keysym;

  /* also check for directly encoded 24-bit UCS characters */
  if ((keysym & 0xff000000) == 0x01000000)
    return keysym & 0x00ffffff;



  for (i = 0; i < keyboard->layout->sizeofkeysymdefs; i++)
    if (keysym == keyboard->layout->keysymdefs[i].keysym)
      return (keyboard->layout->keysymdefs[i].unicode);

  return (keysym);
}


/* Searches in the tree for composing stuff */
static void get_composed_keysym(on_screen_keyboard * keyboard, osk_composenode * composenode, wchar_t *keysym)
{
  int i;

  /* If there is not a compose table return the keysym */
  if (!composenode)
  {
    if (keyboard->composed)
      free(keyboard->composed);
    keyboard->composed = wcsdup(keysym);
    keyboard->composed_type = 0;
    return;
  }

  /* If there is a compose table, lookup for matches */
  for (i = 0; i < composenode->size; i++)
  {
    /* If matches, set either the result or the next node */
    if (wcscmp(composenode->childs[i]->keysym, keysym) == 0)
    {
      if (composenode->childs[i]->result)
      {
        if (keyboard->composed)
          free(keyboard->composed);
        keyboard->composed = wcsdup(composenode->childs[i]->result);
        keyboard->composing = keyboard->layout->composemap;
        /* The result in the Compose files from xorg is yet in unicode */
        keyboard->composed_type = 1;
        return;
      }
      else
      {
        if (keyboard->composed)
          free(keyboard->composed);
        keyboard->composed = NULL;
        keyboard->composing = composenode->childs[i];
        return;
      }
    }
  }

  /* No matches found, if we were in the middle of a sequence, reset the compose stuff,
     if we were in the beginning node, set the keysym */

  if (keyboard->layout->composemap == composenode)
  {
    if (keyboard->composed)
      free(keyboard->composed);
    keyboard->composed = wcsdup(keysym);
    keyboard->composed_type = 0;
  }
  else                          /* reset */
  {
    keyboard->composing = keyboard->layout->composemap;
    if (keyboard->composed)
      free(keyboard->composed);
    keyboard->composed = NULL;
    keyboard->composed_type = 0;
  }
}


static int is_blank_or_comment(char *line)
{
  int i;

  i = 0;

  if (strlen(line) == 0)
    return 0;
  while (line[i] != '\n')
  {
    if (line[i] == '#')
      return 1;
    else if (line[i] == ' ' || line[i] == '\t')
      i++;
    else
      return 0;
  }
  return 1;
}


/* static int isw_blank_or_comment(wchar_t *line) */
/* { */
/*   int i; */

/*   i = 0; */
/*   if (wcslen(line) == 0) */
/*     return 0; */
/*   while (line[i] != L'\n') */
/*   { */
/*     if (line[i] == L'#') */
/*       return 1; */
/*     else if (line[i] == L' ' || line[i] == L'\t') */
/*       i++; */
/*     else */
/*       return 0; */
/*   } */
/*   return 1; */
/* } */


/* FIXME: Is it safe to supose that if a font is loaded at one size, it will be loaded at any size? */
/* FIXME: starting a layout with one font causes all other layouts be in that font */
static void keybd_prepare(on_screen_keyboard * keyboard)
{
  char *fontname;
  int font_height;

  /* Pick a height (e.g., 16pt for small (24x24), 32pt for large (48x48) buttons) */
  font_height = ((keyboard->button_up->h * 2) / 3);

  fontname = malloc(sizeof(char) * 255);
  if (keyboard->osk_fonty == NULL)
  {

    if (keyboard->layout->fontpath)
    {
      /* First try if it is an absolute path */
      keyboard->osk_fonty = TTF_OpenFont(keyboard->layout->fontpath, font_height);
      if (keyboard->osk_fonty == NULL)
      {
        /* Now trying if it is relative to DATA_PREFIX/fonts/ */
        snprintf(fontname, 255, "%s/fonts/%s", DATA_PREFIX, keyboard->layout->fontpath);

        keyboard->osk_fonty = TTF_OpenFont(fontname, font_height);
        if (keyboard->osk_fonty == NULL)
        {
          /* Perhaps it is relative to DATA_PREFIX only? */
          snprintf(fontname, 255, "%s/%s", DATA_PREFIX, keyboard->layout->fontpath);
          keyboard->osk_fonty = TTF_OpenFont(fontname, font_height);
          if (keyboard->osk_fonty == NULL)
          {
            /* Or to DATA_PREFIX/fonts/locale/ ? */
            snprintf(fontname, 255, "%s/fonts/locale/%s", DATA_PREFIX, keyboard->layout->fontpath);
            keyboard->osk_fonty = TTF_OpenFont(fontname, font_height);
            if (keyboard->osk_fonty == NULL)
            {
              /* Fonts are in assets "data" dir in Android builds */
              snprintf(fontname, 255, "data/fonts/%s", keyboard->layout->fontpath);
              keyboard->osk_fonty = TTF_OpenFont(fontname, font_height);
              if (keyboard->osk_fonty == NULL)
              {
                /* Fonts are in assets "data" dir in Android builds, checking locale dir */
                snprintf(fontname, 255, "data/fonts/locale/%s", keyboard->layout->fontpath);
                keyboard->osk_fonty = TTF_OpenFont(fontname, font_height);
              }
            }
          }
        }
      }
    }

    if (keyboard->osk_fonty == NULL)
    {
      /* Going with the default */
      sprintf(fontname, "%s/fonts/FreeSansBold.ttf", DATA_PREFIX);
      keyboard->osk_fonty = TTF_OpenFont(fontname, font_height);

      if (keyboard->osk_fonty == NULL)
      {
        /* Also for Android */
        sprintf(fontname, "data/fonts/FreeSansBold.ttf");
        keyboard->osk_fonty = TTF_OpenFont(fontname, font_height);
      }
    }

    if (keyboard->osk_fonty == NULL)
    {
      fprintf(stderr, "\nError: Can't open the font!\n"
              "The Simple DirectMedia Layer error that occurred was:\n" "%s\n\n", SDL_GetError());
      free(fontname);
      exit(1);
    }

    free(fontname);
  }
}


static void apply_surface(int x, int y, SDL_Surface * source, SDL_Surface * destination, SDL_Rect * clip)
{
  SDL_Rect offset;

  offset.x = x;
  offset.y = y;

  SDL_BlitSurface(source, clip, destination, &offset);
}


/* /\* NOTE: This is a duplicate of wcstou16 in tuxpaint.c */

/*    This conversion is required on platforms where Uint16 doesn't match wchar_t. */
/*    On Windows, wchar_t is 16-bit, elsewhere it is 32-bit. */
/*    Mismatch caused by the use of Uint16 for unicode characters by SDL, SDL_ttf. */
/*    I guess wchar_t is really only suitable for internal use ... *\/ */
/* static Uint16 *wcstou16(const wchar_t * str) */
/* { */
/*   unsigned int i, len = wcslen(str); */
/*   Uint16 *res = malloc((len + 1) * sizeof(Uint16)); */

/*   for (i = 0; i < len + 1; ++i) */
/*   { */
/*     /\* This is a bodge, but it seems unlikely that a case-conversion */
/*        will cause a change from one utf16 character into two.... */
/*        (though at least UTF-8 suffers from this problem) *\/ */

/*     // FIXME: mangles non-BMP characters rather than using UTF-16 surrogates! */
/*     res[i] = (Uint16) str[i]; */
/*   } */

/*   return res; */
/* } */

/* Stretches a button from the middle, keeping the extrems intact */
static SDL_Surface *stretch_surface(SDL_Surface * orig, int width)
{
  int i;
  SDL_Surface *dest;
  SDL_Rect rect;
  SDL_Rect orig_rect;

  orig_rect.x = orig->w / 2;
  orig_rect.y = 0;
  orig_rect.w = 1;
  orig_rect.h = orig->h;

  dest = SDL_CreateRGBSurface(orig->flags,
                              width,
                              orig->h,
                              orig->format->BitsPerPixel,
                              orig->format->Rmask, orig->format->Gmask, orig->format->Bmask, 0);

  SDL_BlitSurface(orig, NULL, dest, NULL);
  rect.y = 0;

  if (width > orig->w)
  {
    rect.x = width - orig->w;
    rect.h = orig->h;
    rect.w = orig->w;
    SDL_BlitSurface(orig, NULL, dest, &rect);

    rect.w = 1;
    for (i = orig->w / 2; i < width - orig->w / 2; i++)
    {
      rect.x = i;
      SDL_BlitSurface(orig, &orig_rect, dest, &rect);
    }
  }
  else if (width < orig->w)
  {
    rect.y = 0;
    rect.w = 1;
    rect.h = dest->h;

    orig_rect.y = 0;
    orig_rect.w = 1;
    orig_rect.h = orig->h;

    for (i = 0; i <= width / 2; i++)
    {
      rect.x = dest->w - i;
      orig_rect.x = orig->w - i;
      SDL_BlitSurface(orig, &orig_rect, dest, &rect);
    }
  }

  return dest;
}

/* Draws the keyboard surface */
static void draw_keyboard(on_screen_keyboard * keyboard)
{
  int i, j;
  int key_height, accumulated_width, accumulated_height;
  float key_width;

  key_width = keyboard->button_up->w;
  key_height = keyboard->button_up->h;

  accumulated_height = 0;

  for (j = 0; j < keyboard->layout->height; j++)
  {
    accumulated_width = 0;
    for (i = 0; i < keyboard->layout->width; i++)
    {
      if (keyboard->layout->keys[j][i].width)
      {

        keyboard->layout->keys[j][i].row = j;
        keyboard->layout->keys[j][i].x = accumulated_width;
        keyboard->layout->keys[j][i].y = accumulated_height;

        draw_key(keyboard->layout->keys[j][i], keyboard, 0);
      }
      accumulated_width += (keyboard->layout->keys[j][i].width * key_width);
    }
    accumulated_height += key_height;
  }

  /* draw_key(keyboard->keymodifiers.shift, keyboard, 0); */
  /* draw_key(keyboard->keymodifiers.altgr, keyboard, 0); */
  /* draw_key(keyboard->keymodifiers.compose, keyboard, 0); */
  /* draw_key(keyboard->keymodifiers.dead, keyboard, 0); */
}

static void draw_key(osk_key key, on_screen_keyboard * keyboard, int hot)
{
  char *text;
  SDL_Surface *skey;

  if (!key.width)
    return;

  text = malloc(sizeof(char) * 255);

  snprintf(text, 6, "%s", key.plain_label);

  if (strncmp("NULL", text, 4) != 0 && key.keycode != 0)
  {
    if (hot)
      skey = stretch_surface(keyboard->button_down, key.width * keyboard->button_down->w);

    else if (key.stick)
      skey = stretch_surface(keyboard->button_hold, key.width * keyboard->button_hold->w);

    else
    {
      if (key.keycode == 1 || key.keycode == 2)
      {
        if (keyboard->disable_change)
          skey = stretch_surface(keyboard->button_off, key.width * keyboard->button_off->w);
        else
          skey = stretch_surface(keyboard->button_nav, key.width * keyboard->button_nav->w);
      }
      else
        skey = stretch_surface(keyboard->button_up, key.width * keyboard->button_up->w);
    }
  }
  else
    skey = stretch_surface(keyboard->button_off, key.width * keyboard->button_off->w);

  apply_surface(key.x, key.y, skey, keyboard->surface, NULL);

  SDL_FreeSurface(skey);
  free(text);
  label_key(key, keyboard);
}


/* FIXME: TODO draw top and bottom_right (altgr) labels */
static void label_key(osk_key key, on_screen_keyboard * keyboard)
{
  SDL_Surface *messager;
  int modstate;
  char *text;

  /* To remove a warning... */
  text = NULL;

  modstate = keyboard->modifiers;

  /* FIXME There MUST be a simpler way to do this. Pere 2011/8/3 */
  /* First the plain ones */
  if (modstate == KMOD_NONE || (modstate == (KMOD_NONE | KMOD_LALT)))
    text = strdup(key.plain_label);

  else if (modstate == KMOD_SHIFT)
  {
    text = strdup(key.top_label);
  }

  else if (modstate == KMOD_RALT)
  {
    text = strdup(key.altgr_label);
  }

  else if (modstate == KMOD_CAPS)
  {
    if (key.shiftcaps == 1)
      text = strdup(key.top_label);

    else
      text = strdup(key.plain_label);
  }

  /* Now the combined ones */
  else if (modstate & KMOD_RALT && modstate & KMOD_SHIFT)
  {
    if (modstate & KMOD_CAPS)
    {
      if (key.shiftcaps)
        text = strdup(key.altgr_label);
      else
        text = strdup(key.shift_altgr_label);
    }
    else
    {
      text = strdup(key.shift_altgr_label);
    }
  }

  else if (modstate & KMOD_RALT && modstate & KMOD_CAPS && !(modstate & KMOD_SHIFT))
  {
    if (key.shiftcaps)
      text = strdup(key.shift_altgr_label);
    else
      text = strdup(key.altgr_label);
  }

  else if (modstate & KMOD_SHIFT && modstate & KMOD_CAPS)
  {
    if (key.shiftcaps == 1)
      text = strdup(key.plain_label);
    else
      text = strdup(key.top_label);
  }

  if (strncmp("DELETE", text, 6) == 0)
  {
    apply_surface(key.x, key.y, keyboard->oskdel, keyboard->surface, NULL);
  }

  else if (strncmp("TAB", text, 3) == 0)
  {
    apply_surface(key.x, key.y, keyboard->osktab, keyboard->surface, NULL);
  }

  else if (strncmp("ENTER", text, 5) == 0)
  {
    apply_surface(key.x, key.y, keyboard->oskenter, keyboard->surface, NULL);
  }

  else if (strncmp("CAPSLOCK", text, 8) == 0)
  {
    apply_surface(key.x, key.y, keyboard->oskcapslock, keyboard->surface, NULL);
  }

  else if (strncmp("SHIFT", text, 5) == 0)
  {
    apply_surface(key.x, key.y, keyboard->oskshift, keyboard->surface, NULL);
  }

  else if (strncmp("SPACE", text, 5) != 0 && strncmp("NULL", text, 4) != 0)
  {
    messager = TTF_RenderUTF8_Blended(keyboard->osk_fonty, text, keyboard->layout->fgcolor);

    apply_surface(key.x + 5, key.y, messager, keyboard->surface, NULL);
    SDL_FreeSurface(messager);
  }
  free(text);
}

/* Searches the key corresponding to coordinates */
static osk_key *find_key(on_screen_keyboard * keyboard, int x, int y)
{
  int i, j;
  osk_key *key;

  key = NULL;
  for (j = 0; j < keyboard->layout->height; j++)
  {
    if (keyboard->layout->keys[j][0].y < y && keyboard->layout->keys[j][0].y + keyboard->button_up->h > y)
      for (i = 0; i < keyboard->layout->width; i++)
        if (keyboard->layout->keys[j][i].x < x &&
            keyboard->layout->keys[j][i].x + keyboard->layout->keys[j][i].width * keyboard->button_up->w > x)
        {
          key = &keyboard->layout->keys[j][i];
          return key;
        }
  }

  return NULL;
}

/* Copies orig to dest or sets dest to defaults if orig is NULL.
   if firstime is setted, don't frees the strings as there aren't. */
static void set_key(osk_key * orig, osk_key * dest, int firsttime)
{
  if (orig == NULL)
  {
    dest->keycode = 0;
    dest->row = 0;
    dest->x = 0;
    dest->y = 0;
    dest->width = 0;
    if (!firsttime && dest->plain_label != NULL)
      free(dest->plain_label);
    dest->plain_label = NULL;
    if (!firsttime && dest->top_label != NULL)
      free(dest->top_label);
    dest->top_label = NULL;
    if (!firsttime && dest->altgr_label != NULL)
      free(dest->altgr_label);
    dest->altgr_label = NULL;
    dest->shiftcaps = 0;
  }
  else
  {
    dest->keycode = orig->keycode;
    dest->row = orig->row;
    dest->x = orig->x;
    dest->y = orig->y;
    dest->width = orig->width;

    if (dest->plain_label != NULL)
      free(dest->plain_label);
    dest->plain_label = strdup(orig->plain_label);

    if (dest->top_label != NULL)
      free(dest->top_label);
    dest->top_label = strdup(orig->top_label);

    if (dest->altgr_label != NULL)
      free(dest->altgr_label);
    dest->altgr_label = strdup(orig->altgr_label);

    dest->shiftcaps = orig->shiftcaps;
  }
}

static char *find_keysym(osk_key key, on_screen_keyboard * keyboard)
{
  int keycode;
  char *keysym;
  osk_keymap keysyms;
  SDL_Keymod modstate;

  keycode = key.keycode;
  keysyms = keyboard->layout->keymap[keycode];
  keysym = NULL;

  modstate = keyboard->modifiers;

  /* FIXME There MUST be a simpler way to do this. Pere 2011/8/3 */
  /* First the plain ones */
  if (modstate == KMOD_NONE || (modstate == (KMOD_NONE | KMOD_LALT)))
    keysym = keysyms.plain;

  else if (modstate == KMOD_SHIFT)
  {
    keysym = keysyms.caps;
  }

  else if (modstate == KMOD_RALT)
  {
    keysym = keysyms.altgr;
  }

  else if (modstate == KMOD_CAPS)
  {
    if (key.shiftcaps == 1)
      keysym = keysyms.caps;
    else
      keysym = keysyms.plain;
  }

  /* Now the combined ones */
  else if (modstate & KMOD_RALT && modstate & KMOD_SHIFT)
  {
    if (modstate & KMOD_CAPS)
    {
      if (key.shiftcaps)
        keysym = keysyms.altgr;
      else
        keysym = keysyms.shiftaltgr;
    }
    else
    {
      keysym = keysyms.shiftaltgr;
    }
  }

  else if (modstate & KMOD_RALT && modstate & KMOD_CAPS && !(modstate & KMOD_SHIFT))
  {
    if (key.shiftcaps)
      keysym = keysyms.shiftaltgr;
    else
      keysym = keysyms.altgr;
  }

  else if (modstate & KMOD_SHIFT && modstate & KMOD_CAPS)
  {
    if (key.shiftcaps == 1)
      keysym = keysyms.plain;
    else
      keysym = keysyms.caps;
  }

  return (keysym);
}

/* We lose the SDL ModState by leaving and entering the tuxpaint window, so using a custom state */
static int handle_keymods(char *keysym, osk_key * key, on_screen_keyboard * keyboard)
{
  SDL_Keymod mod;
  SDL_Event ev;

  mod = keyboard->modifiers;

  if (strncmp("Shift", keysym, 5) == 0)
  {
    if (mod & KMOD_SHIFT)
    {
      keyboard->modifiers = mod & 0xFFF0;
      key->stick = 0;
      keyboard->kmdf.shift->stick = 0;
    }
    else
    {
      keyboard->modifiers = mod | KMOD_SHIFT;
      key->stick = 1;
      keyboard->kmdf.shift = key;
    }
    return 1;
  }
  else if (strncmp("Alt_L", keysym, 5) == 0)
  {
    ev.key.keysym.sym = SDLK_LALT;
    ev.text.text[0] = 0;        // FIXME is 0 the right value here?
    ev.type = SDL_KEYDOWN;
    SDL_PushEvent(&ev);
    ev.type = SDL_KEYUP;
    SDL_PushEvent(&ev);

    return 1;
  }

  /* Seems ISO_Level3_Shift and ISO_Next_Group are used too for right Alt */
  else if (strncmp("ISO_Level3_Shift", keysym, 16) == 0 ||
           strncmp("ISO_Next_Group", keysym, 14) == 0 || strncmp("ALT_R", keysym, 5) == 0)
  {
    if (mod & KMOD_RALT)
    {
      keyboard->modifiers = mod & 0xF0FF;
      keyboard->kmdf.altgr->stick = 0;
    }
    else
    {
      keyboard->modifiers = mod | KMOD_RALT;
      key->stick = 1;
      keyboard->kmdf.altgr = key;

      return 1;
    }
    return 0;
  }

  else if (strncmp("Caps_Lock", keysym, 9) == 0)
  {
    if (mod & KMOD_CAPS)
    {
      keyboard->modifiers = mod & 0x0FFF;
      key->stick = 0;
    }
    else
    {
      keyboard->modifiers = mod | KMOD_CAPS;
      key->stick = 1;
    }


    return 1;
  }

  if (mod & KMOD_CAPS)
  {
    keyboard->modifiers = KMOD_CAPS;
  }
  else
    keyboard->modifiers = KMOD_NONE;

  if (keyboard->kmdf.shift)
    keyboard->kmdf.shift->stick = 0;
  if (keyboard->kmdf.altgr)
    keyboard->kmdf.altgr->stick = 0;

  return 0;
}

/* set_dead_sticks and clear_dead_sticks deals with the persistence of
   the keys that are still affecting other key presses. */
static void set_dead_sticks(osk_key * key, on_screen_keyboard * keyboard)
{
  key->stick = 1;
  if (!keyboard->kmdf.dead)
    keyboard->kmdf.dead = key;
  else if (!keyboard->kmdf.dead2)
    keyboard->kmdf.dead2 = key;
  else if (!keyboard->kmdf.dead3)
    keyboard->kmdf.dead3 = key;
  else if (!keyboard->kmdf.dead4)
    keyboard->kmdf.dead4 = key;
}

static void clear_dead_sticks(on_screen_keyboard * keyboard)
{
  if (keyboard->kmdf.dead)
  {
    keyboard->kmdf.dead->stick = 0;
    keyboard->kmdf.dead = NULL;
  }
  if (keyboard->kmdf.dead2)
  {
    keyboard->kmdf.dead2->stick = 0;
    keyboard->kmdf.dead2 = NULL;
  }
  if (keyboard->kmdf.dead3)
  {
    keyboard->kmdf.dead3->stick = 0;
    keyboard->kmdf.dead3 = NULL;
  }
  if (keyboard->kmdf.dead4)
  {
    keyboard->kmdf.dead4->stick = 0;
    keyboard->kmdf.dead4 = NULL;
  }
}

struct osk_keyboard *osk_clicked(on_screen_keyboard * keyboard, int x, int y)
{
  int i;
  osk_key *key;
  SDL_Event event;
  char *keysym, *mnemo;
  char *name, *aux_name, *aux_list, *aux_list_ptr;
  wchar_t *wkeysym;
  wchar_t *ks;
  on_screen_keyboard *new_keyboard;

#ifdef OSK_DEBUG
  printf("list: %s\n", keyboard->keyboard_list);
#endif

  event.key.keysym.mod = KMOD_NONE;
  event.key.keysym.sym = 0;
  event.text.text[0] = 0;

  key = find_key(keyboard, x, y);

  if (key)
  {
    /* First the reserved keycodes */
    /* Select next or previous keyboard */
    if (key->keycode == 1 || key->keycode == 2)
    {
      if (keyboard->disable_change)
      {
        //      free(key);
        return (keyboard);
      }

      aux_list = strdup(keyboard->keyboard_list);
      aux_list_ptr = aux_list;

#ifdef OSK_DEBUG
      printf("auxlist: %s\n", aux_list);
      printf("kn %s\n", keyboard->name);
#endif

      if (key->keycode == 1)
      {
        for (i = 0;; i++, aux_list = NULL)
        {
          name = strtok(aux_list, " \n\r\t");

          if (i == 0)
            aux_name = name;

          if (strcmp(name, keyboard->name) == 0)
          {
            name = strtok(NULL, " \n\r\t");
            if (name == NULL)
              name = aux_name;
            break;
          }
        }
      }
      else
      {
        aux_name = NULL;
        for (i = 0;; i++, aux_list = NULL)
        {
          name = strtok(aux_list, " \n\r\t");

          if (name == NULL)
          {
            name = aux_name;
            break;
          }

          if (strstr(name, keyboard->name))
          {
            name = aux_name;
            if (name != NULL)
              break;
          }

          aux_name = name;
        }
      }


      new_keyboard =
        osk_create(name, keyboard->canvas_ptr,
                   keyboard->BLANK_button_up, keyboard->BLANK_button_down,
                   keyboard->BLANK_button_off, keyboard->BLANK_button_nav,
                   keyboard->BLANK_button_hold,
                   keyboard->BLANK_oskdel, keyboard->BLANK_osktab,
                   keyboard->BLANK_oskenter, keyboard->BLANK_oskcapslock,
                   keyboard->BLANK_oskshift, keyboard->disable_change);

      free(aux_list_ptr);

      if (new_keyboard == NULL)
      {
        //      free(key);
        return (keyboard);      /* Don't break here, at least the old keyboard should work */
      }
      else
      {
        free(new_keyboard->keyboard_list);
        new_keyboard->keyboard_list = strdup(keyboard->keyboard_list);
        //      free(key);
        osk_free(keyboard);
        return (new_keyboard);
      }
    }


    keysym = find_keysym(*key, keyboard);
    if (!keysym)
    {
      return (keyboard);
    }

    draw_key(*key, keyboard, 1);

    if (handle_keymods(keysym, key, keyboard))
    {
      return (keyboard);        /* no more processing is needed */
    }

    wkeysym = malloc(sizeof(wchar_t) * (strlen(keysym) + 1));

    mbsrtowcs(wkeysym, (const char **)&keysym, strlen(keysym) + 1, NULL);

#ifdef OSK_DEBUG
    printf("wkeysym %ls %i\n\n", wkeysym, (int)wcslen(wkeysym));
#endif


    get_composed_keysym(keyboard, keyboard->composing, wkeysym);

    if (keyboard->composed)
    {
      keyboard->last_key_pressed = key;
      set_key(NULL, &keyboard->keymodifiers.compose, 0);
      ks = keyboard->composed;

#ifdef OSK_DEBUG
      printf("keysym found %ls\n", ks);
#endif

      mnemo = malloc(sizeof(char) * 32);
      snprintf(mnemo, 31, "%ls", ks);

      if (wcsncmp(L"Return", ks, 6) == 0)
      {
        event.key.keysym.sym = SDLK_RETURN;
        event.text.text[0] = '\r';
        event.text.text[1] = '\0';
      }
      else if (wcsncmp(L"Tab", ks, 3) == 0 || wcsncmp(L"ISO_Left_Tab", ks, 12) == 0)
      {
        event.key.keysym.sym = SDLK_TAB;
        event.text.text[0] = '\t';
        event.text.text[1] = '\0';
      }
      else if (wcsncmp(L"BackSpace", ks, 9) == 0)
      {
        event.key.keysym.sym = SDLK_BACKSPACE;
        event.text.text[0] = '\b';
        event.text.text[1] = '\0';
      }
      else if (wcsncmp(L"NoSymbol", ks, 8) == 0)
      {
        return (keyboard);
      }
      else
      {
        int len;

        if (keyboard->composed_type == 1)
        {
#ifdef OSK_DEBUG
          printf("Composed_type = 1: \"%ls\"\n", keyboard->composed);
#endif
          len = wcstombs(event.text.text, keyboard->composed, 16);
        }
        else
        {
          int iwc;
          wchar_t buf[2];

          iwc = keysym2unicode(mnemo2keysym(mnemo, keyboard), keyboard);
          buf[0] = (wchar_t)iwc;
          buf[1] = L'\0';

#ifdef OSK_DEBUG
          printf("iwc as buf = \"%ls\"\n", buf);
#endif
          len = wcstombs(event.text.text, buf, 16);
        }

#ifdef OSK_DEBUG
        printf("len = %d\n", len);
        printf("event.text.text = \"%s\"\n", event.text.text);
#else
        len = len;              /* Avoid 'set but not used' warning */
#endif
      }

      clear_dead_sticks(keyboard);
      event.type = SDL_TEXTINPUT;
      SDL_PushEvent(&event);
      free(mnemo);
    }
    else
    {
      if (keyboard->composing == keyboard->layout->composemap)
      {
#ifdef OSK_DEBUG
        printf("compose sequence resetted\n");
#endif
        set_key(NULL, &keyboard->keymodifiers.compose, 0);
        keyboard->last_key_pressed = key;
        clear_dead_sticks(keyboard);
      }
      else
      {
        set_key(key, &keyboard->keymodifiers.compose, 0);
#ifdef OSK_DEBUG
        printf("still composing\n");
#endif
        set_dead_sticks(key, keyboard);
        /* Fixme: Would be nice if we can highlight next available-to-compose keys, but how? */
      }
    }
    free(wkeysym);
  }

  return (keyboard);
}

void osk_released(on_screen_keyboard * keyboard)
{
  osk_key *key;

  key = keyboard->last_key_pressed;
  if (key)
  {
    draw_key(*key, keyboard, 0);
    //    free(key);
  }
  keyboard->last_key_pressed = NULL;
  draw_keyboard(keyboard);
}


static void free_keymap(osk_keymap * keymap)
{
  int i;

  for (i = 0; i < 256; i++)
  {
    if (keymap[i].plain)
      free(keymap[i].plain);
    if (keymap[i].caps)
      free(keymap[i].caps);
    if (keymap[i].altgr)
      free(keymap[i].altgr);
    if (keymap[i].shiftaltgr)
      free(keymap[i].shiftaltgr);
  }
  free(keymap);
}

static void free_composemap(osk_composenode * composenode)
{
  int i;

  for (i = 0; i < composenode->size; i++)
  {
    free_composemap(composenode->childs[i]);
    free(composenode->childs[i]);
  }
  if (composenode->result)
    free(composenode->result);
  else
    free(composenode->childs);

  if (composenode->keysym)
    free(composenode->keysym);
}

static void free_keysymdefs(keysymdefs * ksd, int size)
{
  int i;

  for (i = 0; i <= size; i++)
    free(ksd[i].mnemo);
}

static void free_keys(osk_layout * layout)
{
  int i, j;

  for (j = 0; j < layout->height; j++)
  {
    for (i = 0; i < layout->width; i++)
    {
      if (layout->keys[j][i].plain_label)
        free(layout->keys[j][i].plain_label);
      if (layout->keys[j][i].top_label)
        free(layout->keys[j][i].top_label);
      if (layout->keys[j][i].altgr_label)
        free(layout->keys[j][i].altgr_label);
      if (layout->keys[j][i].shift_altgr_label)
        free(layout->keys[j][i].shift_altgr_label);

    }
    free(layout->keys[j]);
  }
  free(layout->keys);
}

static void free_layout(osk_layout * layout)
{
  if (layout->name != NULL)
    free(layout->name);
  // free(layout->rows);
  free(layout->fontpath);
  free_keys(layout);
  free_keymap(layout->keymap);
  free_composemap(layout->composemap);
  free(layout->composemap);

  free_keysymdefs(layout->keysymdefs, layout->sizeofkeysymdefs);
  free(layout->keysymdefs);
  free(layout);
}

void osk_free(on_screen_keyboard * keyboard)
{
  free(keyboard->name);
  free_layout(keyboard->layout);
  if (keyboard->composed)
    free(keyboard->composed);
  if (keyboard->last_key_pressed)
    free(keyboard->last_key_pressed);
  if (keyboard->keyboard_list)
    free(keyboard->keyboard_list);
  SDL_FreeSurface(keyboard->surface);
  set_key(NULL, &keyboard->keymodifiers.shift, 0);
  set_key(NULL, &keyboard->keymodifiers.altgr, 0);
  set_key(NULL, &keyboard->keymodifiers.compose, 0);
  set_key(NULL, &keyboard->keymodifiers.dead, 0);
  if (keyboard->osk_fonty != NULL)
    TTF_CloseFont(keyboard->osk_fonty);

  free(keyboard);
}



/* static void on_screen_keyboardd(void ) */
/* { */
/* 	int i; */
/*     if (key_board != NULL) */
/*         SDL_FreeSurface(key_board); */

/* 	 key_board = SDL_CreateRGBSurface(canvas->flags, */
/* 				key_width * 19, */
/* 				key_height * 3, */
/* 				canvas->format->BitsPerPixel, */
/* 				canvas->format->Rmask, */
/* 				canvas->format->Gmask, */
/* 				canvas->format->Bmask, 0);	 */
/*   		key_board_color_r = 255; */
/*   		key_board_color_g = 255; */
/*   		key_board_color_b = 255; */

/*   	  SDL_FillRect(key_board, NULL, SDL_MapRGB(key_board->format, 255, 255, 255)); */
/* 	  if (keybd_position == 0) */
/* 	  { */
/* 			initial_y = 400; */
/* 	  } */
/* 	  else */
/* 	  { */
/* 			initial_y = 5; */
/* 	  } */
/* 	  apply_surface( initial_x, initial_y, key_board, screen, NULL); */

/* 	  keybd_prepare(); */

/* 	  for (i = 1; i <= 15 ; i++) */
/* 	  button (i, initial_x + (key_width)*(i-1), initial_y); */

/* 	  for (i = 1; i <= 19; i++) */
/* 	  button (i+15, initial_x + (key_width)*(i-1), initial_y + key_height); */

/* 	  for (i = 1; i <= 19; i++) */
/* 	  button (i+34, initial_x + (key_width)*(i-1), initial_y + 2*key_height); */

/* 	  drawkeybd(); */

/* 	  keybd_finish(); */

/* 	  SDL_UpdateRect(screen, 0, 0, 640, 480);     */

/* 	  /\* SDL_Delay(10); *\/ /\* FIXME: This should not be necessary! -bjk 2011.04.21 *\/ */

/*       keybd_flag = 1;    */
/* } */







/* // Check whether current mouse position is within a rectangle */
/* int regionhit(int x, int y, int w, int h) */
/* { */
/*   if (uistate.mousex < x || */
/*     uistate.mousey < y || */
/*     uistate.mousex >= x + w || */
/*     uistate.mousey >= y + h) */
/*     return 0; */
/*   return 1; */
/* } */




/* void button(int id, int x, int y) */
/* { */
/*   SDL_Rect dest,desti; */
/*   SDL_Surface *tmp_imgup; */
/*   SDL_Event event; */
/*   dest.x = x; */
/*   dest.y = y; */

/*   // Check whether the button should be hot */
/*   if (regionhit(x, y, 24, 24)) */
/*   { */
/*     uistate.hotitem = id; */

/* 	if (uistate.activeitem == 0 && uistate.mousedown) */
/* 	{ */
/* 		uistate.activeitem = id; */
/* 		activeflag = 1; */
/* 	} */
/*   } */

/*   // Render button  */
/*   SDL_BlitSurface(img_btnsm_up, NULL, screen, &dest); */
/*   if (caps_flag % 2 != 0) */
/* 	{ */
/* 		desti.x = initial_x; */
/* 		desti.y = initial_y + key_height; */
/* 		SDL_BlitSurface(img_btnsm_down, NULL, screen, &desti); */
/* 	} */
/*   if (uistate.hotitem == id) */
/*   { */
/*     if (uistate.activeitem == id) */
/*     { */
/*       // Button is both 'hot' and 'active' */
/* 		if (activeflag == 1) */
/* 		{ */
/* 	  		ide = id; */
/* 	  		gen_key_flag = 1; */
/* 			activeflag = 0; */
/* 			uistate.activeitem = 0; */
/* 		} */
/* 	  SDL_BlitSurface(img_btnsm_down, NULL, screen, &dest); */
/*     } */
/*     else */
/*     { */
/*       // Button is merely 'hot' */
/* 	  SDL_BlitSurface(img_btnsm_down, NULL, screen, &dest); */
/*     } */
/*   } */
/*   else */
/*   {	 */
/* 	// button is not hot, but it may be active     */
/* 	SDL_BlitSurface(img_btnsm_up, NULL, screen, &dest); */
/*   } */

/*   if (gen_key_flag == 1) */
/* 	{ */
/* 		int i,j; */
/* 		gen_key_flag = 0; */
/* 		enter_flag = 0; */
/* 		SDL_EnableUNICODE(1); */
/* //		printf("\n entered here %d th time \n", k); */
/* //		k++; */
/* 		if (ide == 1) */
/* 		{ */
/* 			event.key.keysym.sym = SDLK_ESCAPE; */
/* 			event.key.keysym.mod = KMOD_NONE; */
/* 			event.key.keysym.unicode = 27; */

/* 		} */
/* 		else if (ide == 2) */
/* 		{ */
/* 			event.key.keysym.sym = SDLK_BACKQUOTE; */
/* 			event.key.keysym.mod = KMOD_NONE; */
/* 			event.key.keysym.unicode = 96; */
/* 		} */


//....................................................

/* 		if (enter_flag == 0)  */
/* 		{ */

                  /* event.key.type=SDL_KEYDOWN; */
                  /* SDL_PushEvent(&event); */
                  /* event.key.type=SDL_KEYUP; */
                  /* SDL_PushEvent(&event); */


/* 	}	 */
/* 	} */
/* } */
