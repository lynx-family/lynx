// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICESYSTEMINVOKEPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICESYSTEMINVOKEPROTOCOL_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxServiceProtocol;
@class AVCaptureSession;
@class CMMotionManager;
@class CMDeviceMotion;
@class UIView;
@class UIImage;
@class UIColor;

@protocol LynxServiceSystemInvokeProtocol <LynxServiceProtocol>

/**
 @brief Set string in pasteboard.
 @param string NSString
 @see setString: of UIPasteboard for details.
*/
- (void)setString:(NSString *)string;

/**
 @brief Starts device motion updates, providing data to the given handler through the given queue.
 @param motionManager motionManager
 @param queue NSOperationQueue
 @param handler callback with CMDeviceMotion and NSError
 @see startDeviceMotionUpdatesToQueue:withHandler: of CMMotionManager for details.
*/
- (void)startDeviceMotionUpdates:(CMMotionManager *)motionManager
                         toQueue:(NSOperationQueue *)queue
                     withHandler:(void (^)(CMDeviceMotion *__nullable motion,
                                           NSError *__nullable error))handler;
/**
 @brief Stops device motion updates with no handler.
 @param motionManager motionManager
 @see stopDeviceMotionUpdates of CMMotionManager for details.
*/
- (void)stopDeviceMotionUpdates:(CMMotionManager *)motionManager;

/**
 @brief Starts an AVCaptureSession instance running.
 @param captureSession AVCaptureSession
 @see startRunning of AVCaptureSession for details.
*/
- (void)startCaptureSessionRunning:(AVCaptureSession *)captureSession;

/**
 @brief Stops an AVCaptureSession instance that is currently running.
 @param captureSession AVCaptureSession
 @see stopRunning of AVCaptureSession for details.
*/
- (void)stopCaptureSessionRunning:(AVCaptureSession *)captureSession;

/**
 @brief Take screenshot for the given view.
 @param view UIView
 @param color The background color of screenshot
 @param scale float
*/
- (UIImage *)takeScreenshot:(UIView *)view withBackgroundColor:(UIColor *)color scale:(float)scale;

/**
 @brief start audio output unit, do not start when cert is degraded
 @param audioUnitPtr pointer to AudioUnit, should not be null
 @see OSStatus AudioOutputUnitStart(AudioUnit ci) in AudioOutputUnit for details
*/
- (NSInteger)startAudioOutputUnit:(void *)audioUnitPtr;

/**
 @brief stop audio output unit, do not stop when cert is degraded
 @param audioUnitPtr pointer to AudioUnit, should not be null
 @see OSStatus AudioOutputUnitStop(AudioUnit ci) in AudioOutputUnit for details
*/
- (NSInteger)stopAudioOutputUnit:(void *)audioUnitPtr;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICESYSTEMINVOKEPROTOCOL_H_
