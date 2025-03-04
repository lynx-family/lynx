// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

@class LynxUI;

@interface LynxBackgroundCapInsets : NSObject
@property(nonatomic, assign) UIEdgeInsets capInsets;
@property(nonatomic, weak) LynxUI* ui;
- (instancetype)initWithParams:(NSString*)capInsetsString;
- (void)reset;
@end
