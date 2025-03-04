// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEventHandler.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxEventHandler ()

- (void)setEnableViewReceiveTouch:(BOOL)enable;
- (void)setEnableSimultaneousTap:(BOOL)enable;
- (void)setDisableLongpressAfterScroll:(bool)value;
- (void)setTapSlop:(NSString *)tapSlop;
- (void)setLongPressDuration:(int32_t)value;
- (void)dispatchPanEvent:(UIPanGestureRecognizer *)sender;
- (void)needCheckConsumeSlideEvent;

- (id<LynxEventTarget>)hitTestInner:(CGPoint)point withEvent:(nullable UIEvent *)event;
- (NSInteger)checkCanRespondTapOrClick:(id<LynxEventTarget>)ui withSet:(NSSet *)set;

@end

NS_ASSUME_NONNULL_END
