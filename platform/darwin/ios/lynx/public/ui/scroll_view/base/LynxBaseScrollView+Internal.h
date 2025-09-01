// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxBaseScrollView.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxBaseScrollView (Internal)

@property(nonatomic, assign) NSInteger scrollState;
@property(nonatomic, assign) NSInteger forwardsNestedScrollMode;
@property(nonatomic, assign) NSInteger backwardsNestedScrollMode;
@property(nonatomic, assign) CGPoint previousScrollOffset;

@end

NS_ASSUME_NONNULL_END
