// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

@interface LynxDevToolToast : NSObject

// show toast safely on main thread
+ (void)showToast:(NSString*)message;

@end
