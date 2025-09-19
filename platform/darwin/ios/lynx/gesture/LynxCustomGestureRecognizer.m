// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxCustomGestureRecognizer.h"
#import <UIKit/UIGestureRecognizerSubclass.h>

@implementation LynxCustomGestureRecognizer

- (void)possibleGesture {
  self.state = UIGestureRecognizerStatePossible;
}

- (void)beginGesture {
  self.state = UIGestureRecognizerStateBegan;
}

- (void)changeGesture {
  self.state = UIGestureRecognizerStateChanged;
}

- (void)endGesture {
  self.state = UIGestureRecognizerStateEnded;
}

- (void)failGesture {
  self.state = UIGestureRecognizerStateFailed;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
  [super touchesBegan:touches withEvent:event];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
  [super touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
  [super touchesEnded:touches withEvent:event];
  [self failGesture];
  [self possibleGesture];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
  [super touchesCancelled:touches withEvent:event];
  [self failGesture];
  [self possibleGesture];
}

@end
