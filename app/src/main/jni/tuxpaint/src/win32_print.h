/* win32_print.h */

/* printing support for Tux Paint */
/* John Popplewell <john@johnnypops.demon.co.uk> */

/* Sept. 30, 2002 - Oct. 1, 2002 */
/* $Id: win32_print.h,v 1.9 2008/07/10 20:57:25 johnnypops Exp $ */


#ifndef __WIN32_PRINT_H__
#define __WIN32_PRINT_H__

#ifndef _SDL_H
#include "SDL.h"
#endif

/* if printcfg is NULL, uses the default printer */
extern const char *SurfacePrint(SDL_Surface * surf,
				const char *printcfg, int showdialog);
extern int IsPrinterAvailable(void);

/* additional windows functions requiring <windows.h> */
extern char *GetDefaultSaveDir(const char *suffix);
extern char *GetSystemFontDir(void);
extern char *get_temp_fname(const char *const name);

/* keyboard hooking functions */
extern int  InstallKeyboardHook(void);
extern int  RemoveKeyboardHook(void);
extern void SetActivationState(int state);

#endif
