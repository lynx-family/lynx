// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxInspectorOwner.h>

@class LynxDevMenuItem;

@interface LynxDevMenu : NSObject

- (instancetype)initWithInspectorOwner:(LynxInspectorOwner*)owner;
- (BOOL)isActionSheetShown;
- (void)show;
- (void)reload;

@end

@interface LynxDevMenuItem : NSObject

+ (instancetype)buttonItemWithTitle:(NSString*)title handler:(dispatch_block_t)handler;

@end
