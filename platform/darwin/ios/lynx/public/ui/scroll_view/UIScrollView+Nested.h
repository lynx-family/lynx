// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

@interface UIScrollView (Nested)

@property(nonatomic, weak) UIScrollView* parentScrollView;
@property(nonatomic) NSPointerArray* childrenScrollView;
@property(nonatomic) CGPoint lastPosition;
@property(nonatomic) BOOL enableNested;
@property(nonatomic) BOOL scrollY;
@property(nonatomic, strong) NSString* name;
@property(nonatomic) BOOL isRTL;

- (BOOL)childScrollViewCanScrollAtPoint:(CGPoint)point withDirection:(BOOL)isScrollY;
- (BOOL)isOverEdge:(BOOL)isScrollY;
- (UIScrollView*)nearestParentScrollView;
- (void)triggerNestedScrollView:(BOOL)enableScrollY;
- (void)updateChildren;

@end
