//
//  speech.m
//  TuxPaint
//
//  Created by Martin Fuhrer on 13/12/07.
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

#import <Cocoa/Cocoa.h>
#import "speech.h"
#import "i18n.h"

void speak_string(const wchar_t *widecharString)
{
    #ifndef __APPLE_10_2_8__
    char multibyteString[1024];
    NSString *string = [NSString string];
    
    // speech synthesizer can pronounce only English phonemes and syllables
    int lang = get_current_language();
    if( lang != LANG_EN && lang != LANG_EN_GB && lang != LANG_EN_ZA )
        return;
    
    NSArray *voices = [NSSpeechSynthesizer availableVoices];
    NSSpeechSynthesizer *synthesizer = [[NSSpeechSynthesizer alloc] init];
    
    wcstombs(multibyteString,widecharString,sizeof(multibyteString));
    if( [string respondsToSelector:@selector(string:stringWithCString:encoding:)] )
        string = [NSString stringWithCString:multibyteString encoding:NSUTF8StringEncoding];
    else
        string = [NSString stringWithCString:multibyteString];
    
    // speak string using a random voice
    [synthesizer setVoice:[voices objectAtIndex:rand()%[voices count]]];
    [synthesizer startSpeakingString:string];
    [synthesizer release];
    #endif // !__APPLE_10_2_8__
}