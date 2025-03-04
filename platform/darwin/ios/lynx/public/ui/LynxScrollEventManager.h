// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

@class LynxUIContext;

NS_ASSUME_NONNULL_BEGIN

FOUNDATION_EXPORT NSString *const LynxEventScroll;
FOUNDATION_EXPORT NSString *const LynxEventScrollEnd;
FOUNDATION_EXPORT NSString *const LynxEventScrollToUpper;
FOUNDATION_EXPORT NSString *const LynxEventScrollToUpperEdge;
FOUNDATION_EXPORT NSString *const LynxEventScrollToLower;
FOUNDATION_EXPORT NSString *const LynxEventScrollToLowerEdge;
FOUNDATION_EXPORT NSString *const LynxEventScrollToNormalState;
FOUNDATION_EXPORT NSString *const LynxEventContentSizeChange;
FOUNDATION_EXPORT NSString *const LynxEventScrollStateChange;
FOUNDATION_EXPORT NSString *const LynxEventScrollToBounce;
FOUNDATION_EXPORT NSString *const LynxScrollViewInitialScrollOffset;
FOUNDATION_EXPORT NSString *const LynxScrollViewInitialScrollIndex;
FOUNDATION_EXPORT NSString *const LynxEventStickyTop;
FOUNDATION_EXPORT NSString *const LynxEventStickyBottom;
FOUNDATION_EXPORT NSString *const LynxEventSnap;

@protocol LynxCustomScrollDelegate
- (void)autoScrollStop;
@end

@interface LynxScrollEventManager : NSObject
- (instancetype)initWithContext:(LynxUIContext *)context
                           sign:(NSInteger)sign
                       eventSet:(NSDictionary *_Nullable)eventSet;
- (void)sendScrollEvent:(NSString *)name scrollView:(UIScrollView *)scrollView;
- (void)sendScrollEvent:(NSString *)name
             scrollView:(UIScrollView *)scrollView
                 detail:(NSDictionary *)detail;
- (void)sendScrollEvent:(NSString *)name
             scrollView:(UIScrollView *)scrollView
                 deltaX:(CGFloat)x
                 deltaY:(CGFloat)y;
- (void)sendScrollEvent:(NSString *)name
             scrollView:(UIScrollView *)scrollView
    targetContentOffset:(CGPoint)targetContentOffset;
- (BOOL)eventBound:(NSString *)name;

@end  // LynxScrollEventManager

NS_ASSUME_NONNULL_END
