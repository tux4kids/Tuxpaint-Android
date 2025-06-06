/*







DO NOT EDIT ME!







*/
#ifndef TP_MAGIC_API_H
#define TP_MAGIC_API_H

/* src/tp_magic_api.h.in last modified 2024-06-01 */

#include "SDL.h"
#include "SDL_mixer.h"
#include "libintl.h"
#ifndef gettext_noop
#define gettext_noop(String) String
#endif

#ifdef __OS2__
#  define TX_EXTERN __declspec(dllexport)
#else
#  define TX_EXTERN
#endif

/* min() and max() variable comparisons: */

#ifdef __GNUC__
// This version has strict type checking for safety.
// See the "unnecessary" pointer comparison. (from Linux)
#define min(x,y) ({ \
  typeof(x) _x = (x);     \
  typeof(y) _y = (y);     \
  (void) (&_x == &_y);            \
  _x < _y ? _x : _y; })
#define max(x,y) ({ \
  typeof(x) _x = (x);     \
  typeof(y) _y = (y);     \
  (void) (&_x == &_y);            \
  _x > _y ? _x : _y; })
#else
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

/* clamp() returns 'value', unless it's less than 'lo' or greater than 'hi',
   in which cases it returns 'lo' or 'hi', respectively: */

#define clamp(lo,value,hi)    (min(max(value,lo),hi))


/* Flags you can send to 'special_notify' */

/* The image has been mirrored (so starter should be, too) */
/* (as of API version 0x00000001) */
#define SPECIAL_MIRROR	0x0001

/* The image has been flipped (so starter should be, too) */
/* (as of API version 0x00000001) */
#define SPECIAL_FLIP	0x0002


/* Flags you return when asked what modes you work in */

/* User can paint the tool, freehand */
/* (Icon: Paint) */
#define MODE_PAINT              0x0001 /* (as of API version 0x00000002) */

/* User can apply effect to entire canvas at once */
/* (Icon: Fullscreen) */
#define MODE_FULLSCREEN         0x0002 /* (as of API version 0x00000002) */

/* User can paint the tool, freehand -- shows a preview in the meantime */
/* (Icon: Paint) */
#define MODE_PAINT_WITH_PREVIEW 0x0004 /* (as of API version 0x00000003) */

/* User can click once at different points on the canvas */
/* (Icon: Paint) */
#define MODE_ONECLICK           0x0008 /* (as of API version 0x00000003) */

/* Note: You can "|" (OR) together MODE_FULLSCREEN with one of the other modes
   (e.g., "MODE_PAINT | MODE_FULLSCREEN", or "MODE_ONECLICK | MODE_FULLSCREEN")
   You cannot OR those others together (e.g., "MODE_PAINT | MODE_ONECLICK");
   "MODE_PAINT" will take precedence */

#define MAX_MODES 2 /* Paint & Fullscreen */


typedef struct magic_api_t {
  /* A string containing the current version of Tux Paint (e.g., "0.9.18") */
  char * tp_version;

  /* A string containing Tux Paint's data directory
     (e.g., "/usr/local/share/tuxpaint/") */
  char * data_directory;

  /* Call to have Tux Paint draw (and animate) its progress bar */
  void (*update_progress_bar)(void);

  /* Call to request special events; see "SPECIAL_..." flags, above */
  void (*special_notify)(int);

  /* Converts an RGB byte to a linear float */
  float (*sRGB_to_linear)(Uint8);

  /* Converts a linear float to an RGB byte */
  Uint8 (*linear_to_sRGB)(float);

  /* Returns whether an (x,y) location is within a circle of a particular
     radius (centered around the origin: (0,0)); useful for creating tools
     that have a circular 'brush' */
  int (*in_circle)(int,int,int);

  /* Receives an SDL pixel value from the surface at an (x,y) location;
     use "SDL_GetRGB()" to convert the Uint32 into a Uint8 RGB values;
     NOTE: Use SDL_LockSurface() on the surface before doing (a batch of)
     this call!  Use SDL_UnlockSurface() when you're done.
     SDL_MustLockSurface() can tell you whether a surface needs to be locked. */
  Uint32 (*getpixel)(SDL_Surface *, int, int);

  /* Assigns an SDL pixel value on a surface at an (x,y) location;
     use "SDL_MapRGB()" to convert a triplet of Uint8 RGB values to a Uint32;
     NOTE: Use SDL_LockSurface() on the surface before doing (a batch of)
     this call!  Use SDL_UnlockSurface() when you're done.
     SDL_MustLockSurface() can tell you whether a surface needs to be locked. */
  void (*putpixel)(SDL_Surface *, int, int, Uint32);

  /* XOR's the pixel at (x,y) location of the surface. */
  void (*xorpixel)(SDL_Surface *, int, int);

  /* Asks Tux Paint to play a sound (one loaded via SDL_mixer library);
     the first value is for left/right panning (0 is left, 128 is center,
     255 is right); the second value is for total volume (0 is off, 255 is
     loudest) */
  void (*playsound)(Mix_Chunk *, int, int);

  /* Asks Tux Paint whether a sound is currently being played (by 'playsound()') */
  int (*playingsound)(void);

  /* Asks Tux Paint to pause the sound being played by 'playsound()' */
  void (*pausesound)(void);

  /* Asks Tux Paint to resume (unpause) the sound being played by
     'playsound()' (if any) */
  void (*unpausesound)(void);

  /* Asks Tux Paint to stop playing the sound played by 'playsound()' */
  void (*stopsound)(void);

  /* Asks Tux Paint to calculate a line between (x1,y1) and (x2,y2);
     every 'step' iterations, it will call your callback function
     (which must accept a 'magic_api *' Magic API pointer and 'which' integer
     for which tool is being used, the 'last' and current ('canvas')
     SDL_Surfaces, and an (x,y) position) */
  void (*line)(void *, int, SDL_Surface *, SDL_Surface *, int, int, int, int, int, void (*)(void *, int, SDL_Surface *, SDL_Surface *, int, int));

  /* Returns whether the mouse button is down */
  int (*button_down)(void);

  /* Converts RGB bytes into HSV floats */
  void (*rgbtohsv)(Uint8, Uint8, Uint8, float *, float *, float *);

  /* Converts HSV floats into RGB bytes */
  void (*hsvtorgb)(float, float, float, Uint8 *, Uint8 *, Uint8 *);

  /* Holds Tux Paint's canvas dimensions */
  int canvas_w;
  int canvas_h;

  /* Returns a new surface containing the scaled contents of an input
     surface, scaled to, at maximum, w x h dimensions
     (keeping aspect ratio, if requested; check the return surface's
     'w' and 'h' elements to confirm the actual size) */
  SDL_Surface * (*scale)(SDL_Surface *, int, int, int);

  /* Returns a new surface containing the rotated/scaled contents of
     an input surface, rotated to r degrees, scaled to the w dimension and keeping its aspect ratio. */

  SDL_Surface * (*rotate_scale)(SDL_Surface *, int, int);

  /* Returns whether a particular position of the canvas has been labeled
     as 'touched,' since the mouse was first clicked; this function ALSO
     assigns the position as touched, until the next time the mouse is
     clicked; useful for not applying the same effect from 'last' to 'canvas'
     more than once per click-and-drag sequence */
  Uint8 (*touched)(int, int);

  /* Retracts the last undo buffer record; useful if a Magic tool has
     drawn something "temporary" (such as guides) onto the canvas during
     a previous click event. */
  void (*retract_undo)(void);
} magic_api;


/* The version of the Tux Paint Magic tool API you are being compiled
   against.  Your 'XYZ_api_version()' should return this value.
   If Tux Paint deems you compatible, it will call your 'XYZ_init()' (etc.)
   and you will be active. */

#define TP_MAGIC_API_VERSION 0x0000000B

#ifndef ATTRIBUTE_UNUSED
#define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#endif /* ATTRIBUTE_UNUSED */

/* Pre-defined Magic tool grouping codes */
enum {
  MAGIC_TYPE_DISTORTS,
  MAGIC_TYPE_COLOR_FILTERS,
  MAGIC_TYPE_PICTURE_WARPS,
  MAGIC_TYPE_PAINTING,
  MAGIC_TYPE_PATTERN_PAINTING,
  MAGIC_TYPE_PICTURE_DECORATIONS,
  MAGIC_TYPE_PROJECTIONS,
  MAGIC_TYPE_ARTISTIC
};

/* Magic-relevant Tux Paint features (which may be reported as disabled) */
/* (Uint8) */

#define MAGIC_FEATURE_CONTROL 0b00000001
#define MAGIC_FEATURE_SIZE    0b00000010

/* Magic complexity level requested by the user (allowing plugins to simplify) */

enum {
  MAGIC_COMPLEXITY_NOVICE,
  MAGIC_COMPLEXITY_BEGINNER,
  MAGIC_COMPLEXITY_ADVANCED,
  NUM_MAGIC_COMPLEXITY_LEVELS
};

#define MAGIC_COMPLEXITY_DEFAULT MAGIC_COMPLEXITY_ADVANCED

const char * MAGIC_COMPLEXITY_LEVEL_NAMES[NUM_MAGIC_COMPLEXITY_LEVELS] = {
  "novice",
  "beginner",
  "advanced",
};


#endif

