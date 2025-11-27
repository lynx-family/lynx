// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Cocoa/Cocoa.h>

#import "clay/shell/platform/darwin/macos/framework/Headers/FlutterViewController.h"

/**
 * A plugin to handle mouse cursor.
 *
 * Responsible for bridging the native macOS mouse cursor system with the
 * Flutter engine , via system channels.
 */
@interface ClayMouseCursorPlugin : NSObject

- (void)activateSystemCursor:(int)type path:(const char*)path;

@end
