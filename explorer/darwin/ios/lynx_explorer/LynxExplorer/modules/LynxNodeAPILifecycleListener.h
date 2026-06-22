// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxRuntimeLifecycleListener.h>

@interface LynxNodeAPILifecycleListener : NSObject <LynxRuntimeLifecycleListener>
@property(nonatomic, weak) id token;
- (instancetype)initWithToken:(id)token;
@end
