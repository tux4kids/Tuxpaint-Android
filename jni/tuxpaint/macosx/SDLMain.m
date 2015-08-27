/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
*/

#import "SDL.h"
#import "SDLMain.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>

#include <Security/Authorization.h>
#include <Security/AuthorizationTags.h>
#include <fontconfig/fontconfig.h>
#include <stdlib.h>

#import "macosx_print.h"
#import "message.h"
#import "wrapperdata.h"

/* For some reaon, Apple removed setAppleMenu from the headers in 10.4,
but the method still is there and works. To avoid warnings, we declare
it ourselves here. */
@interface NSApplication(SDL_Missing_Methods)
- (void)setAppleMenu:(NSMenu *)menu;
@end

/* Use this flag to determine whether we use SDLMain.nib or not */
#define	    SDL_USE_NIB_FILE	1

/* Use this flag to determine whether we use CPS (docking) or not */
#define	    SDL_USE_CPS		1

#ifdef SDL_USE_CPS
/* Portions of CPS.h */
typedef struct CPSProcessSerNum
{
	UInt32		lo;
	UInt32		hi;
} CPSProcessSerNum;

extern OSErr	CPSGetCurrentProcess( CPSProcessSerNum *psn);
extern OSErr 	CPSEnableForegroundOperation( CPSProcessSerNum *psn, UInt32 _arg2, UInt32 _arg3, UInt32 _arg4, UInt32 _arg5);
extern OSErr	CPSSetFrontProcess( CPSProcessSerNum *psn);

#endif /* SDL_USE_CPS */

static int    gArgc;
static char  **gArgv;
static BOOL   gFinderLaunch;
static BOOL   gCalledAppMainline = FALSE;

WrapperData macosx; 
SDLMain *sdlMain;

static NSString *getApplicationName(void)
{
    NSDictionary *dict;
    NSString *appName = 0;
	
    /* Determine the application name */
    dict = (NSDictionary *)CFBundleGetInfoDictionary(CFBundleGetMainBundle());
    if (dict)
        appName = [dict objectForKey: @"CFBundleName"];
    
    if (![appName length])
        appName = [[NSProcessInfo processInfo] processName];
	
    return appName;
}

#if SDL_USE_NIB_FILE
/* A helper category for NSString */
@interface NSString (ReplaceSubString)
- (NSString *)stringByReplacingRange:(NSRange)aRange with:(NSString *)aString;
@end
#endif

@interface SDLApplication : NSApplication
@end

@implementation SDLApplication

- (void)sendEvent:(NSEvent *)anEvent 
{	
	if (!macosx.cocoaKeystrokes)
	{
		if (NSKeyDown == [anEvent type] || NSKeyUp == [anEvent type])
		{
			if( ( [anEvent modifierFlags] & NSCommandKeyMask ) == 0 ) 
            {
				return;		// do not intercept keystrokes intended for SDL layer
            }
		}
	}
	[super sendEvent: anEvent];
}

/* Invoked from the Quit menu item */
- (void)terminate:(id)sender
{
    /* Post a SDL_QUIT event */
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

- (void)tuxpaintHelp:(id)sender
{
	NSString* helpPath = [[NSBundle mainBundle] pathForResource:@"README" ofType:@"html" inDirectory:@"html"];
	[[NSWorkspace sharedWorkspace] openFile:helpPath];
}
@end


/* Class to pass information from Cocoa to SDL application */
@interface CocoaToSDLBridge : NSObject {}
- (void)dataPath:(NSString *)directory;
- (void)preferencesPath;
- (void)fontsPath;
@end

@implementation CocoaToSDLBridge

-(void) dataPath:(NSString *)directory;
{
    NSBundle *mainBundle;
    NSString *path;

    mainBundle = [NSBundle mainBundle];
    path = [mainBundle pathForResource:@"data" ofType:nil];

    [path getCString:(macosx.dataPath) maxLength:sizeof(macosx.dataPath) encoding:NSUTF8StringEncoding];        //EP added maxLength: and encoding: to avoid deprecation warning for 10.6
}

-(void) preferencesPath;
{
    NSString *path;

    path = [@"~/Library/Application Support/TuxPaint" stringByExpandingTildeInPath];
    [path getCString:(macosx.preferencesPath) maxLength:sizeof(macosx.preferencesPath) encoding:NSUTF8StringEncoding];  //EP added maxLength: and encoding: to avoid deprecation warning for 10.6
        
    path = @"/Library/Application Support/TuxPaint";
    [path getCString:(macosx.globalPreferencesPath) maxLength:sizeof(macosx.globalPreferencesPath) encoding:NSUTF8StringEncoding];      //EP added maxLength: and encoding: to avoid deprecation warning for 10.6

}

-(void) fontsPath;
{
    NSString *path;

    path = [@"~/Library/Fonts" stringByExpandingTildeInPath];
    [path getCString:(macosx.fontsPath) maxLength:sizeof(macosx.fontsPath) encoding:NSUTF8StringEncoding];      //EP added maxLength: and encoding: to avoid deprecation warning for 10.6
}

@end


/* The main class of the application, the application's delegate */
@implementation SDLMain

- (IBAction) onAbout:(id)sender
{
    NSBundle *mainBundle = [NSBundle mainBundle];
    NSDictionary *bundleInfo = [mainBundle infoDictionary];
    NSMutableString *string;
    NSMutableAttributedString *attributedString;
    NSMutableDictionary *attributes;
    
    /* string attributes */
    NSMutableParagraphStyle *paragraphStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
    [paragraphStyle setAlignment:NSCenterTextAlignment];
    attributes = [NSMutableDictionary dictionary];
    [attributes setObject:[NSFont boldSystemFontOfSize:12] forKey:NSFontAttributeName];
    [attributes setObject:paragraphStyle forKey:NSParagraphStyleAttributeName];
    [paragraphStyle release];
    
    /* display Tux Paint */
    string = [bundleInfo objectForKey:@"CFBundleName"];
    attributedString = [[NSAttributedString alloc] initWithString:string attributes:attributes];
    [appnameText setAttributedStringValue:attributedString];
    [attributedString release];
    
    /* display version */
    string = [NSMutableString stringWithString:@"Version "];
    [string appendString:[bundleInfo objectForKey:@"CFBundleShortVersionString"]];
    [string appendString:@" ("];
    [string appendString:[bundleInfo objectForKey:@"CFBundleVersion"]];
    [string appendString:@")"];
    [versionText setStringValue:string];
    
    /* display credits */
    NSString *filePath = [mainBundle pathForResource:@"credits" ofType:@"txt"];
    string = [NSString stringWithContentsOfFile:filePath];
    [attributes setObject:[NSFont systemFontOfSize:10] forKey:NSFontAttributeName];
    attributedString = [[NSMutableAttributedString alloc] initWithString:string attributes:attributes];
    [[acknowledgmentsText textStorage] setAttributedString:attributedString];
    [attributedString release];
    [acknowledgmentsText activateURLs];
    [acknowledgmentsText setEditable:NO];

    [aboutWindow makeKeyAndOrderFront:sender];
}


- (IBAction) onNew:(id)sender
{
	[self sendSDLControlKeystroke:SDLK_n];
}

- (IBAction) onOpen:(id)sender
{
	[self sendSDLControlKeystroke:SDLK_o];
}

- (IBAction) onSave:(id)sender
{
	[self sendSDLControlKeystroke:SDLK_s];
}

- (IBAction) onPrint:(id)sender
{
    macosx.menuAction = 1;
    [self sendSDLControlKeystroke:SDLK_p];
    macosx.menuAction = 0;
}

- (IBAction) onPageSetup:(id)sender
{
    [self sendSDLControlShiftKeystroke:SDLK_p];
}

- (IBAction) onUndo:(id)sender
{
	[self sendSDLControlKeystroke:SDLK_z];
}

- (IBAction) onRedo:(id)sender
{
	[self sendSDLControlKeystroke:SDLK_r];
}

- (IBAction)onHelp:(id)sender
{
	NSString* helpPath = [[NSBundle mainBundle] pathForResource:@"README" ofType:@"html" inDirectory:@"html"];
	[[NSWorkspace sharedWorkspace] openFile:helpPath];
}

- (IBAction) onQuit:(id)sender
{
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    /* Post a SDL_QUIT event */
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

- (void) sendSDLControlKeystroke:(int)key
{
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = key;
	event.key.keysym.mod = KMOD_CTRL;
	SDL_PushEvent(&event);	
}

- (void) sendSDLControlShiftKeystroke:(int)key
{
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = key;
	event.key.keysym.mod = KMOD_CTRL | KMOD_SHIFT;
	SDL_PushEvent(&event);	
}

/* Set the working directory to the .app's parent directory */
- (void) setupWorkingDirectory:(BOOL)shouldChdir
{
    if (shouldChdir)
    {
        char parentdir[MAXPATHLEN];
		CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
		CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
		if (CFURLGetFileSystemRepresentation(url2, true, (UInt8 *)parentdir, MAXPATHLEN)) {
	        assert ( chdir (parentdir) == 0 );   /* chdir to the binary app's parent */
		}
		CFRelease(url);
		CFRelease(url2);
	}	
}

- (void) displayMessage:(NSString*)message andStatus:(NSString*)status withProgressIndicator:(BOOL)progress
{
    [messageText setStringValue:message];
    [messageStatus setStringValue:status];
    [messagePanel makeKeyAndOrderFront:nil]; 
    if (progress)
    {
        [messageProgress setUsesThreadedAnimation:YES];
        [messageProgress startAnimation:nil];
    }
}

- (void) hideMessage
{
    [messageProgress stopAnimation:nil];
    [messagePanel close];
}

#if SDL_USE_NIB_FILE

/* Fix menu to contain the real app name instead of "SDL App" */
- (void)fixMenu:(NSMenu *)aMenu withAppName:(NSString *)appName
{
    NSRange aRange;
    NSEnumerator *enumerator;
    NSMenuItem *menuItem;
	
    aRange = [[aMenu title] rangeOfString:@"SDL App"];
    if (aRange.length != 0)
        [aMenu setTitle: [[aMenu title] stringByReplacingRange:aRange with:appName]];
	
    enumerator = [[aMenu itemArray] objectEnumerator];
    while ((menuItem = [enumerator nextObject]))
    {
        aRange = [[menuItem title] rangeOfString:@"SDL App"];
        if (aRange.length != 0)
            [menuItem setTitle: [[menuItem title] stringByReplacingRange:aRange with:appName]];
        if ([menuItem hasSubmenu])
            [self fixMenu:[menuItem submenu] withAppName:appName];
    }
    //EP commented line to avoid deprecation warning for 10.6:  [aMenu sizeToFit];
}

#else

static void setApplicationMenu(void)
{
    /* warning: this code is very odd */
    NSMenu *appleMenu;

	NSMenuItem *menuItem;
	NSString *title;
	NSString *appName;
	
	appName = getApplicationName();	
    appleMenu = [[NSMenu alloc] initWithTitle:@""];
    
	/* Add menu items */
	title = [@"About " stringByAppendingString:appName];
	[appleMenu addItemWithTitle:title action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
	
	[appleMenu addItem:[NSMenuItem separatorItem]];
	
	title = [@"Hide " stringByAppendingString:appName];
	[appleMenu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@"h"];

	menuItem = (NSMenuItem *)[appleMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
	[menuItem setKeyEquivalentModifierMask:(NSAlternateKeyMask|NSCommandKeyMask)];

	[appleMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
	
	[appleMenu addItem:[NSMenuItem separatorItem]];
	
	title = [@"Quit " stringByAppendingString:appName];
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

/* Create a window menu */
static void setupWindowMenu(void)
{
	NSMenu      *windowMenu;
	NSMenuItem  *windowMenuItem;
	NSMenuItem  *menuItem;

    windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
    
    /* "Minimize" item */
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
    [windowMenu addItem:menuItem];
    [menuItem release];
    
    /* Put menu into the menubar */
    windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
    [windowMenuItem setSubmenu:windowMenu];
    [[NSApp mainMenu] addItem:windowMenuItem];
    
    /* Tell the application object that this is now the window menu */
    [NSApp setWindowsMenu:windowMenu];

    /* Finally give up our references to the objects */
    [windowMenu release];
    [windowMenuItem release];
}

/* Create a window menu */
static void setupHelpMenu(void)
{
	NSMenu      *helpMenu;
	NSMenuItem  *helpMenuItem;
	NSMenuItem  *menuItem;

    helpMenu = [[NSMenu alloc] initWithTitle:@"Help"];
    
    /* "Help" item */
	 NSString *appName = getApplicationName();
    menuItem = [[NSMenuItem alloc] initWithTitle:[appName stringByAppendingString:@" Help"] action:@selector(tuxpaintHelp:) keyEquivalent:@"?"];
    [helpMenu addItem:menuItem];
    [menuItem release];
    
    /* Put menu into the menubar */
    helpMenuItem = [[NSMenuItem alloc] initWithTitle:@"Help" action:nil keyEquivalent:@""];
    [helpMenuItem setSubmenu:helpMenu];
    [[NSApp mainMenu] addItem:helpMenuItem];

    /* Finally give up our references to the objects */
    [helpMenu release];
    [helpMenuItem release];
}

/* Replacement for NSApplicationMain */
static void CustomApplicationMain (argc, argv)
{

	NSAutoreleasePool   *pool = [[NSAutoreleasePool alloc] init];
	SDLMain             *sdlMain;

    /* Ensure the application object is initialised */
    [SDLApplication sharedApplication];
    
#ifdef SDL_USE_CPS
    {
        CPSProcessSerNum PSN;
        /* Tell the dock about us */
        if (!CPSGetCurrentProcess(&PSN))
            if (!CPSEnableForegroundOperation(&PSN,0x03,0x3C,0x2C,0x1103))
                if (!CPSSetFrontProcess(&PSN))
                    [SDLApplication sharedApplication];
    }
#endif /* SDL_USE_CPS */

    /* Set up the menubar */
    [NSApp setMainMenu:[[NSMenu alloc] init]];
	setApplicationMenu();
    setupWindowMenu();
	setupHelpMenu();
	
    /* Create SDLMain and make it the app delegate */
    sdlMain = [[SDLMain alloc] init];
    [NSApp setDelegate:sdlMain];
    
    /* Start the main event loop */
    [NSApp run];
    
    [sdlMain release];
    [pool release];
}

#endif

/* Make Mac-specific information available to SDL app */
- (void) setupBridge
{
	CocoaToSDLBridge    *bridge;
    bridge = [[CocoaToSDLBridge alloc] init];
    [bridge autorelease];
    [bridge fontsPath];
	[bridge preferencesPath];
}

- (BOOL) installFontconfigFiles
{
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *executable = [bundle pathForAuxiliaryExecutable:@"fcinstaller"];
    NSString *arguments = [NSString stringWithCString:(macosx.globalPreferencesPath)];
    
    char command[4096];
        //EP commented to avoid deprecation warning for 10.6:    sprintf(command, "\"%s\" \"%s\"", [executable cString], [arguments cString]);
        sprintf(command, "\"%@\" \"%@\"", executable, arguments);
    
    int result = system(command);
    
    return (BOOL)result;
}

- (BOOL) installFontconfigFilesWithAuthorization
{
    OSStatus status;
    AuthorizationFlags flags = kAuthorizationFlagDefaults;
    AuthorizationRef authorizationRef;
    NSBundle *bundle = [NSBundle mainBundle];
    
    status = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, flags, &authorizationRef);
    if (status != errAuthorizationSuccess)
        return status;

    AuthorizationItem items = {kAuthorizationRightExecute, 0, NULL, 0};
    AuthorizationRights rights = {1, &items};
        
    flags = kAuthorizationFlagDefaults | kAuthorizationFlagInteractionAllowed | kAuthorizationFlagPreAuthorize | kAuthorizationFlagExtendRights;
    status = AuthorizationCopyRights(authorizationRef, &rights, NULL, flags, NULL);
    
    if (status == errAuthorizationSuccess)
    {
        NSString *fcInstallerPath = [bundle pathForAuxiliaryExecutable:@"fcinstaller"];
        
        char executable[2048]; 
        char *arguments[] = { "/Library/Application Support/TuxPaint", NULL };
        FILE *communicationsPipe = NULL;
        
        strcpy(executable, [fcInstallerPath cStringUsingEncoding:NSUTF8StringEncoding]);        //EP replaced cString by cStringUsingEncoding: to avoid deprecation warning for 10.6
        
        flags = kAuthorizationFlagDefaults;
        status = AuthorizationExecuteWithPrivileges(authorizationRef, executable, flags, arguments, &communicationsPipe);
    }
    
    AuthorizationFree(authorizationRef, kAuthorizationFlagDefaults);
    return (status == errAuthorizationSuccess); 
}

- (BOOL) fontconfigFilesAreInstalled
{
    NSString *globalPreferencesPath = [NSString stringWithCString:(macosx.globalPreferencesPath)];
    NSString *fontsPath = [globalPreferencesPath stringByAppendingString:@"/fontconfig/fonts"];
    NSString *fontsConfInstalledPath = [fontsPath stringByAppendingString:@"/fonts.conf"];
    NSString *fontsDtdInstalledPath = [fontsPath stringByAppendingString:@"/fonts.dtd"];
    BOOL filesExist = TRUE;
 
    NSFileManager *fileManager = [NSFileManager defaultManager];
    filesExist = [fileManager fileExistsAtPath:fontsConfInstalledPath] && [fileManager fileExistsAtPath:fontsDtdInstalledPath];
    return filesExist;
}

/* Set up Fontconfig */
- (void) setupFontconfig
{
    /* Tell Fontconfig to use font configuration file in application bundle */
    setenv ("FONTCONFIG_PATH", [[[NSBundle mainBundle] resourcePath] cStringUsingEncoding:NSUTF8StringEncoding], 1);            //EP replaced cString by cStringUsingEncoding: to avoid deprecation warning for 10.6

    /* Install font configuration file */
    /*
    BOOL filesExist = [self fontconfigFilesAreInstalled];
    if (!filesExist)
    {
        [self installFontconfigFiles];
        filesExist = [self fontconfigFilesAreInstalled];
        if (!filesExist)
        {
            [self installFontconfigFilesWithAuthorization];
            filesExist = [self fontconfigFilesAreInstalled];
            if (!filesExist)
                exit(-1);
        }
    }
    */
  
    /* Determine if Fontconfig cache needs to be built */
    NSString *globalPreferencesPath = [NSString stringWithCString:(macosx.globalPreferencesPath)];
    NSString *globalCachePath = [globalPreferencesPath stringByAppendingString:@"/fontconfig/cache"];
    NSString *userCachePath = [[NSString stringWithString:@"~/.fontconfig"] stringByExpandingTildeInPath];
    
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if (![fileManager fileExistsAtPath:globalCachePath] && ![fileManager fileExistsAtPath:userCachePath])
    {
        /* Build Fontconfig cache */
        displayMessage( MSG_FONT_CACHE );
        FcBool initSuccess = FcInit();
        hideMessage();
    }
}

/*
 * Catch document open requests...this lets us notice files when the app
 *  was launched by double-clicking a document, or when a document was
 *  dragged/dropped on the app's icon. You need to have a
 *  CFBundleDocumentsType section in your Info.plist to get this message,
 *  apparently.
 *
 * Files are added to gArgv, so to the app, they'll look like command line
 *  arguments. Previously, apps launched from the finder had nothing but
 *  an argv[0].
 *
 * This message may be received multiple times to open several docs on launch.
 *
 * This message is ignored once the app's mainline has been called.
 */
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
    const char *temparg;
    size_t arglen;
    char *arg;
    char **newargv;
	
    if (!gFinderLaunch)  /* MacOS is passing command line args. */
        return FALSE;
	
    if (gCalledAppMainline)  /* app has started, ignore this document. */
        return FALSE;
	
    temparg = [filename UTF8String];
    arglen = SDL_strlen(temparg) + 1;
    arg = (char *) SDL_malloc(arglen);
    if (arg == NULL)
        return FALSE;
	
    newargv = (char **) realloc(gArgv, sizeof (char *) * (gArgc + 2));
    if (newargv == NULL)
    {
        SDL_free(arg);
        return FALSE;
    }
    gArgv = newargv;
	
    SDL_strlcpy(arg, temparg, arglen);
    gArgv[gArgc++] = arg;
    gArgv[gArgc] = NULL;
    return TRUE;
}

- (BOOL)textView:(NSTextView*)textView clickedOnLink:(id)link atIndex:(unsigned)charIndex 
{
    BOOL success;
    success = [[NSWorkspace sharedWorkspace] openURL: link];
    return success;
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
    int status;
    sdlMain = self;
	
	/* Allow Cocoa events to be processed */
	setenv ("SDL_ENABLEAPPEVENTS", "1", 1);	
	
	/* Set up Cocoa to SDL bridge */
	[self setupBridge];
    
    /* Set up Fontconfig */
    [self setupFontconfig];
	
    /* Set the working directory to the .app's parent directory */
    [self setupWorkingDirectory:gFinderLaunch];
	
#if SDL_USE_NIB_FILE
    /* Set the main menu to contain the real app name instead of "SDL App" */
    [self fixMenu:[NSApp mainMenu] withAppName:getApplicationName()];
#endif
    
    /* Hand off to main application code */
    gCalledAppMainline = TRUE;
    status = SDL_main (gArgc, gArgv);
	
    /* We're done, thank you for playing */
    exit(status);
}
@end


@implementation NSString (ReplaceSubString)

- (NSString *)stringByReplacingRange:(NSRange)aRange with:(NSString *)aString
{
    unsigned int bufferSize;
    unsigned int selfLen = [self length];
    unsigned int aStringLen = [aString length];
    unichar *buffer;
    NSRange localRange;
    NSString *result;
	
    bufferSize = selfLen + aStringLen - aRange.length;
    buffer = NSAllocateMemoryPages(bufferSize*sizeof(unichar));
    
    /* Get first part into buffer */
    localRange.location = 0;
    localRange.length = aRange.location;
    [self getCharacters:buffer range:localRange];
    
    /* Get middle part into buffer */
    localRange.location = 0;
    localRange.length = aStringLen;
    [aString getCharacters:(buffer+aRange.location) range:localRange];
	
    /* Get last part into buffer */
    localRange.location = aRange.location + aRange.length;
    localRange.length = selfLen - localRange.location;
    [self getCharacters:(buffer+aRange.location+aStringLen) range:localRange];
    
    /* Build output string */
    result = [NSString stringWithCharacters:buffer length:bufferSize];
    
    NSDeallocateMemoryPages(buffer, bufferSize);
    
    return result;
}

@end

#ifdef main
#  undef main
#endif

/* Main entry point to executable - should *not* be SDL_main! */
int main (int argc, char **argv)
{
    /* Copy the arguments into a global variable */
    /* This is passed if we are launched by double-clicking */
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) {
        gArgv = (char **) SDL_malloc(sizeof (char *) * 2);
        gArgv[0] = argv[0];
        gArgv[1] = NULL;
        gArgc = 1;
        gFinderLaunch = YES;
    } else {
        int i;
        gArgc = argc;
        gArgv = (char **) SDL_malloc(sizeof (char *) * (argc+1));
        for (i = 0; i <= argc; i++)
            gArgv[i] = argv[i];
        gFinderLaunch = NO;
    }
	
#if SDL_USE_NIB_FILE
    [SDLApplication poseAsClass:[NSApplication class]];
    NSApplicationMain (argc, (const char**) argv);
#else
    CustomApplicationMain (argc, argv);
#endif
    return 0;
}
