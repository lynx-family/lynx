// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIComponent.h"
#import "LynxUIScroller.h"

@interface LynxUIListContainer : LynxUIScroller <LynxUIComponentLayoutObserver>
// Mark c++ has updated contentSize and contentOffset
@property(nonatomic, assign) BOOL needAdjustContentOffset;
// Target delta from c++
@property(nonatomic, assign) CGPoint targetDelta;
// Target contentSize from c++
@property(nonatomic, assign) CGFloat targetContentSize;
@property(nonatomic, strong) NSMutableDictionary *listNativeStateCache;
// <cacheKey, Set<initial- props>> Stores flushed initial- props for cacheKey
@property(nonatomic, strong)
    NSMutableDictionary<NSString *, NSMutableSet<NSString *> *> *initialFlushPropCache;

- (void)updateScrollInfoWithEstimatedOffset:(CGFloat)estimatedOffset
                                     smooth:(BOOL)smooth
                                  scrolling:(BOOL)scrolling;
- (void)insertListComponent:(LynxUIComponent *)component;
- (void)detachedFromWindow;
@end
