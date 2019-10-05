/*
 * FIXME
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "macos.h"

#define MACOS_FONTS_PATH              "%s/Library/Fonts"
#define MACOS_PREFERENCES_PATH        "%s/Library/Application Support/TuxPaint"
#define MACOS_GLOBAL_PREFERENCES_PATH "/Library/Application Support/TuxPaint"


/**
 * FIXME
 */
const char *macos_fontsPath(void)
{
  static char *p = NULL;

  if (!p)
    {
      const char *home = getenv("HOME");

      p = malloc(strlen(home) + strlen(MACOS_FONTS_PATH) + 1);

      if (p)
        sprintf(p, MACOS_FONTS_PATH, getenv("HOME"));
      else
        perror("macos_fontsPath");
    }

  return p;
}


/**
 * FIXME
 */
const char *macos_preferencesPath(void)
{
  static char *p = NULL;

  if (!p)
    {
      const char *home = getenv("HOME");

      p = malloc(strlen(home) + strlen(MACOS_PREFERENCES_PATH) + 1);

      if (p)
        sprintf(p, MACOS_PREFERENCES_PATH, getenv("HOME"));
      else
        perror("macos_preferencesPath");
    }

  return p;
}


/**
 * FIXME
 */
const char *macos_globalPreferencesPath(void)
{
  return MACOS_GLOBAL_PREFERENCES_PATH;
}
