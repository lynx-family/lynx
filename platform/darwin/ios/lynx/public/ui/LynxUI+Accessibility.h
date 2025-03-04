// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUI.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxUI (Accessibility)
- (void)handleAccessibility:(UIView *)accessibilityAttachedCell
                 autoScroll:(BOOL)accessibilityAutoScroll;

- (NSString *)accessibilityText;

- (NSArray *)setExclusiveAccessibilityElements:(BOOL)exclusive
                                       subTree:(LynxUI *)subTree
                                 previousNodes:(NSArray *)previousNodes;

- (void)clearExclusiveAccessibilityElements:(NSArray *)nodes;

@end

NS_ASSUME_NONNULL_END
