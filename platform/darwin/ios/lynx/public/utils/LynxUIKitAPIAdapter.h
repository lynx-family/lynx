// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

// Since some iOS APIs are deprecated in higher versions, the host may encounter compilation
// problems when compiling Lynx. Therefore, in order to uniformly adapt these APIs, we add
// LynxUIKitAPIAdapter to manage them uniformly. Lynx SDK developers only need to call these related
// interfaces.
@interface LynxUIKitAPIAdapter : NSObject

// Get the window collection of the current application.
+ (NSArray<UIWindow*>*)getWindows;

// Get the main window of the current application.
+ (UIWindow*)getKeyWindow;

// Get the status bar position information of the current application.
+ (CGRect)getStatusBarFrame;

@end
