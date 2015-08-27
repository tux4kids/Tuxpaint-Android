//
//  TransparentTextView.m
//  Tux Paint
//
//  Created by Martin Fuhrer on Wed Dec 12 2007.
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

#import "TransparentTextView.h"

@implementation TransparentTextView

- (void)drawViewBackgroundInRect:(NSRect)rect
{
}

- (BOOL)isOpaque
{
    return NO;
}

- (void)activateURLs
{    
	NSTextStorage* textStorage = [self textStorage];
	NSString* string = [textStorage string];
	NSRange searchRange = NSMakeRange(0, [string length]);
	NSRange foundRange;
    
	[textStorage beginEditing];
	do 
    {
		// assume that all URLs are enclosed between < > 
		foundRange = [string rangeOfString:@"<" options:0 range:searchRange];
        
		if (foundRange.length > 0)  //Did we find a URL?
        { 
            NSURL* theURL;
            NSMutableString* theURLString;
            NSDictionary* linkAttributes;
            NSRange endOfURLRange, range;
            
            // restrict the searchRange so that it won't find the same string again
            searchRange.location = foundRange.location + foundRange.length;
            searchRange.length = [string length] - searchRange.location;
            
            // assume the URL ends with >
            endOfURLRange = [string rangeOfString:@">" options:0 range:searchRange];
            
            // set foundRange's length to the length of the URL
            foundRange.location++;
            foundRange.length = endOfURLRange.location - foundRange.location;
            
            // grab the URL from the text and format it properly
            range = [[string substringWithRange:foundRange] rangeOfString:@"@"];
            if (range.length > 0)
                theURLString = [NSMutableString stringWithString:@"mailto:"];
            else
                theURLString = [NSMutableString stringWithString:@"http://"];
            [theURLString appendString:[string substringWithRange:foundRange]];
            
            // generate URL
            theURL = [NSURL URLWithString:theURLString];
            
            // make the link attributes
            linkAttributes = [NSDictionary dictionaryWithObjectsAndKeys: theURL, NSLinkAttributeName,
                [NSColor blueColor], NSForegroundColorAttributeName, nil];
            
            // apply those attributes to the URL in the text
            [textStorage addAttributes:linkAttributes range:foundRange];
		}
    } 
    while (foundRange.length != 0); //repeat the do block until it no longer finds anything
    
	[textStorage endEditing];
}

@end
