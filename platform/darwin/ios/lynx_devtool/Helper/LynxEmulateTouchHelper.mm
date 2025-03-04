// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEmulateTouchHelper.h"
#include <string>

#pragma mark - LynxEmulateTouchHelper
@implementation LynxEmulateTouchHelper

- (nonnull instancetype)initWithLynxView:(LynxView*)view {
  _lynxView = view;
  _mouseWheelFlag = NO;
  _deltaScale = 5;
  return self;
}

- (void)emulateTouch:(nonnull NSString*)type
         coordinateX:(int)x
         coordinateY:(int)y
              button:(nonnull NSString*)button
              deltaX:(CGFloat)dx
              deltaY:(CGFloat)dy
           modifiers:(int)modifiers
          clickCount:(int)click_count
      screenshotMode:(NSString*)screenshotMode {
  // TODO(zhengyuwei): Support simulating clicks based on coordinate on iOS
}

- (void)attachLynxView:(nonnull LynxView*)lynxView {
  _lynxView = lynxView;
}

@end
