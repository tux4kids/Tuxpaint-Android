/*
 *  wrapperdata.h
 *  Tux Paint
 *
 *  Created by Martin Fuhrer on Wed May 12 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 *  $Id: wrapperdata.h,v 1.10 2008/02/20 05:32:21 mfuhrer Exp $
 *
 */

#ifndef WRAPPER_DATA
#define WRAPPER_DATA

struct WrapperDataStruct
{
    char dataPath[2048];                // path to data folder inside Tux Paint application bundle
    char preferencesPath[2048];         // path to the user's Tux Paint preferences folder
    char globalPreferencesPath[2048];   // path to all users' Tux Paint preferences folder
    char fontsPath[2048];               // path to the user's fonts folder
    int foundSDL;                       // was SDL.framework found?
    int foundSDL_image;                 // was SDL_image.framework found?
    int foundSDL_mixer;                 // was SDL_mixer.framework found?
	int cocoaKeystrokes;                // should keystrokes be intercepted by Cocoa wrapper?
    int menuAction;                     // was the action initiated by a Mac OS X menu selection?
};

typedef struct WrapperDataStruct WrapperData;

#endif
