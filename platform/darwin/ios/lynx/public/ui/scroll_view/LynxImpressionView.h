// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUI.h"

NS_ASSUME_NONNULL_BEGIN

UIKIT_EXTERN NSNotificationName const LynxImpressionWillManualExposureNotification;
UIKIT_EXTERN NSNotificationName const LynxImpressionLynxViewIDNotificationKey;
UIKIT_EXTERN NSNotificationName const LynxImpressionStatusNotificationKey;
UIKIT_EXTERN NSNotificationName const LynxImpressionForceImpressionBoolKey;

@protocol LynxImpressionParentView <NSObject>

@optional
- (BOOL)shouldManualExposure;

@end

@interface LynxInnerImpressionView : UIView

@property(nonatomic, assign, readonly) BOOL onScreen;

@property(nonatomic, assign) float impressionPercent;

- (void)impression;
- (void)exit;

@end

@interface LynxImpressionView : LynxUI <LynxInnerImpressionView*>

@end

NS_ASSUME_NONNULL_END
