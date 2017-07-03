//
//  macosx_print.m
//  Tux Paint
//
//  Created by Darrell Walisser on Sat Mar 15 2003.
//  Modified by Martin Fuhrer 2007.
//  Copyright (c) 2007 Darrell Walisser, Martin Fuhrer.
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
//  $Id: macosx_print.m,v 1.9 2014/03/19 23:39:18 wkendrick Exp $
//

#import "macosx_print.h"
#import "wrapperdata.h"
#import <Cocoa/Cocoa.h>

extern WrapperData macosx;
NSData* printData = nil;

// this object presents the image to the printing layer
@interface ImageView : NSView
{
    NSImage* _image;
}
- (void) setImage:(NSImage*)image;
@end

@implementation ImageView

- (void) setImage:(NSImage*)image
{
    _image = [ image retain ];
}

- (void) drawRect:(NSRect)rect
{
    [ _image compositeToPoint: NSMakePoint( 0, 0 ) operation: NSCompositeCopy ];
}

- (BOOL) scalesWhenResized
{
	return YES;
}

@end

// this object waits for the print dialog to go away
@interface ModalDelegate : NSObject
{
    BOOL	_complete;
    BOOL    _wasOK;
}
- (id)   init;
- (BOOL) wait;
- (void) reset;
- (BOOL) wasOK;
@end

@implementation ModalDelegate

- (id) init
{
    self = [ super init ];
    _complete = NO;
    _wasOK = NO;
    return self;
}

- (BOOL) wait
{
    while (!_complete) {
        NSEvent *event;
        event = [ NSApp nextEventMatchingMask:NSAnyEventMask
                        untilDate:[ NSDate distantFuture ]
                        inMode: NSDefaultRunLoopMode dequeue:YES ];
        [ NSApp sendEvent:event ];
    }
    
    return [ self wasOK ];
}

- (void) reset
{
    _complete = NO;
    _wasOK = NO;
}

- (BOOL) wasOK
{
    return _wasOK;
}

- (void)printDidRun:(NSPrintOperation *)printOperation 
            success:(BOOL)success contextInfo:(void *)contextInfo
{
    _complete = YES;
    _wasOK = success;
}

- (void)pageLayoutEnded:(NSPageLayout *)pageLayout 
             returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    _complete = YES;
    _wasOK = returnCode == NSOKButton;
}

@end

static NSImage* CreateImage( SDL_Surface *surface )
{
    NSBitmapImageRep* imageRep;
    NSSize            imageSize;
    NSImage*          image;
    SDL_Surface*      surface32RGBA;

    // convert surface to 32bit RGBA
#ifdef BIG_ENDIAN_ARCH
    surface32RGBA = SDL_CreateRGBSurface( SDL_SWSURFACE, surface->w, surface->h,
                                          32, 0xff<<24, 0xff<<16, 0xff<<8, 0xff<<0 );
#else
    surface32RGBA = SDL_CreateRGBSurface( SDL_SWSURFACE, surface->w, surface->h,
                                          32, 0xff<<0, 0xff<<8, 0xff<<16, 0xff<<24 );
#endif
    if( surface32RGBA == NULL ) {
        NSLog (@"CreateImage: Cannot allocate conversion surface");
        return nil;
    }
    
    SDL_BlitSurface( surface, NULL, surface32RGBA, NULL );
    
    // convert surface to an NSBitmapImageRep
    imageRep = [ [ NSBitmapImageRep alloc] 
                    initWithBitmapDataPlanes:(unsigned char **)&surface32RGBA->pixels 
                        pixelsWide:surface->w 
                        pixelsHigh:surface->h 
                        bitsPerSample:8
                        samplesPerPixel:4 
                        hasAlpha:YES
                        isPlanar:NO
                        colorSpaceName:NSDeviceRGBColorSpace 
                        bytesPerRow:surface->w * 4
                        bitsPerPixel:32 ];
    if( imageRep == nil ) {
        NSLog (@"CreateImage: Could not create image representation.");
        return nil;
    }
    
    imageSize = NSMakeSize( surface->w, surface->h );
    
    image = [ [ NSImage alloc ] initWithSize:imageSize ];
    if( image == nil ) {
        NSLog (@"CreateImage: Could not allocate image");
        return nil;
    }
    
    [ image addRepresentation:imageRep ];
	[ image setScalesWhenResized:YES ];
	[ image setDataRetained:YES ];
    
    [ image autorelease ];
    [ imageRep release ];
    free( surface32RGBA );
    
    return image;
}

void DefaultPrintSettings( const SDL_Surface *surface, NSPrintInfo *printInfo )
{
    if( surface->w > surface->h )
        [ printInfo setOrientation:NSLandscapeOrientation ];
    else
        [ printInfo setOrientation:NSPortraitOrientation ];
    
	[ printInfo setHorizontallyCentered:true ];
	[ printInfo setVerticallyCentered:true ];
	[ printInfo setVerticalPagination:NSFitPagination ];
	[ printInfo setHorizontalPagination:NSFitPagination ];    
}

NSPrintInfo* LoadPrintInfo( const SDL_Surface *surface )
{
    NSUserDefaults*   standardUserDefaults;
    NSPrintInfo*      printInfo;
    NSData*           printData = nil;
    static BOOL       firstTime = YES;   
    
    standardUserDefaults = [ NSUserDefaults standardUserDefaults ];
    
    if( standardUserDefaults ) 
        printData = [ standardUserDefaults dataForKey:@"PrintInfo" ];
   
    if( printData )
        printInfo = (NSPrintInfo*)[ NSUnarchiver unarchiveObjectWithData:printData ]; 
    else
    {
        printInfo = [ NSPrintInfo sharedPrintInfo ];
        if( firstTime == YES ) 
        {
            DefaultPrintSettings( surface, printInfo );
            firstTime = NO;
        }
    }
    
    return printInfo;
}

void SavePrintInfo( NSPrintInfo* printInfo )
{
    NSUserDefaults*   standardUserDefaults;
    NSData*           printData = nil;
    
    printData = [ NSArchiver archivedDataWithRootObject:printInfo ];
    standardUserDefaults = [ NSUserDefaults standardUserDefaults ];
    
    if( standardUserDefaults ) 
        [ standardUserDefaults setObject:printData forKey:@"PrintInfo" ];
}

int DisplayPageSetup( const SDL_Surface * surface )
{
	NSPageLayout*     pageLayout;
	NSPrintInfo*      printInfo;
	ModalDelegate*    delegate;
    BOOL              result;
    
    macosx.cocoaKeystrokes = 1;
    
    printInfo = LoadPrintInfo( surface );

    delegate = [ [ [ ModalDelegate alloc ] init ] autorelease ];
	pageLayout = [ NSPageLayout pageLayout ];
	[ pageLayout beginSheetWithPrintInfo:printInfo 
                 modalForWindow:[ NSApp mainWindow ]
                 delegate:delegate
                 didEndSelector:@selector(pageLayoutEnded:returnCode:contextInfo:)
                 contextInfo:nil ];
	
	result = [ delegate wait ];
    SavePrintInfo( printInfo );
    
    macosx.cocoaKeystrokes = 0;
    
    return (int)( result );
}

const char* SurfacePrint( SDL_Surface *surface, int showDialog )
{
    NSImage*          image;
    ImageView*        printView;
    NSWindow*         printWindow;
    NSPrintOperation* printOperation;
    NSPrintInfo*      printInfo;
    ModalDelegate*    delegate;
    BOOL              ok = YES;

    // check if printers are available
    NSArray* printerNames = [NSPrinter printerNames];
    if( [printerNames count] == 0 && !showDialog)
        return "No printer is available.  Run Tux Paint in window mode (not fullscreen), and select File > Print... to choose a printer.";
    
    // create image for surface
    image = CreateImage( surface );
    if( image == nil )
        return "Could not create a print image.";
	
    // create print control objects
    printInfo = LoadPrintInfo( surface );
     
	NSRect pageRect = [ printInfo imageablePageBounds ];
	NSSize pageSize = pageRect.size;
	NSPoint pageOrigin = pageRect.origin;
	
	[ printInfo setTopMargin:pageOrigin.y ];
	[ printInfo setLeftMargin:pageOrigin.x ];
	[ printInfo setRightMargin:pageOrigin.x ];
	[ printInfo setBottomMargin:pageOrigin.y ];
	
	float surfaceRatio = (float)( surface->w ) / (float)( surface->h );
	float pageRatio = pageSize.width / pageSize.height;
	
	NSSize imageSize = pageSize;
	if( pageRatio > surfaceRatio )   // wide page
	    imageSize.width = surface->w * pageSize.height / surface->h;
	else  // tall page
		imageSize.height = surface->h * pageSize.width / surface->w;
    
	// create print view
    printView = [ [ [ ImageView alloc ] initWithFrame: NSMakeRect( 0, 0, imageSize.width, imageSize.height ) ] autorelease ];
	if (printView == nil)
        return "Could not create a print view.";
    
	[ image setSize:imageSize ];
    [ printView setImage:image ];
        
    // run printing
    printOperation = [ NSPrintOperation printOperationWithView:printView printInfo:printInfo ];
    [ printOperation setShowsPrintPanel:showDialog ];   //EP replaced setShowPanels by setShowsPrintPanel
    
    macosx.cocoaKeystrokes = 1;
    delegate = [ [ [ ModalDelegate alloc ] init ] autorelease ];
    [ printOperation runOperationModalForWindow:[ NSApp mainWindow ]
        delegate:delegate didRunSelector:@selector(printDidRun:success:contextInfo:) contextInfo:nil ];
    
    ok = [ delegate wait ];
        
    macosx.cocoaKeystrokes = 0;
    
    SavePrintInfo( printInfo );
    [ image release ];
	    
    return NULL;
}

