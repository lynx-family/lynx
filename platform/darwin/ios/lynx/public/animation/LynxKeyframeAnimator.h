// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxAnimationInfo.h"
#import "LynxConverter.h"
#import "LynxKeyframes.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxUI;

#pragma mark - LynxKeyframeAnimator
@interface LynxKeyframeAnimator : NSObject

typedef NS_ENUM(NSUInteger, LynxKFAnimatorState) {
  LynxKFAnimatorStateIdle = 0,
  LynxKFAnimatorStateRunning,
  LynxKFAnimatorStatePaused,
  LynxKFAnimatorStateCanceled,
  LynxKFAnimatorStateCanceledLegacy,
  LynxKFAnimatorStateDestroy
};

@property(nonatomic, weak, nullable) LynxUI* ui;
@property(class, readonly) NSString* kTransformStr;
@property(class, readonly) NSString* kOpacityStr;
@property(class, readonly) NSString* kBackgroundColorStr;
@property(nonatomic, strong) NSMutableDictionary* propertyOriginValue;

// This config is only for iOS.
// IOS will remove animation from layer when app enter background or view is deattached. If you
// enable autoResumeAnimation, it will try to resume animation when app enter foreground or view is
// attached.
@property(nonatomic) BOOL autoResumeAnimation;

- (instancetype)initWithUI:(LynxUI*)ui;
- (void)apply:(LynxAnimationInfo*)info;
- (void)destroy;
- (void)cancel;
- (void)notifyBGLayerAdded;
- (void)notifyPropertyUpdated:(NSString*)name value:(id)value;
- (BOOL)isRunning;
- (BOOL)shouldReInitTransform;
- (void)tryToResumeAnimationOnNextFrame;
- (void)detachFromUI;
- (void)attachToUI:(LynxUI*)ui;

@end

#pragma mark - LynxKeyframeParsedData
@interface LynxKeyframeParsedData : NSObject
@property(nonatomic, strong) NSMutableDictionary<NSString*, NSMutableArray*>* keyframeValues;
@property(nonatomic, strong)
    NSMutableDictionary<NSString*, NSMutableArray<NSNumber*>*>* keyframeTimes;
@property(nonatomic, strong) NSMutableDictionary<NSString*, id>* beginStyles;
@property(nonatomic, strong) NSMutableDictionary<NSString*, id>* endStyles;
@property(nonatomic) BOOL isPercentTransform;

- (instancetype)init;
@end

NS_ASSUME_NONNULL_END
