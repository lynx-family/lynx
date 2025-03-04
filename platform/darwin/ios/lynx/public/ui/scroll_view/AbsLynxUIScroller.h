// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LynxUIScrollerApi_h
#define LynxUIScrollerApi_h

#import "LynxUI.h"
/* A helper class for LynxUIScroller
 */

@protocol LynxUIScrollerDelegate <NSObject>

@optional
- (void)scrollerDidScroll:(UIScrollView *)scrollView;
- (void)scrollerWillBeginDragging:(UIScrollView *)scrollView;
- (void)scrollerDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate;
- (void)scrollerDidEndDecelerating:(UIScrollView *)scrollView;
- (void)scrollerDidEndScrollingAnimation:(UIScrollView *)scrollView;
@end

@interface AbsLynxUIScroller<__covariant V : UIView *> : LynxUI <V>

typedef NS_ENUM(NSInteger, ScrollDirection) {
  SCROLL_UP = 0,
  SCROLL_DOWN = 1,
  SCROLL_LEFT = 2,
  SCROLL_RIGHT = 3,
};

- (void)setScrollY:(BOOL)value requestReset:(BOOL)requestReset;

- (void)setScrollX:(BOOL)value requestReset:(BOOL)requestReset;

- (void)setScrollYReverse:(BOOL)value requestReset:(BOOL)requestReset;

- (void)setScrollXReverse:(BOOL)value requestReset:(BOOL)requestReset;

- (void)setScrollBarEnable:(BOOL)value requestReset:(BOOL)requestReset;

- (void)setUpperThreshold:(NSInteger)value requestReset:(BOOL)requestReset;

- (void)setLowerThreshold:(NSInteger)value requestReset:(BOOL)requestReset;

- (void)setScrollTop:(int)vale requestReset:(BOOL)requestReset;

- (void)setScrollLeft:(int)vale requestReset:(BOOL)requestReset;

- (void)setScrollToIndex:(int)vale requestReset:(BOOL)requestReset;

- (void)scrollInto:(LynxUI *)value
          isSmooth:(BOOL)isSmooth
         blockType:(NSString *)blockType
        inlineType:(NSString *)inlineTyle;

- (void)sendScrollEvent:(NSString *)name
              scrollTop:(float)top
              scollleft:(float)left
           scrollHeight:(float)height
            scrollWidth:(float)width
                 deltaX:(float)x
                 deltaY:(float)y;

- (BOOL)canScroll:(ScrollDirection)direction;

- (void)scrollByX:(float)delta;

- (void)scrollByY:(float)delta;

- (void)flickX:(float)velocity;

- (void)flickY:(float)velocity;
- (void)addScrollerDelegate:(id<LynxUIScrollerDelegate>)delegate;
- (void)removeScrollerDelegate:(id<LynxUIScrollerDelegate>)delegate;

@end

#endif /* LynxUIScrollerApi_h */
