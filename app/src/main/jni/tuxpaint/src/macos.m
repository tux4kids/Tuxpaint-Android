/*
  macos.m

  Copyright (c) 2021-2022
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

  Last updated: December 11, 2022
*/

#import <stdio.h>
#import <stdlib.h>
#import <string.h>
#import <libintl.h>
#import <Cocoa/Cocoa.h>
#import "macos.h"
#import "debug.h"

#define MACOS_FONTS_PATH              "%s/Library/Fonts"
#define MACOS_PREFERENCES_PATH        "%s/Library/Application Support/TuxPaint"
#define MACOS_GLOBAL_PREFERENCES_PATH "/Library/Application Support/TuxPaint"
#define MACOS_PICTURES_PATH           "%s/Pictures"


static char *APPLE_LOCALE = NULL;


static void setupApplicationMenu(void)
{
    /*
    * Forked from SDLMain.m that comes with the SDL source code that comes with
    * MacPorts.  Credits to Darrell Walisser <dwaliss1@purdue.edu> and Max Horn
    * <max@quendi.de>.
    */

    /* warning: this code is very odd */
    NSMenu *appleMenu;
    NSMenuItem *menuItem;
    NSString *title;

    appleMenu = [[NSMenu alloc] initWithTitle:@""];

    /* Add menu items */
    title = [NSString stringWithUTF8String:gettext("About Tux Paint")];
    [appleMenu addItemWithTitle:title action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];

    [appleMenu addItem:[NSMenuItem separatorItem]];

    title = [NSString stringWithUTF8String:gettext("Hide Tux Paint")];
    [appleMenu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@"h"];

    menuItem = (NSMenuItem *)[appleMenu addItemWithTitle:[NSString stringWithUTF8String:gettext("Hide Others")] action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
    [menuItem setKeyEquivalentModifierMask:(NSAlternateKeyMask|NSCommandKeyMask)];

    [appleMenu addItemWithTitle:[NSString stringWithUTF8String:gettext("Show All")] action:@selector(unhideAllApplications:) keyEquivalent:@""];

    [appleMenu addItem:[NSMenuItem separatorItem]];

    title = [NSString stringWithUTF8String:gettext("Quit Tux Paint")];
    [appleMenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];


    /* Put menu into the menubar */
    menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:appleMenu];
    [[NSApp mainMenu] addItem:menuItem];

    /* Tell the application object that this is now the application menu */
    [NSApp setAppleMenu:appleMenu];

    /* Finally give up our references to the objects */
    [appleMenu release];
    [menuItem release];
}


static void setupWindowMenu(void)
{
    /*
    * Forked from SDLMain.m that comes with the SDL source code that comes with
    * MacPorts.  Credits to Darrell Walisser <dwaliss1@purdue.edu> and Max Horn
    * <max@quendi.de>.
    */

    NSMenu      *windowMenu;
    NSMenuItem  *windowMenuItem;
    NSMenuItem  *menuItem;

    windowMenu = [[NSMenu alloc] initWithTitle:[NSString stringWithUTF8String:gettext("Window")]];

    /* "Minimize" item */
    menuItem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:gettext("Minimize")] action:@selector(performMiniaturize:) keyEquivalent:@"m"];
    [windowMenu addItem:menuItem];
    [menuItem release];

    /* Put menu into the menubar */
    windowMenuItem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:gettext("Window")] action:nil keyEquivalent:@""];
    [windowMenuItem setSubmenu:windowMenu];
    [[NSApp mainMenu] addItem:windowMenuItem];

    /* Tell the application object that this is now the window menu */
    [NSApp setWindowsMenu:windowMenu];

    /* Finally give up our references to the objects */
    [windowMenu release];
    [windowMenuItem release];
}


static void removeSdlMenu(void)
{
    NSMenu* rootMenu = [NSApp mainMenu];

    /* SDL has two menus. Remove both. */

    [rootMenu removeItemAtIndex:0];
    [rootMenu removeItemAtIndex:0];
}


void apple_init(void)
{
    /* Override SDL's default menu with our gettext-translatable menu.  We do
     * this by removing the menus added by SDL, then adding ours. */
    removeSdlMenu();
    setupApplicationMenu();
    setupWindowMenu();
}


const char *apple_locale(void)
{
    if(!APPLE_LOCALE) {
        const char *locale = [[[NSLocale preferredLanguages] firstObject] UTF8String];

        /* Copy to writable memory */
        APPLE_LOCALE = strdup(locale);

        if(!APPLE_LOCALE) {
            perror("apple_locale");
            return "C";  /* Default to C */
        }

        /* Change the locale hyphen separator to underscore (e.g., en-US to en_US) */
        if(APPLE_LOCALE[2] == '-') {
            APPLE_LOCALE[2] = '_';
        }
    }

    DEBUG_PRINTF("locale=%s\n", APPLE_LOCALE);

    return APPLE_LOCALE;
}


const char *apple_fontsPath(void)
{
    static char *p = NULL;

    if(!p) {
        const char *home = getenv("HOME") ? getenv("HOME") : "";

        p = malloc(strlen(home) + strlen(MACOS_FONTS_PATH) + 1);

        if(p) sprintf(p, MACOS_FONTS_PATH, home);
        else perror("apple_fontsPath");
    }

    return p;
}


const char *apple_preferencesPath(void)
{
    static char *p = NULL;

    if(!p) {
        const char *home = getenv("HOME") ? getenv("HOME") : "";

        p = malloc(strlen(home) + strlen(MACOS_PREFERENCES_PATH) + 1);

        if(p) sprintf(p, MACOS_PREFERENCES_PATH, home);
        else perror("apple_preferencesPath");
    }

    return p;
}


const char *apple_globalPreferencesPath(void)
{
    return MACOS_GLOBAL_PREFERENCES_PATH;
}


const char *apple_picturesPath(void)
{
    static char *p = NULL;

    if(!p) {
        const char *home = getenv("HOME") ? getenv("HOME") : "";

        p = malloc(strlen(home) + strlen(MACOS_PICTURES_PATH) + 1);

        if(p) sprintf(p, MACOS_PICTURES_PATH, home);
        else perror("apple_picturesPath");
    }

    return p;
}


int apple_trash(const char *path)
{
    NSFileManager *manager = [NSFileManager defaultManager];
    NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
    NSURL *trash = nil;
    NSError *error = nil;

    [manager trashItemAtURL:url resultingItemURL:&trash error:&error];
    if(error) {
        DEBUG_PRINTF("%s\n", [[error localizedDescription] UTF8String]);
        return -1;
    }

    return 0;
}
