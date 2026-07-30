// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxView.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxDevToolPointerEventType) {
  LynxDevToolPointerEventTypeDown,
  LynxDevToolPointerEventTypeMove,
  LynxDevToolPointerEventTypeUp,
  LynxDevToolPointerEventTypeCancel,
  LynxDevToolPointerEventTypeScroll,
};

@interface LynxEmulateTouchHelper : NSObject

@property(nonatomic, weak) LynxView *lynxView;
@property(nonatomic, assign) Boolean mouseWheelFlag;
@property(nonatomic, assign) CGPoint last;
@property(nonatomic, copy, nullable) dispatch_block_t task;
@property(nonatomic, assign) int deltaScale;
@property(nonatomic, strong, nullable) UITouch *touch;
@property(nonatomic, strong, nullable) UIEvent *event;

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

@interface LynxEmulateTouchHelper (PointerEventInjection)

- (BOOL)injectPointerEvent:(LynxDevToolPointerEventType)type
               coordinateX:(CGFloat)x
               coordinateY:(CGFloat)y
                    deltaX:(CGFloat)dx
                    deltaY:(CGFloat)dy
            screenshotMode:(NSString *)screenshotMode;

@end

NS_ASSUME_NONNULL_END
