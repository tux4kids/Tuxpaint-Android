/* tp_magic_example.c

   An example of a "Magic" tool plugin for Tux Paint
   Last modified: 2008.07.10
*/


/* Inclusion of header files: */
/* -------------------------- */

#include <stdio.h>
#include <string.h>  // For "strdup()"
#include <libintl.h>  // For "gettext()"

#include "tp_magic_api.h"  // Tux Paint "Magic" tool API header
#include "SDL_image.h"  // For IMG_Load(), to load our PNG icon
#include "SDL_mixer.h"  // For Mix_LoadWAV(), to load our sound effects


/* Tool Enumerations: */
/* ------------------ */

/* What tools we contain: */

enum {
  TOOL_ONE,  // Becomes '0'
  TOOL_TWO,  // Becomes '1'
  NUM_TOOLS  // Becomes '2'
};


/* A list of filenames for sounds and icons to load at startup: */

const char * snd_filenames[NUM_TOOLS] = {
  "one.wav",
  "two.wav"
};

const char * icon_filenames[NUM_TOOLS] = {
  "one.png",
  "two.png"
};


// NOTE: We use a macro called "gettext_noop()" below in some arrays of
// strings (char *'s) that hold the names and descriptions of our "Magic"
// tools.  This allows the strings to be localized into other languages.


/* A list of names for the tools */

const char * names[NUM_TOOLS] = {
  gettext_noop("A tool"),
  gettext_noop("Another tool")
};


/* A list of descriptions of the tools */

const char * descs[NUM_TOOLS] = {
  gettext_noop("This is example tool number 1."),
  gettext_noop("This is example tool number 2.")
};



/* Our global variables: */
/* --------------------- */

/* Sound effects: */
Mix_Chunk * snd_effect[NUM_TOOLS];

/* The current color (an "RGB" value) the user has selected in Tux Paint: */
Uint8 example_r, example_g, example_b;


/* Our local function prototypes: */
/* ------------------------------ */

// These functions are called by other functions within our plugin,
// so we provide a 'prototype' of them, so the compiler knows what
// they accept and return.  This lets us use them in other functions
// that are declared _before_ them.

void example_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect);

void example_line_callback(void * ptr, int which,
                           SDL_Surface * canvas, SDL_Surface * snapshot,
                           int x, int y);


/* Setup Functions: */
/* ---------------- */

// API Version check
// 
// The running copy of Tux Paint that has loaded us first asks us what
// version of the Tux Paint "Magic" tool plugin API we were built against.
// If it deems us compatible, we'll be used!
//
// All we need to do here is return "TP_MAGIC_API_VERSION",
// which is #define'd in tp_magic_api.h.

Uint32 example_api_version(void)
{
  return(TP_MAGIC_API_VERSION);
}


// Initialization
//
// This happens once, when Tux Paint starts up and is loading all of the
// "Magic" tool plugins.  (Assuming what we returned from api_version() was
// acceptable!)
// 
// All we're doing in this example is loading our sound effects,
// which we'll use later (in click(), drag() and release())
// when the user is using our Magic tools.
// 
// The memory we allocate here to store the sounds will be
// freed (aka released, aka deallocated) when the user quits Tux Paint,
// when our shutdown() function is called.

int example_init(magic_api * api)
{
  int i;
  char fname[1024];

  for (i = 0; i < NUM_TOOLS; i++)
  {
    // Assemble the filename from the "snd_filenames[]" array into
    // a full path to a real file.
    //
    // Use "api->data_directory" to figure out where our sounds should be.
    // (The "tp-magic-config --dataprefix" command would have told us when
    // we installed our plugin and its data.)

    snprintf(fname, sizeof(fname),
             "%s/sounds/magic/%s",
	     api->data_directory, snd_filenames[i]);

    printf("Trying to load %s sound file\n", fname);

    // Try to load the file!

    snd_effect[i] = Mix_LoadWAV(fname);
  }

  return(1);
}


// Report our tool count
// 
// Tux Paint needs to know how many "Magic" tools we'll be providing.
// Return that number here.  (We simply grab the value of "NUM_TOOLS"
// from our 'enum' above!)
// 
// When Tux Paint is starting up and loading plugins, it will call
// some of the following setup functions once for each tool we report.

int example_get_tool_count(magic_api * api)
{
  return(NUM_TOOLS);
}


// Load icons
//
// When Tux Paint is starting up and loading plugins, it asks us to
// provide icons for the "Magic" tool buttons.

SDL_Surface * example_get_icon(magic_api * api, int which)
{
  char fname[1024];

  // Assemble the filename from the "icon_filenames[]" array into
  // a full path to a real file.
  //
  // Use "api->data_directory" to figure out where our sounds should be.
  // (The "tp-magic-config --dataprefix" command would have told us when
  // we installed our plugin and its data.)
  //
  // We use 'which' (which of our tools Tux Paint is asking about)
  // as an index into the array.

  snprintf(fname, sizeof(fname), "%s/images/magic/%s.png",
	     api->data_directory, icon_filenames[which]);


  // Try to load the image, and return the results to Tux Paint:

  return(IMG_Load(fname));
}


// Report our "Magic" tool names
//
// When Tux Paint is starting up and loading plugins, it asks us to
// provide names (labels) for the "Magic" tool buttons.

char * example_get_name(magic_api * api, int which)
{
  const char * our_name_english;
  const char * our_name_localized;

  // Get our name from the "names[]" array.
  //
  // We use 'which' (which of our tools Tux Paint is asking about)
  // as an index into the array.

  our_name_english = names[which];


  // Return a localized (aka translated) version of our name,
  // if possible.
  //
  // We send "gettext()" the English version of the name from our array.

  our_name_localized = gettext(our_name_english);


  // Finally, duplicate the string into a new section of memory, and
  // send it to Tux Paint.  (Tux Paint keeps track of the string and
  // will free it for us, so we have one less thing to keep track of.)

  return(strdup(our_name_localized));
}


// Report our "Magic" tool descriptions
//
// When Tux Paint is starting up and loading plugins, it asks us to
// provide names (labels) for the "Magic" tool buttons.

char * example_get_description(magic_api * api, int which, int mode)
{
  const char * our_desc_english;
  const char * our_desc_localized;

  // Get our desc from the "descs[]" array.
  //
  // We use 'which' (which of our tools Tux Paint is asking about)
  // as an index into the array.

  our_desc_english = descs[which];


  // Return a localized (aka translated) version of our description,
  // if possible.
  //
  // We send "gettext()" the English version of the description from our array.

  our_desc_localized = gettext(our_desc_english);


  // Finally, duplicate the string into a new section of memory, and
  // send it to Tux Paint.  (Tux Paint keeps track of the string and
  // will free it for us, so we have one less thing to keep track of.)

  return(strdup(our_desc_localized));
}

// Report whether we accept colors

int example_requires_colors(magic_api * api, int which)
{
  // Both of our tools accept colors, so we're always returning '1' (for "true")

  return 1;
}


// Report what modes we work in

int example_modes(magic_api * api, int which)
{
  // Both of our tools are painted (neither affect the full-screen),
  // so we're always returning 'MODE_PAINT'

  return MODE_PAINT;
}


// Shut down
//
// Tux Paint is quitting.  When it quits, it asks all of the plugins
// to 'clean up' after themselves.  We, for example, loaded some sound
// effects at startup (in our init() function), so we should free the
// memory used by them now.

void example_shutdown(magic_api * api)
{
  int i;

  // Free (aka release, aka deallocate) the memory used to store the
  // sound effects that we loaded during init():

  for (i = 0; i < NUM_TOOLS; i++)
    Mix_FreeChunk(snd_effect[i]);
}


/* Functions that respond to events in Tux Paint: */
/* ---------------------------------------------- */

// Affect the canvas on click:

void example_click(magic_api * api, int which, int mode,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
	           int x, int y, SDL_Rect * update_rect)
{
  // In our case, a single click (which is also the start of a drag!)
  // is identical to what dragging does, but just at one point, rather
  // than across a line.
  // 
  // So we 'cheat' here, by calling our draw() function with
  // (x,y) for both the beginning and end points of a line.

  example_drag(api, which, canvas, snapshot, x, y, x, y, update_rect);
}


// Affect the canvas on drag:

void example_drag(magic_api * api, int which, SDL_Surface * canvas,
	          SDL_Surface * snapshot, int ox, int oy, int x, int y,
		  SDL_Rect * update_rect)
{
  // Call Tux Paint's "line()" function.
  //
  // It will calculate a straight line between (ox,ox) and (x,y).
  // Every N steps along that line (in this case, N is '1'), it
  // will call _our_ function, "example_line_callback()", and send
  // the current X,Y coordinates along the line, as well as other
  // useful things (which of our "Magic" tools is being used and
  // the current and snapshot canvases).

  api->line((void *) api, which, canvas, snapshot,
            ox, oy, x, y, 1, example_line_callback);


  // If we need to, swap the X and/or Y values, so that
  // (ox,oy) is always the top left, and (x,y) is always the bottom right,
  // so the values we put inside "update_rect" make sense:

  if (ox > x) { int tmp = ox; ox = x; x = tmp; }
  if (oy > y) { int tmp = oy; oy = y; y = tmp; }


  // Fill in the elements of the "update_rect" SDL_Rect structure
  // that Tux Paint is sharing with us.

  update_rect->x = ox - 4;
  update_rect->y = oy - 4;
  update_rect->w = (x + 4) - update_rect->x;
  update_rect->h = (y + 4) - update_rect->h;


  // Play the appropriate sound effect
  //
  // We're calculating a value between 0-255 for where the mouse is
  // across the canvas (0 is the left, ~128 is the center, 255 is the right).
  //
  // These are the exact values Tux Paint's "playsound()" wants,
  // to determine what speaker to play the sound in.
  // (So the sound will pan from speaker to speaker as you drag the
  // mouse around the canvas!)

  api->playsound(snd_effect[which],
                 (x * 255) / canvas->w, // pan
	         255); // distance
}


// Affect the canvas on release:

void example_release(magic_api * api, int which,
	           SDL_Surface * canvas, SDL_Surface * snapshot,
	           int x, int y, SDL_Rect * update_rect)
{
  // Neither of our effects do anything special when the mouse is released
  // from a click or click-and-drag, so there's no code here...
}


// Accept colors
//
// When any of our "Magic" tools are activated by the user,
// if that tool accepts colors, the current color selection is sent to us.
//
// Additionally, if one of our color-accepting tools is active when the
// user changes colors, we'll be informed of that, as well.
// 
// The color comes in as RGB values.

void example_set_color(magic_api * api, Uint8 r, Uint8 g, Uint8 b)
{
  // We simply store the RGB values in the global variables we
  // declared at the top of this file.

  example_r = r;
  example_g = g;
  example_b = b;
}


/* The Magic Effect Routines! */
/* -------------------------- */

// Our "callback" function
//
// We do the 'work' in this callback.  Our plugin file has just one.
// Some "Magic" tool plugins may have more, depending on the tools they're
// providing.  Some have none (since they're not click-and-drag
// painting-style tools).
//
// Our callback function gets called once for every point along a line between
// the mouse's previous and current position, as it's being dragged.
//
// It pays attention to 'which' to determine which of our plugin's tools
// is currently selected.

void example_line_callback(void * ptr, int which,
                           SDL_Surface * canvas, SDL_Surface * snapshot,
                           int x, int y)
{
  // For technical reasons, we can't accept a pointer to the "magic_api"
  // struct, like the other functions do.
  //
  // Instead, we receive a 'generic' pointer (a "void *").
  // The line below declares a local "magic_api" pointer variable called "api",
  // and then assigns it to the value of the 'generic' pointer we received.
  // 
  // (The "(magic_api *)" casts the generic pointer into the 'type' of
  // pointer we want, a pointer to a "magic_api".)
  magic_api * api = (magic_api *) ptr;
  int xx, yy;


  // This function handles both of our tools, so we need to check which
  // is being used right now.  We compare the 'which' argument that
  // Tux Paint sends to us with the values we enumerated above.

  if (which == TOOL_ONE)
  {
    // Tool number 1 simply draws a single pixel at the (x,y) location.
    // It's a 1x1 pixel brush

    api->putpixel(canvas, x, y, SDL_MapRGB(canvas->format,
                                           example_r,
                                           example_g,
                                           example_b));

    // We use "SDL_MapRGB()" to convert the RGB value we receive from Tux Paint
    // for the user's current color selection to a 'Uint32' pixel value
    // we can send to Tux Paint's "putpixel()" function.
  }
  else if (which == TOOL_TWO)
  {
    // Tool number 2 copies an 8x8 square of pixels from the opposite side
    // of the canvas and puts it under the cursor

    for (yy = -4; yy < 4; yy++)
    {
      for (xx = -4; xx < 4; xx++)
      {
        api->putpixel(canvas, x + xx, y + yy,
		      api->getpixel(snapshot,
				    canvas->w - x - xx,
		      		    canvas->h - y - yy));

	// We simply use Tux Paint's "getpixel()" routine to pull pixel
	// values from the 'snapshot', and then "putpixel()" to draw them
	// right into the 'canvas'.
	
	// Note: putpixel() and getpixel() are safe to use, even if your
	// X,Y values are outside of the SDL surface (e.g., negative, or
	// greater than the surface's width or height).
      }
    }
  }
}

// Switch-In event
//
// This happens whenever a Magic tool is enabled, either because the
// user just selected it, or they just came back to "Magic" after using
// another tool (e.g., Brush or Text), and this was the most-recently
// selected Magic tool.
// 
// (This also applies to momentary tools, like
// Undo and Redo, and image-changing tools such as New and Open.)
// 
// It also happens when a Magic tool's mode changes (it first
// receives a 'switchout()', below, for the old mode).
//
// Our example doesn't do anything when we switch to, or away from, our
// Magic tools, so we just do nothing here.

void example_switchin(magic_api * api, int which, int mode, SDL_Surface * canvas)
{
}

// Switch-Out event
//
// This happens whenever a Magic tool is disabled, either because the
// user selected a different Magic tool, or they selected a completely
// different tool (e.g., Brush or Text).
// 
// (This also applies to momentary tools, like Undo and Redo, and
// image-changing tools such as New and Open, in which case the
// switchin() function will be called moments later.)
//
// It also happens when a Magic tool's mode changes (it then
// receives a 'switchin()', above, for the new mode).
// 
// Our example doesn't do anything when we switch to, or away from, our
// Magic tools, so we just do nothing here.

void example_switchout(magic_api * api, int which, int mode, SDL_Surface * canvas)
{
}
