// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/UIScrollView+LynxGesture.h>
#import <Lynx/UIScrollView+Nested.h>

@class LynxUIScroller;

@interface LynxScrollView : UIScrollView <UIGestureRecognizerDelegate>

// Force scroll-view to consume gesture and fails specified classes' gesture
@property(nonatomic) BOOL forceCanScroll;
// Use with forceCanScroll. Specify which class should scroll-view fail.
@property(nonatomic) Class blockGestureClass;
// Use with blockGestureClass. Specify a tag for one view in blockGestureClass
@property(nonatomic) NSInteger recognizedViewTag;
// Use to find UI
@property(weak, nonatomic) LynxUIScroller *weakUIScroller;

@property(nonatomic, assign) CGFloat keyboardAvoidingContentHeightExtra;
@property(nonatomic, assign) BOOL duringGestureScroll;
@property(nonatomic, assign) BOOL gestureEnabled;
@property(nonatomic, assign) BOOL increaseFrequencyWithGesture;

@property(nonatomic, strong) UIPanGestureRecognizer *nativeGesturePanRecognizer;

@property(nonatomic, strong) LynxGestureConsumer *gestureConsumer;

- (void)updateContentSize;

- (void)setupNativeGestureRecognizerIfNeeded:
    (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureMap;

@end  // LynxScrollView
