/* tp_magic_example.c

   An example of a "Magic" tool plugin for Tux Paint
   avril 13, 2023
*/


/* Inclusion of header files */
/* ---------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>        // For "strdup()"
#include <libintl.h>       // For "gettext()"

#include "tp_magic_api.h"  // Tux Paint "Magic" tool API header
#include "SDL_image.h"     // For IMG_Load(), to load our PNG icon
#include "SDL_mixer.h"     // For Mix_LoadWAV(), to load our sound effects


/* Tool Enumerations: */
/* ---------------------------------------------------------------------- */

/* What tools we contain: */


enum
{
  TOOL_ONE, // Becomes '0'
  TOOL_TWO, // Becomes '1'
  NUM_TOOLS // Becomes '2'
};


/* Lists of filenames for sounds and icons to load at startup: */

const char *sound_filenames[NUM_TOOLS] = {
  "tool_one.wav",
  "tool_two.wav"
};

const char *icon_filenames[NUM_TOOLS] = {
  "tool_one.png",
  "tool_two.png"
};


/*
NOTE: We use a macro called "gettext_noop()" below in some arrays of
strings (char *'s) that hold the names and descriptions of our "Magic"
tools.  This allows the strings to be localized into other languages.
*/


/* A list of names for the tools */

const char *tool_names[NUM_TOOLS] = {
  gettext_noop("A tool"),
  gettext_noop("Another tool")
};


/* How to group the tools with other similar tools, within the 'Magic' selector: */

const int tool_groups[NUM_TOOLS] = {
  MAGIC_TYPE_PAINTING,
  MAGIC_TYPE_DISTORTS
};


/* A list of descriptions of the tools */

const char *tool_descriptions[NUM_TOOLS] = {
  gettext_noop("This is example tool number 1."),
  gettext_noop("This is example tool number 2.")
};



/* Our global variables: */
/* ---------------------------------------------------------------------- */

/* Sound effects: */
Mix_Chunk *sound_effects[NUM_TOOLS];

/* The current color (an "RGB" -- red, green, blue -- value) the user has
selected in Tux Paint (for tool 1): */
Uint8 example_r, example_g, example_b;

/* The size the user has selected in Tux Paint (for tool 2): */
Uint8 example_size;


/* Our local function prototypes: */
/* ---------------------------------------------------------------------- */

/*
These functions are called by other functions within our plugin, so we
provide a 'prototype' of them, so the compiler knows what they accept and
return.  This lets us use them in other functions that are declared
_before_ them.
*/

void example_drag(magic_api * api, int which, SDL_Surface * canvas,
  SDL_Surface * snapshot, int old_x, int old_y, int x, int y,
  SDL_Rect * update_rect);

void example_line_callback(void *pointer, int which, SDL_Surface * canvas,
  SDL_Surface * snapshot, int x, int y);


/* Setup Functions: */
/* ---------------------------------------------------------------------- */

/*
API Version check

The running copy of Tux Paint that has loaded us first asks us what version
of the Tux Paint 'Magic' tool plugin API we were built against.  If it
deems us compatible, we'll be used!

All we need to do here is return "TP_MAGIC_API_VERSION", which is defined
(#define) in the header file "tp_magic_api.h".
*/

Uint32 example_api_version(void)
{
  return (TP_MAGIC_API_VERSION);
}


/*
Initialization

This happens once, when Tux Paint starts up and is loading all of the
'Magic' tool plugins.  (Assuming what we returned from api_version was
acceptable!)

All we're doing in this example is loading our sound effects, which we'll
use later (in example_click(), example_drag(), and example_release()) when
the user is using our Magic tools.

The memory we allocate here to store the sounds will be freed (aka
released, aka deallocated) when the user quits Tux Paint, when our
example_shutdown() function is called.
*/

int example_init(magic_api * api, Uint32 disabled_features)
{
  int i;
  char filename[1024];

  for (i = 0; i < NUM_TOOLS; i++)
  {
    /*
    Assemble the filename from the "sound_filenames[]" array into a full path
    to a real file.

    Use "api->data_directory" to figure out where our sounds should be. (The
    "tp-magic-config --dataprefix" command would have told us when we installed
    our plugin and its data.)
    */
    snprintf(filename, sizeof(filename), "%s/sounds/magic/%s", api->data_directory,
             sound_filenames[i]);

    printf("Trying to load %s sound file\n", filename);

    /* Try to load the file! */

    sound_effects[i] = Mix_LoadWAV(filename);
  }

  return (1);
}


/*
Report our tool count

Tux Paint needs to know how many 'Magic' tools we'll be providing. Return
that number here.  (We simply grab the value of 'NUM_TOOLS' from our 'enum'
above!)

When Tux Paint is starting up and loading plugins, it will call some of the
following setup functions once for each tool we report.
*/
int example_get_tool_count(magic_api * api)
{
  return (NUM_TOOLS);
}


/*
Load our icons

When Tux Paint is starting up and loading plugins, it asks us to provide icons for the 'Magic' tool buttons.
*/
SDL_Surface *example_get_icon(magic_api * api, int which)
{
  char filename[1024];

  /*
  Assemble the filename from the "icon_filenames[]" array into a full path to
  a real file.

  Use "api->data_directory" to figure out where our sounds should be. (The
  "tp-magic-config --dataprefix" command would have told us when we installed
  our plugin and its data.)

  We use "which" (which of our tools Tux Paint is asking about) as an index
  into the array.
  */
  snprintf(filename, sizeof(filename), "%s/images/magic/%s",
           api->data_directory, icon_filenames[which]);

  printf("Trying to load %s icon\n", filename);

  /* Try to load the image, and return the results to Tux Paint: */

  return (IMG_Load(filename));
}


/*
Report our 'Magic' tool names

When Tux Paint is starting up and loading plugins, it asks us to provide
names (labels) for the 'Magic' tool buttons.
*/
char *example_get_name(magic_api * api, int which)
{
    const char *our_name_english;
  const char *our_name_localized;

  /*
  Get our name from the "tool_names[]" array.

  We use 'which' (which of our tools Tux Paint is asking about) as an index
  into the array.
  */
  our_name_english = tool_names[which];


  /*
  Return a localized (aka translated) version of our name, if possible.

  We send "gettext()" the English version of the name from our array.
  */
  our_name_localized = gettext(our_name_english);


  /*
  Finally, duplicate the string into a new section of memory, and send it to
  Tux Paint.  (Tux Paint keeps track of the string and will free it for us,
  so we have one less thing to keep track of.)
  */
  return (strdup(our_name_localized));
}


/*
Report our 'Magic' tool groups

When Tux Paint is starting up and loading plugins, it asks us to specify
where the tool should be grouped.
*/
int example_get_group(magic_api * api, int which)
{
  /*
  Return our group, found in the "tool_groups[]" array.

  We use 'which' (which of our tools Tux Paint is asking about) as an index
  into the array.
  */
  return (tool_groups[which]);
}


/*
Report our 'Magic' tool descriptions

When Tux Paint is starting up and loading plugins, it asks us to provide
descriptions of each 'Magic' tool.
*/
char *example_get_description(magic_api * api, int which, int mode)
{
  const char *our_desc_english;
  const char *our_desc_localized;

  /*
  Get our description from the "tool_descriptions[]" array.

  We use 'which' (which of our tools Tux Paint is asking about) as an index
  into the array.
  */
  our_desc_english = tool_descriptions[which];


  /*
  Return a localized (aka translated) version of our description, if
  possible.

  We send "gettext" the English version of the description from our array.
  */
  our_desc_localized = gettext(our_desc_english);


  /*
  Finally, duplicate the string into a new section of memory, and send it to
  Tux Paint.  (Tux Paint keeps track of the string and will free it for us,
  so we have one less thing to keep track of.)
  */

  return (strdup(our_desc_localized));
}


// Report whether we accept colors

int example_requires_colors(magic_api * api, int which)
{
  if (which == TOOL_ONE)
    return 1;
  else
    return 0;
}


// Report what modes we work in

int example_modes(magic_api * api, int which)
{
  /*
  Both of our tools are painted (neither affect the full-screen), so we're
  always returning 'MODE_PAINT'
  */

  return MODE_PAINT;
}


// Report whether the tools offer sizing options

Uint8 example_accepted_sizes(magic_api * api, int which, int mode)
{
  if (which == TOOL_ONE)
    return 1;
  else
    return 4;
}


// Return our default sizing option

Uint8 example_default_size(magic_api * api, int which, int mode)
{
  return 1;
}


/*
Shut down

Tux Paint is quitting.  When it quits, it asks all of the plugins to 'clean
up' after themselves.  We, for example, loaded some sound effects at
startup (in our example_init() function), so we should free the memory used
by them now.
*/
void example_shutdown(magic_api * api)
{
  int i;

  /*
  Free (aka release, aka deallocate) the memory used to store the sound
  effects that we loaded during example_init():
  */
  for (i = 0; i < NUM_TOOLS; i++)
    Mix_FreeChunk(sound_effects[i]);
}


/* Functions that respond to events in Tux Paint: */
/* ---------------------------------------------------------------------- */

/* Affect the canvas on click: */

void
example_click(magic_api * api, int which, int mode,
              SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y,
              SDL_Rect * update_rect)
{
  /*
  In our case, a single click (which is also the start of a drag!) is
  identical to what dragging does, but just at one point, rather than across
  a line.

  So we 'cheat' here, by calling our "example_draw()" function with (x,y) for
  both the beginning and end points of a line.
  */

  example_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
}


/* Affect the canvas on drag: */
void
example_drag(magic_api * api, int which,
             SDL_Surface * canvas, SDL_Surface * snapshot,
             int old_x, int old_y, int x, int y,
             SDL_Rect * update_rect)
{
  /*
  Call Tux Paint's "line()" (line-traversing) function.

  It will calculate a straight line between (old_x,old_y) and (x,y). Every
  N steps along that line (in this case, N is '1'), it will call _our_
  function, "example_line_callback()", and send the current X,Y
  coordinates along the line, as well as other useful things (which of our
  'Magic' tools is being used and the current and snapshot canvases).
  */
  SDL_LockSurface(snapshot);
  SDL_LockSurface(canvas);

  api->line((void *) api, which, canvas, snapshot,
            old_x, old_y, x, y, 1,
            example_line_callback);

  SDL_UnlockSurface(canvas);
  SDL_UnlockSurface(snapshot);

  /*
  If we need to, swap the X and/or Y values, so that the coordinates
  (old_x,old_y) is always the top left, and the coordinates (x,y) is
  always the bottom right, so the values we put inside "update_rect" make
  sense:
  */

  if (old_x > x)
  {
    int temp = old_x;

    old_x = x;
    x = temp;
  }
  if (old_y > y)
  {
    int temp = old_y;

    old_y = y;
    y = temp;
  }


  /*
  Fill in the elements of the "update_rect" SDL_Rect structure that Tux
  Paint is sharing with us, therefore telling Tux Paint which part of the
  canvas has been modified and should be updated.
  */

  if (which == TOOL_ONE) {
    update_rect->x = old_x;
    update_rect->y = old_y;
    update_rect->w = (x - old_x) + 1;
    update_rect->h = (y - old_y) + 1;
  } else {
    update_rect->x = old_x - example_size;
    update_rect->y = old_y - example_size;
    update_rect->w = (x + example_size) - update_rect->x + 1;
    update_rect->h = (y + example_size) - update_rect->y + 1;
  }

  /*
  Play the appropriate sound effect

  We're calculating a value between 0-255 for where the mouse is
  horizontally across the canvas (0 is the left, ~128 is the center, 255
  is the right).

  These are the exact values Tux Paint's "playsound()" wants, to determine
  what speaker to play the sound in. (So the sound will pan from speaker
  to speaker as you drag the mouse around the canvas!)
  */
  api->playsound(sound_effects[which],
    (x * 255) / canvas->w, /* Left/right pan */
    255 /* Near/far distance (loudness) */);
}


/* Affect the canvas on release: */

void
example_release(magic_api * api, int which,
                SDL_Surface * canvas, SDL_Surface * snapshot, int x, int y,
                SDL_Rect * update_rect)
{
  /*
  Neither of our effects do anything special when the mouse is released
  from a click or click-and-drag, so there's no code here...
  */
}


/*
Accept colors

When any of our 'Magic' tools are activated by the user, if that tool
accepts colors, the current color selection is sent to us.

Additionally, if one of our color-accepting tools is active when the user
changes their chosen, we'll be informed of that as well.

The color comes in as RGB (red, green, and blue) values from 0 (darkest) to
255 (brightest).
*/
void example_set_color(magic_api * api, int which, SDL_Surface * canvas, SDL_Surface * snapshot, Uint8 r, Uint8 g, Uint8 b, SDL_Rect * update_rect)
{
  /*
  We simply store the RGB values in the global variables we declared at
  the top of this file.
  */

  example_r = r;
  example_g = g;
  example_b = b;
}


/*
Accept sizes

When any of our 'Magic' tools are activated by the user, if that tool
offer's sizes, the current size selection is sent to us.

Additionally, if the user changes the tool's size, we'll be informed of
that as well.

The size comes in as an unsigned integer (Uint8) between 1 and the value
returned by our example_accepted_sizes() function during setup.
*/
void example_set_size(magic_api * api, int which, SDL_Surface * canvas, SDL_Surface * snapshot, Uint8 size, SDL_Rect * update_rect)
{
  /*
  Store the new size into the global variable we declared at the top of
  this file.
  */

  example_size = size * 4;
}


/* The Magic Effect Routines! */
/* ---------------------------------------------------------------------- */

/*
Our 'callback' function

We do the 'work' in this callback.  Our plugin file has just one. Some
'Magic' tool plugins may have more, depending on the tools they're
providing.  Some have none (since they're not click-and-drag painting-style
tools).

Our callback function gets called once for every point along a line between
the mouse's previous and current position, as it's being dragged.

Our callback pays attention to 'which' to determine which of our plugin's
tools is currently selected.
*/
void example_line_callback(void *pointer, int which, SDL_Surface * canvas,
  SDL_Surface * snapshot, int x, int y)
{
  /*
  For technical reasons, we can't accept a pointer to the Tux Paint API's
  "magic_api" struct, like the other functions do.

  Instead, we receive a 'generic' pointer (a "void *"). The line below
  declares a local "magic_api" pointer variable called "api", and then
  assigns it to the value of the 'generic' pointer we received.

  The "(magic_api *)" seen below casts the generic "void *" pointer into
  the 'type' of pointer we want, a pointer to a "magic_api" struct.)
  */
  magic_api *api = (magic_api *) pointer;
  int xx, yy;

  /*
  This function handles both of our tools, so we need to check which is
  being used right now.  We compare the 'which' argument that Tux Paint
  sends to us with the values we enumerated above.
  */

  if (which == TOOL_ONE)
  {
    /*
    Tool number 1 simply draws a single pixel at the (x,y) location. It acts
    as a 1x1 pixel brush.
    */

    api->putpixel(canvas, x, y,
                  SDL_MapRGB(canvas->format,
                             example_r, example_g, example_b));

    /*
    We use "SDL_MapRGB()" to convert the RGB value we receive from Tux Paint
    for the user's current color selection to a 'Uint32' pixel value we can
    send to Tux Paint's "putpixel()" function.
    */
  }
  else if (which == TOOL_TWO)
  {
    /*
    Tool number 2 copies a square of pixels (of the size chosen by the user)
    from the opposite side of the canvas and puts it under the cursor.
    */

    for (yy = -example_size; yy < example_size; yy++)
    {
      for (xx = -example_size; xx < example_size; xx++)
      {
        api->putpixel(canvas, x + xx, y + yy,
                      api->getpixel(snapshot,
                                    snapshot->w - x - xx,
                                    snapshot->h - y - yy));

        /*
        Here we have simply use Tux Paint's "getpixel()" routine to pull pixel
        values from the 'snapshot', and then "putpixel()" to draw them right
        into the 'canvas'.

        Note: putpixel() and getpixel() are safe to use, even if your X,Y values
        are outside of the SDL surface (e.g., negative, or greater than the
        surface's width and/or height).
        */
      }
    }
  }
}

/*
Switch-In event

This happens whenever a Magic tool is enabled, either because the user just
selected it, or they just came back to 'Magic' after using another tool
(e.g., Brush or Text), and this was the most-recently selected Magic tool.

(This also applies to momentary tools, like Undo and Redo, and
image-changing tools such as New and Open.)

It also happens when a Magic tool's mode changes (we will first receive a
call to 'example_switchout()', below, for the old mode).

Our example doesn't do anything when we switch to, or away from, our Magic
tools, so we just do nothing here.
*/
void example_switchin(magic_api * api, int which, int mode,
                      SDL_Surface * canvas)
{
}

/*
Switch-Out event

This happens whenever a Magic tool is disabled, either because the user
selected a different Magic tool, or they selected a completely different
tool (e.g., Brush or Text).

(This also applies to momentary tools, like Undo and Redo, and
image-changing tools such as New and Open.)

(And in that case, our example_switchin() function will be called moments
later.

It also happens when a Magic tool's mode changes (we will then receive a
call to 'example_switchin()', above, for the new mode).

Our example doesn't do anything when we switch to, or away from, our Magic
tools, so we just do nothing here.
*/
void example_switchout(magic_api * api, int which, int mode,
                       SDL_Surface * canvas)
{
}
