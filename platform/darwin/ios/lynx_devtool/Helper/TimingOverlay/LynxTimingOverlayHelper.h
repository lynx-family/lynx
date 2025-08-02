// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxFSPTracer+Internal.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxUIOwner.h>

NS_ASSUME_NONNULL_BEGIN

@class DevToolPlatformDarwinDelegate;

@interface LynxTimingOverlayHelper : NSObject

+ (void)setEnabled:(BOOL)enabled;

- (instancetype)initWithDelegate:(DevToolPlatformDarwinDelegate*)delegate;

- (void)attachLynxView:(LynxView*)view uiOwner:(LynxUIOwner*)uiOwner;

@end

NS_ASSUME_NONNULL_END
