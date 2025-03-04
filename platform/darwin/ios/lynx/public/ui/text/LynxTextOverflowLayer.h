// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxTextView;

@interface LynxTextOverflowLayer : CALayer

@property(nonatomic, weak) LynxTextView *view;

- (instancetype)init;
- (instancetype)initWithView:(nullable LynxTextView *)view;

@end

NS_ASSUME_NONNULL_END
