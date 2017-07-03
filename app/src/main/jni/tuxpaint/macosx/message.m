//
//  message.m
//  Tux Paint
//
//  Created by Martin Fuhrer on Sat Dec 8 2007.
//  Copyright (c) 2007 Martin Fuhrer.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  (See COPYING.txt)
//

#import "SDLMain.h"
#import "message.h"

extern SDLMain* sdlMain;

void displayMessage( int msgId )
{
    NSString *message = nil;
    NSString *status = nil;
    
    if( sdlMain == nil ) 
        return;
    
    switch( msgId )
    {
        case( MSG_FONT_CACHE ):
            message = @"I'm currently fishing for fonts on your Mac.  This may take me a minute, as I'd much rather be feeding on fish from the sea.";
            status = @"Status: Caching fonts (this only needs to be done once)...";
            [sdlMain displayMessage:message andStatus:status withProgressIndicator:YES];
            break;
        default:
            break;
    }
}

void hideMessage()
{
    if( sdlMain == nil )
        return;
    
    [sdlMain hideMessage];
}