// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxView.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxEmulateTouchHelper : NSObject

@property(nonatomic, weak, readonly, nullable) LynxView *lynxView;
@property(nonatomic, assign, readonly) Boolean mouseWheelFlag;
@property(nonatomic, assign, readonly) CGPoint last;
@property(nonatomic, copy, readonly, nullable) dispatch_block_t task;
@property(nonatomic, assign) int deltaScale;
@property(nonatomic, strong, readonly, nullable) UITouch *touch;
@property(nonatomic, strong, readonly, nullable) UIEvent *event;

- (nonnull instancetype)initWithLynxView:(LynxView *)view;

- (void)emulateTouch:(nonnull NSString *)type
         coordinateX:(int)x
         coordinateY:(int)y
              button:(nonnull NSString *)button
              deltaX:(CGFloat)dx
              deltaY:(CGFloat)dy
           modifiers:(int)modifiers
          clickCount:(int)click_count
      screenshotMode:(NSString *)screenshotMode;

- (void)attachLynxView:(nonnull LynxView *)lynxView;
@end

NS_ASSUME_NONNULL_END
