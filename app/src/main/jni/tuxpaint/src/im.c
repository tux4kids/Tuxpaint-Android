/*
  im.c

  Input method handling
  Copyright (c)2007 by Mark K. Kim and others

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

  $Id: im.c,v 1.14 2014/03/19 23:39:18 wkendrick Exp $
*/

/*
* See the LANGUAGE-SPECIFIC IM FUNCTIONS section for instructions on adding
* support for new languages.
*
* This file is called IM (Input Method), but it's actually an Input Translator.
* This implementation was sort of necessary in order to work without having to
* modify SDL.
*
* Basically, to read in text in foreign language, read Keysym off of SDL and
* pass to im_read.  im_read will translate the text and pass the unicode string
* back to you.  But before all this is done, be sure to create the IM_DATA
* structure and initialize it with the proper language translator you want to use.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>  
#include "im.h"

#ifdef __ANDROID__
#include "android_mbstowcs.h"
#endif


/* ***************************************************************************
* I18N GETTEXT
*/

#ifndef gettext_noop
#define gettext_noop(s) (s)
#endif


enum {
  IM_TIP_NONE,
  IM_TIP_ENGLISH,
  IM_TIP_HIRAGANA,
  IM_TIP_KATAKANA,
  IM_TIP_HANGUL,
  IM_TIP_THAI,
  IM_TIP_ZH_TW,
  NUM_IM_TIPS
};


static const char* const im_tip_text[NUM_IM_TIPS] =
{
  NULL,

  // Input Method: English mode
  gettext_noop("English"),

  // Input Method: Japanese Romanized Hiragana mode
  gettext_noop("Hiragana"),

  // Input Method: Japanese Romanized Katakana mode
  gettext_noop("Katakana"),

  // Input Method: Korean Hangul 2-Bul mode
  gettext_noop("Hangul"),

  // Input Method: Thai mode
  gettext_noop("Thai"),

  // Input Method: Traditional Chinese mode
  gettext_noop("ZH_TW")
};


/* ***************************************************************************
* CONSTANTS
*/

/* #define IM_DEBUG       1 */

#define MAX_SECTIONS     8    /* Maximum numbers of sections in *.im file */
#define MAX_UNICODE_SEQ 16    /* Output of state machine, including NUL */
#define INITIAL_SMSIZE   8    /* Initial num of transitions in STATE_MACHINE */

#ifndef LANG_DEFAULT
#define LANG_DEFAULT   (LANG_EN)
#endif


/**
* Event types that im_event_*() functions need to handle.
*/
enum {
  IM_REQ_TRANSLATE,    /* The ever-more important IM translation request */
  IM_REQ_INIT,         /* Initialization request */
  IM_REQ_RESET_SOFT,   /* Soft reset request */
  IM_REQ_RESET_FULL,   /* Full reset request */
  IM_REQ_FREE,         /* Free resources */
  NUM_IM_REQUESTS
};


/**
* Match statuses.
*/
enum {
  MATCH_STAT_NONE       = 0x00,
  MATCH_STAT_NOMOSTATES = 0x01,
  MATCH_STAT_NOMOBUF    = 0x02,
};


/* ***************************************************************************
* TYPES
*/

/**
* All im_event_*() functions have this type.
*/
typedef int (*IM_EVENT_FN)(IM_DATA*, SDL_Event);   /* IM_EVENT_FN type */


/**
* State Machine key-value pair for transition control.  When the "key"
* is pressed, transition is made to "state".
*
* @see STATE_MACHINE
*/
typedef struct SM_WITH_KEY {
  char key;
  struct STATE_MACHINE* state;
} SM_WITH_KEY;


/**
* A State Machine is used to map key strokes to the unicode output.
* A single State Machine has a possible output (the unicode) and pointers
* to next states.  The "next state" is determined by the key stroke
* pressed by the user - this key is looked up in SM_WITH_KEY and
* its next state determined by the STATE_MACHINE pointer in SM_WITH_KEY.
*
* The number of possible transitions to the next state is dynamically
* adjustable using the parameter next_maxsize.  The actual storage in
* use can be determined via next_size.
*
* @see SM_WITH_KEY
*/
typedef struct STATE_MACHINE {
  wchar_t output[MAX_UNICODE_SEQ];
  char flag;

  SM_WITH_KEY* next;      /* Possible transitions */
  size_t next_maxsize;    /* Potential size of the next pointer */
  size_t next_size;       /* Used size of the next pointer */
} STATE_MACHINE;


/**
* A Character Map loads the *.im file, which may have several "sections".
* Each section has its own state machine, and the C code determines which
* section is used in determining which STATE_MACHINE to use for the
* key mapping.
*/
typedef struct {
  STATE_MACHINE sections[MAX_SECTIONS];
  int section;

  /* These variables get populated when a search is performed */
  int match_count;              /* How many char seq was used for output */
  int match_is_final;           /* T/F - tells if match is final */
  int match_stats;              /* Statistics gathering */
  STATE_MACHINE* match_state;
  STATE_MACHINE* match_state_prev;
} CHARMAP;


/* ***************************************************************************
* STATIC GLOBALS
*/

/**
* Global initialization flag.
*/
static int im_initialized = 0;


/**
* Language-specific IM event-handler function pointers.  This lookup table
* is initialized in im_init().  Every support language should have a pointer
* mapped here.
*
* @see im_init()
* @see im_read()
*/
static IM_EVENT_FN im_event_fns[NUM_LANGS];


/* ***************************************************************************
* UTILITY FUNCTIONS
*/

#define MIN(a,b)              ((a)<=(b) ? (a) : (b))
#define IN_RANGE(a,v,b)       ( (a)<=(v) && (v)<(b) )
#define ARRAYLEN(a)           ( sizeof(a)/sizeof(*(a)) )


static void wcs_lshift(wchar_t* s, size_t count)
{
  wchar_t* dest = s;
  wchar_t* src = s+count;
  size_t len = wcslen(src)+1;   /* Copy over all src string + NUL */

  memmove(dest, src, len * sizeof(wchar_t));
}


/**
* Pull out "count" characters from the back.
*/
static void wcs_pull(wchar_t* s, size_t count)
{
  int peg = (int)wcslen(s) - (int)count;
  if(peg < 0) peg = 0;

  s[peg] = L'\0';
}


/* ***************************************************************************
* STATE_MACHINE FUNCTIONS
*/

/**
* Compare two SM_WITH_KEY, return appropriate result.
*/
static int swk_compare(const void* swk1, const void* swk2)
{
  SM_WITH_KEY* sk1 = (SM_WITH_KEY*)swk1;
  SM_WITH_KEY* sk2 = (SM_WITH_KEY*)swk2;

  return (sk1->key) - (sk2->key);
}


/**
* Initialize the State Machine.
*/
static int sm_init(STATE_MACHINE* sm)
{
  memset(sm, 0, sizeof(STATE_MACHINE));

  sm->next = calloc(INITIAL_SMSIZE, sizeof(SM_WITH_KEY));
  if(!sm->next) {
    perror("sm_init");
    return 1;
  }

  sm->next_maxsize = INITIAL_SMSIZE;
  return 0;
}


/**
* Free the State Machine resources.
*/
static void sm_free(STATE_MACHINE* sm)
{
  if(sm->next) {
    int i = 0;

    for(i = 0; i < (int)sm->next_maxsize; i++) {
      STATE_MACHINE* next_state = sm->next[i].state;
      if(next_state) sm_free(next_state);
      sm->next[i].state = NULL;
    }

    free(sm->next);
    sm->next = NULL;
  }

  memset(sm, 0, sizeof(STATE_MACHINE));
}


/**
* Double the storage space of the possible transition states.
*/
static int sm_dblspace(STATE_MACHINE* sm)
{
  size_t newsize = sm->next_maxsize * 2;
  SM_WITH_KEY* next = realloc(sm->next, sizeof(SM_WITH_KEY) * newsize);

  if(next == NULL) {
    perror("sm_dblspace");
    return 1;
  }

  sm->next = next;
  sm->next_maxsize = newsize;
  return 0;
}


/**
* Search the state machine's transition keys, return pointer to the next state.
* Return NULL if none is found.  The search is done only at 1 level, and does
* not recurse deep.
*/
static STATE_MACHINE* sm_search_shallow(STATE_MACHINE* sm, char key)
{
  SM_WITH_KEY smk = { key, NULL };
  SM_WITH_KEY* smk_found;

  smk_found = bsearch(
      &smk, sm->next, sm->next_size, sizeof(SM_WITH_KEY), swk_compare);

  if(!smk_found) return NULL;
  return smk_found->state;
}


/**
* Search the state machine's transition keys, return the unicode output of the
* last state found.  The search is done deep, recursing until no more match
* can be found.
*
* @param start    Starting point of the state transition.  Constant.
* @param key      The key string to look for.  Constant.
* @param matched  The number of character strings matched.  Return on output.
* @param end      The last state found.  Return on output.
* @param penult   The penultimate state found.
*
* @return         Found unicode character sequence output of the last state.
*/
static const wchar_t* sm_search(STATE_MACHINE* start, wchar_t* key, int* matched, STATE_MACHINE** penult, STATE_MACHINE** end)
{
  STATE_MACHINE* sm = sm_search_shallow(start, (char)*key);
  const wchar_t* unicode;

  /* No match - stop recursion */
  if(!sm) {
    *matched = 0;
    *end = start;

    return start->output;
  }

  /* Match - recurse */
  *penult = start;
  unicode = sm_search(sm, key+1, matched, penult, end);
  (*matched)++;

  return unicode;
}


/**
* Sort the state machine's transition keys so it can be binary-searched.
* The sort is done only at 1 level, and does not recurse deep.
*/
static void sm_sort_shallow(STATE_MACHINE* sm)
{
  qsort(sm->next, sm->next_size, sizeof(SM_WITH_KEY), swk_compare);
}


/**
* Add a single sequence-to-unicode path to the state machine.
*/
static int sm_add(STATE_MACHINE* sm, char* seq, const wchar_t* unicode, char flag)
{
  STATE_MACHINE* sm_found = sm_search_shallow(sm, seq[0]);

  /* Empty sequence */
  if(seq[0] == '\0') {
    if(wcslen(sm->output)) {
      size_t i;

      fprintf(stderr, "Unicode sequence ");
      for(i = 0; i < wcslen(sm->output); i++) fprintf(stderr, "%04X ", (int)sm->output[i]);
      fprintf(stderr, " already defined, overriding with ");
      for(i = 0; i < wcslen(unicode); i++) fprintf(stderr, "%04X ", (int)unicode[i]);
      fprintf(stderr, "\n");
    }
    wcscpy(sm->output, unicode);
    sm->flag = flag;
    return 0;
  }

  /* The key doesn't exist yet */
  if(!sm_found) {
    int index = (int)sm->next_size;
    SM_WITH_KEY* next = &sm->next[index];

    /* Add the key */
    next->key = seq[0];
    next->state = malloc(sizeof(STATE_MACHINE));
    if(!next->state) {
      perror("sm_add");
      return 1;
    }
    sm_init(next->state);

    /* Increase store for next time, if necessary */
    if(++(sm->next_size) >= sm->next_maxsize) {
      if(sm_dblspace(sm)) {
        fprintf(stderr, "Memory expansion failure\n");
        return 1;
      }
    }

    sm_found = next->state;
  }

  /* Recurse */
  sm_add(sm_found, seq+1, unicode, flag);

  /* Sort the states */
  sm_sort_shallow(sm);

  return 0;
}



/* ***************************************************************************
* CHARMAP FUNCTIONS
*/

/**
* Initialize the character map table.
*/
static int charmap_init(CHARMAP* cm)
{
  int error_code = 0;
  int i = 0;

  memset(cm, 0, sizeof(CHARMAP));

  for(i = 0; i < MAX_SECTIONS; i++) {
    error_code += sm_init(&cm->sections[i]);
  }

  return error_code;
}


/**
* Add a character-sequence-to-unicode mapping to the character map.
*
* @param cm      Character map to which to add the mapping.
* @param section The section of the character map to add the mapping.
* @param seq     The character sequence to which to add the mapping.
* @param unicode The unicode of the character sequence.
* @param flag    The flag associated with this state, if any.
*
* @return        0 if no error, 1 if error.
*/
static int charmap_add(CHARMAP* cm, int section, char* seq, const wchar_t* unicode, char* flag)
{
  if(section >= MAX_SECTIONS) {
    fprintf(stderr, "Section count exceeded\n");
    return 1;
  }

  /* For now, we only utilize one-character flags */
  if(strlen(flag) > 1) {
    fprintf(stderr, "%04X: Multi-character flag, truncated.\n", (int)(intptr_t)unicode);        //EP added (intptr_t) to avoid warning on x64
  }

  return sm_add(&cm->sections[section], seq, unicode, flag[0]);
}


/**
* Load the character map table from a file.
*
* @param cm     Character Map to load the table into.
* @param path   The path of the file to load.
* @return       Zero if the file is loaded fine, nonzero otherwise.
*/
static int charmap_load(CHARMAP* cm, const char* path)
{
  FILE* is = NULL;
  int section = 0;
  int error_code = 0;

  /* Open */
  is = fopen(path, "rt");
  if(!is) {
    perror("path");
    return 1;
  }

  /* Load */
  while(!feof(is)) {
    wchar_t unicode[MAX_UNICODE_SEQ];
    int ulen = 0;

    char buf[256];
    char flag[256];

    int scanned = 0;

    /* Scan a single token first */
    scanned = fscanf(is, "%255s", buf);
    if(scanned < 0) break;
    if(scanned == 0) {
      fprintf(stderr, "%s: Character map syntax error\n", path);
      return 1;
    }

    /* Handle the first argument */
    if(strcmp(buf, "section") == 0) {    /* Section division */
      section++;
      continue;
    }
    else if(buf[0] == '#') {             /* Comment */
      fscanf(is, "%*[^\n]");
      continue;
    }
    else {
      char* bp = buf;
      int u;

      do {
        if(sscanf(bp, "%x", &u) == 1) {   /* Unicode */
          unicode[ulen++] = u;
        }
        else {
          fprintf(stderr, "%s: Syntax error at '%s'\n", path, buf);
          return 1;
        }

        bp = strchr(bp, ':');
        if(bp) bp++;
      } while(bp && ulen < MAX_UNICODE_SEQ-1);
      unicode[ulen] = L'\0';
    }

    /* Scan some more */
    scanned = fscanf(is, "%255s\t%255s", buf, flag);
    if(scanned < 0) break;

    /* Input count checking */
    switch(scanned) {
      case 0: case 1:
        fprintf(stderr, "%s: Character map syntax error\n", path);
        return 1;

      default:
        if(charmap_add(cm, section, buf, unicode, flag)) {
          size_t i = 0;

#ifndef __BEOS__
#if defined __GLIBC__ && __GLIBC__ == 2 && __GLIBC_MINOR__ >=2 || __GLIBC__ > 2 || __APPLE__
          fwprintf(stderr, L"Unable to add sequence '%ls', unicode ", buf);
          for(i = 0; i < wcslen(unicode); i++) fwprintf(stderr, L"%04X ", (int)unicode[i]);
          fwprintf(stderr, L"in section %d\n", section);
#endif
#endif

          error_code = 1;
        }
    }
  }

  /* Close */
  fclose(is);

  return error_code;
}


/**
* Free the resources used by a character map.
*/
static void charmap_free(CHARMAP* cm)
{
  int i;

  for(i = 0; i < MAX_SECTIONS; i++) {
    sm_free(&cm->sections[i]);
  }

  memset(cm, 0, sizeof(CHARMAP));
}


/**
* Search for a matching character string in the character map.
*/
static const wchar_t* charmap_search(CHARMAP* cm, wchar_t* s)
{
  STATE_MACHINE* start;
  const wchar_t* unicode;
  int section;

  /* Determine the starting state based on the charmap's active section */
  section = cm->section;
  if(!IN_RANGE(0, section, (int)ARRAYLEN(cm->sections))) section = 0;
  start = &cm->sections[section];

  cm->match_state = NULL;
  cm->match_state_prev = NULL;
  unicode = sm_search(start, s, &cm->match_count, &cm->match_state_prev, &cm->match_state);

  /**
  * Determine whether the match is final.  A match is considered to be final
  * in two cases: (1)if the last state mached has no exit paths, or (2)if we
  * did not consume all of the search string.  (1) is obvious - if there are
  * no more states to transition to, then the unicode we find is the final
  * code.  (2) means we reached the final state that can be the only
  * interpretation of the input string, so it must be the final state.
  * If neither of these is true, that means further input from the user
  * may allow us to get to a different state, so we have not reached the
  * final state we possibly can.
  */
  cm->match_is_final = 0;
  if(cm->match_count < (int)wcslen(s)) {
    cm->match_is_final = 1;
  }

  /* Statistics */
  cm->match_stats = MATCH_STAT_NONE;
  if(cm->match_state->next_size == 0) {
    cm->match_is_final = 1;
    cm->match_stats |= MATCH_STAT_NOMOSTATES;
  }
  if(cm->match_count == (int)wcslen(s)) {
    cm->match_stats |= MATCH_STAT_NOMOBUF;
  }

  return unicode;
}


/* ***************************************************************************
* GENERIC IM FUNCTIONS
*/

/**
* Default C IM event handler.
*
* @see im_read
*/
static int im_event_c(IM_DATA* im, SDL_Event event)
{
  SDL_Keysym ks = event.key.keysym;

  /* Handle event requests */
  im->s[0] = L'\0';
  if(im->request != IM_REQ_TRANSLATE) return 0;

  /* Handle key stroke */
  switch(ks.sym) {
    case SDLK_BACKSPACE: im->s[0] = L'\b'; im->s[1] = L'\0'; break;
    case SDLK_TAB:       im->s[0] = L'\t'; im->s[1] = L'\0'; break;
    case SDLK_RETURN:    im->s[0] = L'\r'; im->s[1] = L'\0'; break;
  default:             mbstowcs(im->s , event.text.text, 16);
	  //default:             wcsncpy(im->s , event.text.text, 16);
  }

  //im->s[1] = L'\0';
  printf("im->s %ls, event.text.text %s\n", im->s, event.text.text);
  im->buf[0] = L'\0';

  return 0;
}

/* ***************************************************************************
* PUBLIC IM FUNCTIONS
*/

/**
* IM-process a character.  This function simply looks up the language from
* IM and calls the appropriate im_event_<lang>() language-specific IM event
* handler.  im_event_c() is called by default if no language-specific
* function is specified for the specified language.
*
* @param im  IM-processed data to return to the caller function.
* @param ks  SDL_keysym typed on the keyboard.
*
* @return    The number of characters in im->s that should not be committed.
*            In other words, the returned number of characters at the end of
*            im->s should be overwritten the next time im_read is called.
*
* @see im_event_c()
* @see im_event_fns
*/
int im_read(IM_DATA* im, SDL_Event event)
{
  IM_EVENT_FN im_event_fp = NULL;
  int redraw = 0;

  /* FIXME SDL2: This adds text to events that are not meant to carry text.
     Right procedure would be to modify the different im_event_LL() functions,
     but for now letting as this as it is what the functions expect. */
  switch (event.key.keysym.sym)
    {
    case SDLK_BACKSPACE:
      event.text.text[0] = L'\b';
      event.text.text[1] = L'\0';
      break;
    case SDLK_RETURN:
      event.text.text[0] = L'\r';
      event.text.text[1] = L'\0';
      break;
    case SDLK_TAB:
      event.text.text[0] = L'\t';
      event.text.text[1] = L'\0';
      break;
    }

  /* Sanity check */
  if(im->lang < 0 || im->lang >= NUM_LANGS) {
    fprintf(stderr, "im->lang out of range (%d), using default\n", im->lang);
    im->lang = LANG_DEFAULT;
  }

  /* Function pointer to the language-specific im_event_* function */
  im_event_fp = im_event_fns[im->lang];

  /* Run the language-specific IM or run the default C IM */
  if(im_event_fp) redraw = (*im_event_fp)(im, event);
  else redraw = im_event_c(im, event);

  #ifdef IM_DEBUG
  wprintf(L"* [%8ls] [%8ls] %2d %2d (%2d)\n", im->s, im->buf, wcslen(im->s), wcslen(im->buf), im->redraw);
  #endif

  return redraw;
}

/* ***************************************************************************
* OTHER STATIC IM FUNCTIONS
*/

/**
* Generic event handler that calls the appropriate language handler.
* im->request should have the event ID.
*/
static void im_event(IM_DATA* im)
{
  SDL_Event event;

  event.key.keysym.sym = 0;
  event.text.text[0] = '\0';

  im_read(im, event);
}


/**
* Make an event request and call the event handler.
*/
static void im_request(IM_DATA* im, int request)
{
  im->request = request;
  im_event(im);
  im->request = IM_REQ_TRANSLATE;
}

/* ***************************************************************************
* PUBLIC IM FUNCTIONS
*/

/**
* Free any allocated resources.
*/
static void im_free(IM_DATA* im)
{
  im_request(im, IM_REQ_FREE);
}


void im_softreset(IM_DATA* im)
{
  im->s[0] = L'\0';
  im->buf[0] = L'\0';

  im_request(im, IM_REQ_RESET_SOFT);
}


static void im_fullreset(IM_DATA* im)
{
  im->s[0] = L'\0';
  im->buf[0] = L'\0';

  im_request(im, IM_REQ_RESET_FULL);
}







/* ***************************************************************************
* LANGUAGE-SPECIFIC IM FUNCTIONS
*
* If you want to add a new language support, add the main code to this
* section.  More specifically, do the following:
*
*   1) Add im_event_<lang>() function to this section.  Use the existing
*      im_event_* functions as models, and feel free to use the state-machine
*      character map engine (CHARMAP struct) but do not feel obligated to
*      do so.  The CHARMAP engine exists for the programmer's benefit, to
*      make it easier to support complex languages.
*
*   2) Update the im_init() functions so that it initializes im_event_fns[]
*      with a pointer to your im_event_<lang>() function.
*
*   3) Create <lang>.im in the "im" directory, if you use the CHARMAP engine.
*      Your code is what loads this file so you should already know to do this
*      step if you have already written a working im_event_<lang>() function
*      that uses CHARMAP, but I explicitly write out this instruction for
*      those trying to figure out the relationship of <lang>.im to this IM
*      framework.
*
*   4) Increase MAX_SECTION if your language needs more sections in <lang>.im
*
*   5) Increase INITIAL_SMSIZE if your <lang>.im is huginormous and takes too
*      long to load.  I can't think of any reason why this would happen unless
*      you're writing a Chinese IM with a significant characters of the
*      language represented, but the code as-is is somewhat lacking when it
*      comes to writing a Chinese IM (need some way to show a dropdown box
*      from the main app - same problem with Korean Hanja and Japanese Kanji
*      inputs, but this isn't meant to be a complex IM framework so I think
*      we're safe for Hanja and Kanji.)  Do this with caution because
*      changing INITIAL_SMSIZE will affect the memory consumption of all IM
*      functions.
*/

/**
* Chinese Traditional IM.
*
* @see im_read
*/
static int im_event_zh_tw(IM_DATA* im, SDL_Event event)
{
  SDL_Keysym ks = event.key.keysym;
  static const char* lang_file = IMDIR "zh_tw.im";
  enum { SEC_ENGLISH, SEC_ZH_TW, SEC_TOTAL };

  static CHARMAP cm;


  /* Handle event requests */
  switch(im->request) {
    case 0: break;

    case IM_REQ_FREE:        /* Free allocated resources */
      charmap_free(&cm);
      /* go onto full reset */

    case IM_REQ_RESET_FULL:  /* Full reset */
      cm.section = SEC_ENGLISH;
      im->tip_text = im_tip_text[IM_TIP_ENGLISH];
      /* go onto soft reset */

    case IM_REQ_RESET_SOFT:  /* Soft reset */
      im->s[0] = L'\0';
      im->buf[0] = L'\0';
      im->redraw = 0;
      cm.match_count = 0;
      cm.match_is_final = 0;
      cm.match_state = &cm.sections[cm.section];
      cm.match_state_prev = &cm.sections[cm.section];
      break;

    case IM_REQ_INIT:        /* Initialization */
      charmap_init(&cm);

      if(charmap_load(&cm, lang_file)) {
        fprintf(stderr, "Unable to load %s, defaulting to im_event_c\n", lang_file);
        im->lang = LANG_DEFAULT;
        return im_event_c(im, event);
      }

      im_fullreset(im);

      #ifdef DEBUG
      printf("IM: Loaded '%s'\n", lang_file);
      #endif
      break;
  }
  if(im->request != IM_REQ_TRANSLATE) return 0;


  /* Discard redraw characters, so they can be redrawn */
  if((int)wcslen(im->s) < im->redraw) im->redraw = wcslen(im->s);
  wcs_lshift(im->s, (wcslen(im->s) - im->redraw) );


  /* Handle keys */
  switch(ks.sym) {
    /* Keys to ignore */
    case SDLK_NUMLOCKCLEAR: case SDLK_CAPSLOCK: case SDLK_SCROLLLOCK:
    case SDLK_LSHIFT:  case SDLK_RSHIFT:
    case SDLK_LCTRL:   case SDLK_RCTRL:
    case SDLK_LGUI:   case SDLK_RGUI:
    case SDLK_MENU:
    case SDLK_MODE:    case SDLK_APPLICATION:
      break;

    /* Left-Alt & Right-Alt mapped to mode-switch */
    case SDLK_RALT:    case SDLK_LALT:
      cm.section = (++cm.section % SEC_TOTAL);   /* Change section */
      im_softreset(im);                          /* Soft reset */

      /* Set tip text */
      switch(cm.section) {
        case SEC_ENGLISH:  im->tip_text = im_tip_text[IM_TIP_ENGLISH]; break;
        case SEC_ZH_TW: im->tip_text = im_tip_text[IM_TIP_ZH_TW]; break;
      }
      break;

    /* Enter finalizes previous redraw */
    case SDLK_RETURN:
      if(im->redraw <= 0) {
        im->s[0] = L'\r';
        im->s[1] = L'\0';
      }
      im->buf[0] = L'\0';
      im->redraw = 0;
      break;

    /* Actual character processing */
    default:
	if (event.type == SDL_TEXTINPUT|| ks.sym == SDLK_BACKSPACE|| ks.sym == SDLK_RETURN || ks.sym == SDLK_TAB)
	  {
      /* English mode */
      if(cm.section == SEC_ENGLISH) {
	mbstowcs(im->s , event.text.text, 16);
//        im->s[0] = event.text.text[0];
//        im->s[1] = L'\0';
        im->buf[0] = L'\0';
      }
      /* ZH_TW mode */
      else 
	{
      wchar_t u = event.text.text[0];

	  im->s[0] = L'\0';                     /* Zero-out output string */
	  wcsncat(im->buf, &u, 1);              /* Copy new character */

	  /* Translate the characters */
	  im->redraw = 0;
	  while(1) {
	    const wchar_t* us = charmap_search(&cm, im->buf);
            #ifdef IM_DEBUG
	    wprintf(L"  [%8ls] [%8ls] %2d %2d\n", im->s, im->buf, wcslen(im->s), wcslen(im->buf));
            #endif

	    /* Match was found? */
	    if(us && wcslen(us)) {
              #ifdef IM_DEBUG
	      wprintf(L"    1\n");
              #endif

	      wcscat(im->s, us);

	      /* Final match */
	      if(cm.match_is_final) {
		wcs_lshift(im->buf, cm.match_count);
		cm.match_count = 0;
		cm.match_is_final = 0;
	      }
	      /* May need to be overwritten next time */
	      else {
		im->redraw += wcslen(us);
		break;
	      }
	    }
	    /* No match, but more data is in the buffer */
	    else if(wcslen(im->buf) > 0) {
	      /* If the input character has no state, it's its own state */
	      if(cm.match_count == 0) {
                #ifdef IM_DEBUG
		wprintf(L"    2a\n");
                #endif
		wcsncat(im->s, im->buf, 1);
		wcs_lshift(im->buf, 1);
		cm.match_is_final = 0;
	      }
	      /* If the matched characters didn't consume all, it's own state */
	      else if((size_t)cm.match_count != wcslen(im->buf)) {
                #ifdef IM_DEBUG
		wprintf(L"    2b (%2d)\n", cm.match_count);
                #endif
		wcsncat(im->s, im->buf, 1);
		wcs_lshift(im->buf, 1);
		cm.match_is_final = 0;
	      }
	      /* Otherwise it's just a part of a future input */
	      else {
                #ifdef IM_DEBUG
		wprintf(L"    2c (%2d)\n", cm.match_count);
                #endif
		wcscat(im->s, im->buf);
		cm.match_is_final = 0;
		im->redraw += wcslen(im->buf);
		break;
	      }
	    }
	    /* No match and no more data in the buffer */
	    else {
              #ifdef IM_DEBUG
	      wprintf(L"    3\n");
              #endif
	      break;
	    }

	    /* Is this the end? */
	    if(cm.match_is_final) break;
	  }
	}
  }
  }

  return im->redraw;
}


/**
* Thai IM.
*
* @see im_read
*/
static int im_event_th(IM_DATA* im, SDL_Event event)
{
  SDL_Keysym ks = event.key.keysym;
  static const char* lang_file = IMDIR "th.im";
  enum { SEC_ENGLISH, SEC_THAI, SEC_TOTAL };

  static CHARMAP cm;


  /* Handle event requests */
  switch(im->request) {
    case 0: break;

    case IM_REQ_FREE:        /* Free allocated resources */
      charmap_free(&cm);
      /* go onto full reset */

    case IM_REQ_RESET_FULL:  /* Full reset */
      cm.section = SEC_ENGLISH;
      im->tip_text = im_tip_text[IM_TIP_ENGLISH];
      /* go onto soft reset */

    case IM_REQ_RESET_SOFT:  /* Soft reset */
      im->s[0] = L'\0';
      im->buf[0] = L'\0';
      im->redraw = 0;
      cm.match_count = 0;
      cm.match_is_final = 0;
      cm.match_state = &cm.sections[cm.section];
      cm.match_state_prev = &cm.sections[cm.section];
      break;

    case IM_REQ_INIT:        /* Initialization */
      charmap_init(&cm);

      if(charmap_load(&cm, lang_file)) {
        fprintf(stderr, "Unable to load %s, defaulting to im_event_c\n", lang_file);
        im->lang = LANG_DEFAULT;
        return im_event_c(im, event);
      }

      im_fullreset(im);

      #ifdef DEBUG
      printf("IM: Loaded '%s'\n", lang_file);
      #endif
      break;
  }
  if(im->request != IM_REQ_TRANSLATE) return 0;


  /* Discard redraw characters, so they can be redrawn */
  if((int)wcslen(im->s) < im->redraw) im->redraw = wcslen(im->s);
  wcs_lshift(im->s, (wcslen(im->s) - im->redraw) );


  /* Handle keys */
  switch(ks.sym) {
    /* Keys to ignore */
    case SDLK_NUMLOCKCLEAR: case SDLK_CAPSLOCK: case SDLK_SCROLLLOCK:
    case SDLK_LSHIFT:  case SDLK_RSHIFT:
    case SDLK_LCTRL:   case SDLK_RCTRL:
    case SDLK_LALT:
    case SDLK_LGUI:   case SDLK_RGUI:
    case SDLK_MENU:
    case SDLK_MODE:    case SDLK_APPLICATION:
      break;

    /* Right-Alt mapped to mode-switch */
    case SDLK_RALT:
      cm.section = (++cm.section % SEC_TOTAL);   /* Change section */
      im_softreset(im);                          /* Soft reset */

      /* Set tip text */
      switch(cm.section) {
        case SEC_ENGLISH:  im->tip_text = im_tip_text[IM_TIP_ENGLISH]; break;
        case SEC_THAI: im->tip_text = im_tip_text[IM_TIP_THAI]; break;
      }
      break;

    /* Enter finalizes previous redraw */
    case SDLK_RETURN:
      if(im->redraw <= 0) {
        im->s[0] = L'\r';
        im->s[1] = L'\0';
      }
      im->buf[0] = L'\0';
      im->redraw = 0;
      break;

    /* Actual character processing */
    default:
      /* English mode */
      if(cm.section == SEC_ENGLISH) {
	//        im->s[0] = event.text.text[0];
	mbstowcs(im->s , event.text.text, 16);
        //im->s[1] = L'\0';
        im->buf[0] = L'\0';
      }
      /* Thai mode */
      else {
        wchar_t u = event.text.text[0];

        im->s[0] = L'\0';                     /* Zero-out output string */
        wcsncat(im->buf, &u, 1);              /* Copy new character */

        /* Translate the characters */
        im->redraw = 0;
        while(1) {
          const wchar_t* us = charmap_search(&cm, im->buf);
          #ifdef IM_DEBUG
          wprintf(L"  [%8ls] [%8ls] %2d %2d\n", im->s, im->buf, wcslen(im->s), wcslen(im->buf));
          #endif

          /* Match was found? */
          if(us && wcslen(us)) {
            #ifdef IM_DEBUG
            wprintf(L"    1\n");
            #endif

            wcscat(im->s, us);

            /* Final match */
            if(cm.match_is_final) {
              wcs_lshift(im->buf, cm.match_count);
              cm.match_count = 0;
              cm.match_is_final = 0;
            }
            /* May need to be overwritten next time */
            else {
              im->redraw += wcslen(us);
              break;
            }
          }
          /* No match, but more data is in the buffer */
          else if(wcslen(im->buf) > 0) {
            /* If the input character has no state, it's its own state */
            if(cm.match_count == 0) {
              #ifdef IM_DEBUG
              wprintf(L"    2a\n");
              #endif
              wcsncat(im->s, im->buf, 1);
              wcs_lshift(im->buf, 1);
              cm.match_is_final = 0;
            }
            /* If the matched characters didn't consume all, it's own state */
            else if((size_t)cm.match_count != wcslen(im->buf)) {
              #ifdef IM_DEBUG
              wprintf(L"    2b (%2d)\n", cm.match_count);
              #endif
              wcsncat(im->s, im->buf, 1);
              wcs_lshift(im->buf, 1);
              cm.match_is_final = 0;
            }
            /* Otherwise it's just a part of a future input */
            else {
              #ifdef IM_DEBUG
              wprintf(L"    2c (%2d)\n", cm.match_count);
              #endif
              wcscat(im->s, im->buf);
              cm.match_is_final = 0;
              im->redraw += wcslen(im->buf);
              break;
            }
          }
          /* No match and no more data in the buffer */
          else {
            #ifdef IM_DEBUG
            wprintf(L"    3\n");
            #endif
            break;
          }

          /* Is this the end? */
          if(cm.match_is_final) break;
        }
      }
  }

  return im->redraw;
}


/**
* Japanese IM.
*
* @see im_read
*/
static int im_event_ja(IM_DATA* im, SDL_Event event)
{
  SDL_Keysym ks = event.key.keysym;
  static const char* lang_file = IMDIR "ja.im";
  enum { SEC_ENGLISH, SEC_HIRAGANA, SEC_KATAKANA, SEC_TOTAL };

  static CHARMAP cm;


  /* Handle event requests */
  switch(im->request) {
    case 0: break;

    case IM_REQ_FREE:        /* Free allocated resources */
      charmap_free(&cm);
      /* go onto full reset */

    case IM_REQ_RESET_FULL:  /* Full reset */
      cm.section = SEC_ENGLISH;
      im->tip_text = im_tip_text[IM_TIP_ENGLISH];
      /* go onto soft reset */

    case IM_REQ_RESET_SOFT:  /* Soft reset */
      im->s[0] = L'\0';
      im->buf[0] = L'\0';
      im->redraw = 0;
      cm.match_count = 0;
      cm.match_is_final = 0;
      cm.match_state = &cm.sections[cm.section];
      cm.match_state_prev = &cm.sections[cm.section];
      break;

    case IM_REQ_INIT:        /* Initialization */
      charmap_init(&cm);

      if(charmap_load(&cm, lang_file)) {
        fprintf(stderr, "Unable to load %s, defaulting to im_event_c\n", lang_file);
        im->lang = LANG_DEFAULT;
        return im_event_c(im, event);
      }

      im_fullreset(im);

      #ifdef DEBUG
      printf("IM: Loaded '%s'\n", lang_file);
      #endif
      break;
  }
  if(im->request != IM_REQ_TRANSLATE) return 0;


  /* Discard redraw characters, so they can be redrawn */
  if((int)wcslen(im->s) < im->redraw) im->redraw = wcslen(im->s);
  wcs_lshift(im->s, (wcslen(im->s) - im->redraw) );


  /* Handle keys */
  switch(ks.sym) {
    /* Keys to ignore */
    case SDLK_NUMLOCKCLEAR: case SDLK_CAPSLOCK: case SDLK_SCROLLLOCK:
    case SDLK_LSHIFT:  case SDLK_RSHIFT:
    case SDLK_LCTRL:   case SDLK_RCTRL:
    case SDLK_LALT:
    case SDLK_LGUI:   case SDLK_RGUI:
    case SDLK_MENU:
    case SDLK_MODE:    case SDLK_APPLICATION:
      break;

    /* Right-Alt mapped to mode-switch */
    case SDLK_RALT:
      cm.section = (++cm.section % SEC_TOTAL);   /* Change section */
      im_softreset(im);                          /* Soft reset */

      /* Set tip text */
      switch(cm.section) {
        case SEC_ENGLISH:  im->tip_text = im_tip_text[IM_TIP_ENGLISH]; break;
        case SEC_HIRAGANA: im->tip_text = im_tip_text[IM_TIP_HIRAGANA]; break;
        case SEC_KATAKANA: im->tip_text = im_tip_text[IM_TIP_KATAKANA]; break;
      }
      break;

    /* Enter finalizes previous redraw */
    case SDLK_RETURN:
      if(im->redraw <= 0) {
        im->s[0] = L'\r';
        im->s[1] = L'\0';
      }
      im->buf[0] = L'\0';
      im->redraw = 0;
      break;

    /* Actual character processing */
    default:
      if (event.type == SDL_TEXTINPUT|| ks.sym == SDLK_BACKSPACE|| ks.sym == SDLK_RETURN || ks.sym == SDLK_TAB)
	{
      /* English mode */
      if(cm.section == SEC_ENGLISH) {
	mbstowcs(im->s , event.text.text, 16);
//        im->s[0] = event.text.text[0];
//      im->s[1] = L'\0';
        im->buf[0] = L'\0';
      }
      /* Hiragana and Katakana modes */
      else {
        wchar_t u = event.text.text[0];

        im->s[0] = L'\0';                     /* Zero-out output string */
        wcsncat(im->buf, &u, 1);              /* Copy new character */

        /* Translate the characters */
        im->redraw = 0;
        while(1) {
          const wchar_t* us = charmap_search(&cm, im->buf);
          #ifdef IM_DEBUG
          wprintf(L"  [%8ls] [%8ls] %2d %2d\n", im->s, im->buf, wcslen(im->s), wcslen(im->buf));
          #endif

          /* Match was found? */
          if(us && wcslen(us)) {
            #ifdef IM_DEBUG
            wprintf(L"    1\n");
            #endif

            wcscat(im->s, us);

            /* Final match */
            if(cm.match_is_final) {
              wcs_lshift(im->buf, cm.match_count);
              cm.match_count = 0;
              cm.match_is_final = 0;
            }
            /* May need to be overwritten next time */
            else {
              im->redraw += wcslen(us);
              break;
            }
          }
          /* No match, but more data is in the buffer */
          else if(wcslen(im->buf) > 0) {
            /* If the input character has no state, it's its own state */
            if(cm.match_count == 0) {
              #ifdef IM_DEBUG
              wprintf(L"    2a\n");
              #endif
              wcsncat(im->s, im->buf, 1);
              wcs_lshift(im->buf, 1);
              cm.match_is_final = 0;
            }
            /* If the matched characters didn't consume all, it's own state */
            else if((size_t)cm.match_count != wcslen(im->buf)) {
              #ifdef IM_DEBUG
              wprintf(L"    2b (%2d)\n", cm.match_count);
              #endif
              wcsncat(im->s, im->buf, 1);
              wcs_lshift(im->buf, 1);
              cm.match_is_final = 0;
            }
            /* Otherwise it's just a part of a future input */
            else {
              #ifdef IM_DEBUG
              wprintf(L"    2c (%2d)\n", cm.match_count);
              #endif
              wcscat(im->s, im->buf);
              cm.match_is_final = 0;
              im->redraw += wcslen(im->buf);
              break;
            }
          }
          /* No match and no more data in the buffer */
          else {
            #ifdef IM_DEBUG
            wprintf(L"    3\n");
            #endif
            break;
          }

          /* Is this the end? */
          if(cm.match_is_final) break;
        }
      }
  }
  }
  return im->redraw;
}


/**
* Korean IM helper function to tell whether a character typed will produce
* a vowel.
*
* @see im_event_ko
*/
static int im_event_ko_isvowel(CHARMAP* cm, wchar_t c)
{
  STATE_MACHINE *start, *next;
  const wchar_t* unicode;
  int section;

  /* Determine the starting state based on the charmap's active section */
  section = cm->section;
  if(!IN_RANGE(0, section, (int)ARRAYLEN(cm->sections))) section = 0;
  start = &cm->sections[section];

  next = sm_search_shallow(start, (char)c);
  unicode = next ? next->output : NULL;

  return (unicode && wcslen(unicode) == 1 && 0x314F <= unicode[0] && unicode[0] <= 0x3163);
}


/**
* Korean IM.
*
* @see im_read
*/
static int im_event_ko(IM_DATA* im, SDL_Event event)
{
  SDL_Keysym ks = event.key.keysym;
  static const char* lang_file = IMDIR "ko.im";
  enum { SEC_ENGLISH, SEC_HANGUL, SEC_TOTAL };

  static CHARMAP cm;


  /* Handle event requests */
  switch(im->request) {
    case 0: break;

    case IM_REQ_FREE:        /* Free allocated resources */
      charmap_free(&cm);
      /* go onto full reset */

    case IM_REQ_RESET_FULL:  /* Full reset */
      cm.section = SEC_ENGLISH;
      im->tip_text = im_tip_text[IM_TIP_ENGLISH];
      /* go onto soft reset */

    case IM_REQ_RESET_SOFT:  /* Soft reset */
      im->s[0] = L'\0';
      im->buf[0] = L'\0';
      im->redraw = 0;
      cm.match_count = 0;
      cm.match_is_final = 0;
      cm.match_state = &cm.sections[cm.section];
      cm.match_state_prev = &cm.sections[cm.section];
      break;

    case IM_REQ_INIT:        /* Initialization */
      charmap_init(&cm);

      if(charmap_load(&cm, lang_file)) {
        fprintf(stderr, "Unable to load %s, defaulting to im_event_c\n", lang_file);
        im->lang = LANG_DEFAULT;
        return im_event_c(im, event);
      }

      im_fullreset(im);

      #ifdef DEBUG
      printf("IM: Loaded '%s'\n", lang_file);
      #endif
      break;
  }
  if(im->request != IM_REQ_TRANSLATE) return 0;


  /* Discard redraw characters, so they can be redrawn */
  if((int)wcslen(im->s) < im->redraw) im->redraw = wcslen(im->s);
  wcs_lshift(im->s, (wcslen(im->s) - im->redraw) );


  /* Handle keys */
  switch(ks.sym) {
    /* Keys to ignore */
    case SDLK_NUMLOCKCLEAR: case SDLK_CAPSLOCK: case SDLK_SCROLLLOCK:
    case SDLK_LSHIFT:  case SDLK_RSHIFT:
    case SDLK_LCTRL:   case SDLK_RCTRL:
    case SDLK_LGUI:   case SDLK_RGUI:
    case SDLK_MENU:
    case SDLK_MODE:    case SDLK_APPLICATION:
      break;

    /* Right-Alt mapped to mode-switch */
    case SDLK_LALT: case SDLK_RALT:
      cm.section = (++cm.section % SEC_TOTAL);   /* Change section */
      im_softreset(im);                          /* Soft reset */

      /* Set tip text */
      switch(cm.section) {
        case SEC_ENGLISH: im->tip_text = im_tip_text[IM_TIP_ENGLISH]; break;
        case SEC_HANGUL:  im->tip_text = im_tip_text[IM_TIP_HANGUL]; break;
      }
      break;

    /* Backspace removes only a single buffered character */
    case SDLK_BACKSPACE:
      /* Delete one buffered character */
      if(wcslen(im->buf) > 0) {
        wcs_pull(im->buf, 1);
        if(im->redraw > 0) im->redraw--;
        event.text.text[0] = L'\0';
      }
      /* continue processing: */

    /* Actual character processing */
    default:
      if (event.type == SDL_TEXTINPUT|| ks.sym == SDLK_BACKSPACE|| ks.sym == SDLK_RETURN || ks.sym == SDLK_TAB)
	{
      /* English mode */
      if(cm.section == SEC_ENGLISH) {
	mbstowcs(im->s , event.text.text, 16);
        im->buf[0] = L'\0';
      }
      /* Hangul mode */
      else {
        wchar_t u = event.text.text[0];
        wchar_t* bp = im->buf;

        im->s[0] = L'\0';                     /* Zero-out output string */
        wcsncat(bp, &u, 1);                   /* Copy new character */

        /* Translate the characters */
        im->redraw = 0;
        while(1) {
          const wchar_t* us = charmap_search(&cm, bp);
          #ifdef IM_DEBUG
          wprintf(L"  [%8ls] [%8ls] %2d %2d\n", im->s, im->buf, wcslen(im->s), wcslen(im->buf));
          #endif

          /* Match was found? */
          if(us && wcslen(us)) {
            /* Final match */
            if(cm.match_is_final) {
              /* Batchim may carry over to the next character */
              if(cm.match_state->flag == 'b') {
                wchar_t next_char = bp[cm.match_count];

                /* If there is no more buffer, output it */
                if(cm.match_stats & MATCH_STAT_NOMOBUF) {
                  #ifdef IM_DEBUG
                  wprintf(L"    1a\n");
                  #endif

                  wcscat(im->s, us);          /* Output */
                  im->redraw += wcslen(us);  /* May need to re-eval next time */
                  bp += cm.match_count;       /* Keep buffer data for re-eval*/
                  cm.match_count = 0;
                  cm.match_is_final = 0;
                }
                /* If there is buffer data but it's not vowel, finalize it */
                else if(!im_event_ko_isvowel(&cm, next_char)) {
                  #ifdef IM_DEBUG
                  wprintf(L"    1b\n");
                  #endif

                  wcscat(im->s, us);     /* Output */
                  wcs_lshift(bp, cm.match_count);
                  cm.match_count = 0;
                  cm.match_is_final = 0;
                }
                /* If there is buffer and it's vowel, re-eval */
                else {
                  #ifdef IM_DEBUG
                  wprintf(L"    1c\n");
                  #endif

                  us = cm.match_state_prev->output;
                  wcscat(im->s, us);      /* Output */
                  cm.match_count--;       /* Matched all but one */
                  cm.match_is_final = 0;
                  wcs_lshift(bp, cm.match_count);
                }
              }
              /* No batchim - this is final */
              else {
                #ifdef IM_DEBUG
                wprintf(L"    1d\n");
                #endif

                wcscat(im->s, us);
                wcs_lshift(bp, cm.match_count);
                cm.match_count = 0;
                cm.match_is_final = 0;
              }
            }
            /* May need to be overwritten next time */
            else {
              #ifdef IM_DEBUG
              wprintf(L"    1e\n");
              #endif

              wcscat(im->s, us);
              im->redraw += wcslen(us);
              break;
            }
          }
          /* No match, but more data is in the buffer */
          else if(wcslen(bp) > 0) {
            /* If the input character has no state, it's its own state */
            if(cm.match_count == 0) {
              #ifdef IM_DEBUG
              wprintf(L"    2a\n");
              #endif
              wcsncat(im->s, bp, 1);
              wcs_lshift(bp, 1);
              cm.match_is_final = 0;
            }
            /* If the matched characters didn't consume all, it's own state */
            else if((size_t)cm.match_count != wcslen(bp)) {
              #ifdef IM_DEBUG
              wprintf(L"    2b (%2d)\n", cm.match_count);
              #endif
              wcsncat(im->s, bp, 1);
              wcs_lshift(bp, 1);
              cm.match_is_final = 0;
            }
            /* Otherwise it's just a part of a future input */
            else {
              #ifdef IM_DEBUG
              wprintf(L"    2c (%2d)\n", cm.match_count);
              #endif
              wcscat(im->s, bp);
              cm.match_is_final = 0;
              im->redraw += wcslen(bp);
              break;
            }
          }
          /* No match and no more data in the buffer */
          else {
            #ifdef IM_DEBUG
            wprintf(L"    3\n");
            #endif
            break;
          }

          /* Is this the end? */
          if(cm.match_is_final) break;
        }
      }
  }
  }

  return im->redraw;
}


/* ***************************************************************************
* PUBLIC IM FUNCTIONS
*/

/**
* Initialize the IM_DATA structure.
*
* @param im    IM_DATA structure to initialize.
* @param lang  LANG_* defined constant to initialize the structure with.
*/
void im_init(IM_DATA* im, int lang)
{
  /* Free already allocated resources if initialized before */
  if(im_initialized) {
    im_free(im);
  }

  /* Initialize */
  memset(im, 0, sizeof(IM_DATA));
  im->lang = lang;

  /* Setup static globals */
  if(!im_initialized) {
    /* ADD NEW LANGUAGE SUPPORT HERE */
    im_event_fns[LANG_JA] = &im_event_ja;
    im_event_fns[LANG_KO] = &im_event_ko;
    im_event_fns[LANG_TH] = &im_event_th;
    im_event_fns[LANG_ZH_TW] = &im_event_zh_tw;

    im_initialized = 1;
  }

  #ifdef DEBUG
  assert(0 <= im->lang && im->lang < NUM_LANGS);
  if(im_initialized) printf("Initializing IM for %s...\n", lang_prefixes[im->lang]);
  #endif

  /* Initialize the individual IM */
  im_request(im, IM_REQ_INIT);
}




/* vim:ts=2:et
*/
