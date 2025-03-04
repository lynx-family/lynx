// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxAnimationInfo.h"
#import "LynxConverter.h"
#import "LynxKeyframes.h"

@class LynxUI;

@interface LynxKeyframeManager : NSObject

@property(nonatomic, weak) LynxUI* ui;

// This config is only for iOS.
// IOS will remove animation from layer when app enter background or view is deattached. If you
// enable autoResumeAnimation, it will try to resume animation when app enter foreground or view is
// attached.
@property(nonatomic) BOOL autoResumeAnimation;

- (instancetype)initWithUI:(LynxUI*)ui;
- (void)setAnimations:(NSArray<LynxAnimationInfo*>*)infos;
- (void)setAnimation:(LynxAnimationInfo*)info;
- (void)notifyAnimationUpdated;
- (void)notifyBGLayerAdded;
- (void)notifyPropertyUpdated:(NSString*)name value:(id)value;
- (void)endAllAnimation;
- (BOOL)hasAnimationRunning;
// Only use for list to reset cell keyframe animation when it prepare for reusing cell.
- (void)resetAnimation;
// Only use for list to restart cell keyframe animation when it reuse cell successful.
- (void)restartAnimation;
- (void)resumeAnimation;
- (void)detachFromUI;
- (void)attachToUI:(LynxUI*)ui;
@end
