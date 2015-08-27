//
//  fcinstaller.m
//  TuxPaint
//
//  Created by Martin Fuhrer on 03/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
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

#import <Foundation/Foundation.h>
#include <fontconfig/fontconfig.h>

/* Category for NSFileManager */
@interface NSFileManager (CreateDirectoryRecursively)
- (BOOL)createDirectoryRecursively:(NSString *)path attributes:(NSDictionary *)attributes;
@end

int main(int argc, const char* argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *bundlePath = [bundle bundlePath];
    
    BOOL success = TRUE;
    BOOL fileExists = TRUE;
    
    if( argc < 2 )
        return -1;
    
    NSString *globalPreferencesPath = [NSString stringWithCString:(argv[1])];
    NSString *fontsPath = [globalPreferencesPath stringByAppendingString:@"/fontconfig/fonts"];
    NSString *fontsConfInstalledPath = [fontsPath stringByAppendingString:@"/fonts.conf"];
    NSString *fontsDtdInstalledPath = [fontsPath stringByAppendingString:@"/fonts.dtd"];
    NSString *fontsConfBundlePath = [bundle pathForResource:@"fonts" ofType:@"conf"];
    NSString *fontsDtdBundlePath = [bundle pathForResource:@"fonts" ofType:@"dtd"];
    
    fileExists = [fileManager fileExistsAtPath:fontsConfInstalledPath];
    if (!fileExists) {
        success = ([fileManager createDirectoryRecursively:fontsPath attributes:nil] && success);
        success = ([fileManager copyPath:fontsConfBundlePath toPath:fontsConfInstalledPath handler:nil] && success);
    }
    
    fileExists = [fileManager fileExistsAtPath:fontsDtdInstalledPath];
    if (!fileExists) {
        success = ([fileManager createDirectoryRecursively:fontsPath attributes:nil] && success);
        success = ([fileManager copyPath:fontsDtdBundlePath toPath:fontsDtdInstalledPath handler:nil] && success);
    }
    
    /*
    NSString *globalCachePath = [globalPreferencesPath stringByAppendingString:@"/fontconfig/cache"];
    NSString *userCachePath = [[NSString stringWithString:@"~/.fontconfig"] stringByExpandingTildeInPath];
    fileExists = ([fileManager fileExistsAtPath:globalCachePath] || [fileManager fileExistsAtPath:userCachePath]);
    if (!fileExists)
    {
        FcBool initSuccess = FcInit();
        if( initSuccess == FcFalse )
            success = FALSE;
    }
    */
    
    [pool release];
    
    return (int)(!success);
}

@implementation NSFileManager (CreateDirectoryRecursively)

- (BOOL)createDirectoryRecursively:(NSString *)path attributes:(NSDictionary *)attributes
{ 
    BOOL isDir = TRUE; 
    BOOL fileExists;
    
    fileExists = [self fileExistsAtPath:path isDirectory:&isDir]; 
    if (isDir) {
        if (fileExists) {
            /* directory exists */
            return TRUE; 
        }
        else
        {
            /* create directory */
            NSString *parentDirectory = [path stringByDeletingLastPathComponent];
            [self createDirectoryRecursively:parentDirectory attributes:attributes]; 
            return [self createDirectoryAtPath:path attributes:attributes];
        }
    }
    else
    { 
        /* desired directory path is blocked by a file */ 
        return FALSE; 
    }  
} 

@end