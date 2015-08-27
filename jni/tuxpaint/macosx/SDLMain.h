/*   SDLMain.m - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
	 $Id: SDLMain.h,v 1.6 2008/02/20 05:32:21 mfuhrer Exp $
*/

//#define MAC_OS_X_VERSION_MAX_ALLOWED MAC_OS_X_VERSION_10_2
//#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_2

#import <Cocoa/Cocoa.h>
#import "TransparentTextView.h"

@interface SDLMain : NSObject
{
    IBOutlet NSWindow *messagePanel;
    IBOutlet NSTextField *messageText;
    IBOutlet NSTextField *messageStatus;
    IBOutlet NSProgressIndicator *messageProgress;
    IBOutlet NSWindow *aboutWindow;
    IBOutlet NSTextField *appnameText;
    IBOutlet NSTextField *versionText;
    IBOutlet TransparentTextView *acknowledgmentsText;
}

- (IBAction)onAbout:(id)sender;
- (IBAction)onNew:(id)sender;
- (IBAction)onOpen:(id)sender;
- (IBAction)onSave:(id)sender;
- (IBAction)onPrint:(id)sender;
- (IBAction)onPageSetup:(id)sender;
- (IBAction)onUndo:(id)sender;
- (IBAction)onRedo:(id)sender;
- (IBAction)onHelp:(id)sender;
- (IBAction)onQuit:(id)sender;

- (void) sendSDLControlKeystroke:(int)key;
- (void) sendSDLControlShiftKeystroke:(int)key;
- (void) setupBridge;

- (void) displayMessage:(NSString*)message andStatus:(NSString*)status withProgressIndicator:(BOOL)progress;
- (void) hideMessage;

@end
