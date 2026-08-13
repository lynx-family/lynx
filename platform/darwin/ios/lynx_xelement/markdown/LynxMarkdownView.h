// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxMarkdownBundleV2;
@class ServalMarkdownView;

@interface LynxMarkdownViewV2 : UIView

- (nullable ServalMarkdownView *)setBundle:(nullable LynxMarkdownBundleV2 *)bundle;
- (void)setContentOffset:(CGPoint)contentOffset;

@end

NS_ASSUME_NONNULL_END
