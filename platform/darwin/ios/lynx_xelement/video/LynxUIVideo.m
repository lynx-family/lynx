// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <AVFoundation/AVFoundation.h>
#import <Lynx/LynxEvent.h>
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxPropsProcessor.h>
#import <XElement/LynxUIVideo.h>
#import <XElement/LynxVideoOperationScheduler.h>
#import <XElement/LynxVideoView.h>
#import <stdint.h>

static NSString *const kLynxVideoSourceChangeCancelMessage = @"request canceled by src change";
static const NSTimeInterval kLynxVideoMaxSeekPosition =
    (NSTimeInterval)INT64_MAX / (NSTimeInterval)NSEC_PER_SEC;

static BOOL LynxVideoIsRepresentableSeekPosition(NSTimeInterval position) {
  return isfinite(position) && position <= kLynxVideoMaxSeekPosition;
}

@interface LynxVideoPendingOperation : NSObject
@property(nonatomic, copy, nullable) LynxUIMethodCallbackBlock callback;
@property(nonatomic, copy, nullable) LynxVideoOperationCompletion completion;
@property(nonatomic, assign) BOOL completed;
@end

@implementation LynxVideoPendingOperation
@end

@interface LynxUIVideo ()
@property(nonatomic, strong, readwrite) UIView<LynxVideoPlayable> *videoView;
@property(nonatomic, strong) LynxVideoOperationScheduler *operationScheduler;
@property(nonatomic, strong) NSMutableArray<LynxVideoPendingOperation *> *pendingPlayOperations;
@property(nonatomic, strong) NSMutableArray<LynxVideoPendingOperation *> *pendingStopOperations;
@end

@implementation LynxUIVideo

- (UIView *)createView {
  self.operationScheduler = [[LynxVideoOperationScheduler alloc] init];
  self.pendingPlayOperations = [NSMutableArray array];
  self.pendingStopOperations = [NSMutableArray array];
  self.videoView = [self createVideoView];
  self.videoView.delegate = self;
  self.videoView.loop = NO;
  self.videoView.volume = 1.0;
  self.videoView.muted = NO;
  self.videoView.speed = 1.0;
  self.videoView.objectFit = LynxVideoObjectFitContain;
  self.videoView.timeUpdateInterval = 0.33;
  return self.videoView;
}

- (UIView<LynxVideoPlayable> *)createVideoView {
  return [[LynxVideoView alloc] init];
}

LYNX_PROP_SETTER("src", setSrc, NSString *) {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    NSString *srcCopy = [value copy];
    [self dispatchOnMainThread:^{
      [weakSelf setSrc:srcCopy requestReset:requestReset];
    }];
    return;
  }
  [self cancelPendingOperationsWithMessage:kLynxVideoSourceChangeCancelMessage];
  self.videoView.src = value ?: @"";
}

LYNX_PROP_SETTER("loop", setLoop, BOOL) { self.videoView.loop = value; }

LYNX_PROP_SETTER("volume", setVolume, CGFloat) {
  self.videoView.volume = MIN(1.0, MAX(0.0, value));
}

LYNX_PROP_SETTER("muted", setMuted, BOOL) { self.videoView.muted = value; }

LYNX_PROP_SETTER("speed", setSpeed, CGFloat) { self.videoView.speed = MIN(2.0, MAX(0.1, value)); }

LYNX_PROP_SETTER("object-fit", setObjectFit, NSString *) {
  self.videoView.objectFit = [self objectFitFromString:value];
}

LYNX_PROP_SETTER("mode", setMode, NSString *) {
  self.operationScheduler.mode = [self operationModeFromString:value];
}

LYNX_PROP_SETTER("timeupdate-interval", setTimeUpdateInterval, CGFloat) {
  if (value > 0) {
    self.videoView.timeUpdateInterval = value;
  }
}

LYNX_UI_METHOD(play) {
  LynxVideoPendingOperation *operation = [self pendingOperationWithCallback:callback];
  __weak typeof(self) weakSelf = self;
  [self.operationScheduler enqueueOperationWithName:@"play"
      executor:^(LynxVideoOperationCompletion completion) {
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf) {
          completion();
          return;
        }
        [strongSelf executePlayWithOperation:operation completion:completion];
      }
      cancelHandler:^{
        __strong typeof(weakSelf) strongSelf = weakSelf;
        [strongSelf finishPendingOperation:operation
                                      code:kUIMethodOperationError
                                      data:kLynxVideoSourceChangeCancelMessage];
      }];
}

LYNX_UI_METHOD(pause) {
  LynxVideoPendingOperation *operation = [self pendingOperationWithCallback:callback];
  __weak typeof(self) weakSelf = self;
  [self.operationScheduler enqueueOperationWithName:@"pause"
      executor:^(LynxVideoOperationCompletion completion) {
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf) {
          completion();
          return;
        }
        operation.completion = completion;
        [strongSelf.videoView pause];
        [strongSelf finishPendingOperation:operation code:kUIMethodSuccess data:nil];
      }
      cancelHandler:^{
        __strong typeof(weakSelf) strongSelf = weakSelf;
        [strongSelf finishPendingOperation:operation
                                      code:kUIMethodOperationError
                                      data:kLynxVideoSourceChangeCancelMessage];
      }];
}

LYNX_UI_METHOD(stop) {
  LynxVideoPendingOperation *operation = [self pendingOperationWithCallback:callback];
  __weak typeof(self) weakSelf = self;
  [self.operationScheduler enqueueOperationWithName:@"stop"
      executor:^(LynxVideoOperationCompletion completion) {
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf) {
          completion();
          return;
        }
        operation.completion = completion;
        [strongSelf.pendingStopOperations addObject:operation];
        [strongSelf.videoView stop];
      }
      cancelHandler:^{
        __strong typeof(weakSelf) strongSelf = weakSelf;
        [strongSelf finishPendingOperation:operation
                                      code:kUIMethodOperationError
                                      data:kLynxVideoSourceChangeCancelMessage];
      }];
}

LYNX_UI_METHOD(seek) {
  NSNumber *position = [params isKindOfClass:[NSDictionary class]] ? params[@"position"] : nil;
  if (!position) {
    [self invokeCallback:callback code:kUIMethodParamInvalid data:@"missing position param"];
    return;
  }
  if (![position isKindOfClass:[NSNumber class]]) {
    [self invokeCallback:callback
                    code:kUIMethodParamInvalid
                    data:@"position param must be a number"];
    return;
  }

  LynxVideoPendingOperation *operation = [self pendingOperationWithCallback:callback];
  __weak typeof(self) weakSelf = self;
  [self.operationScheduler enqueueOperationWithName:@"seek"
      executor:^(LynxVideoOperationCompletion completion) {
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf) {
          completion();
          return;
        }
        operation.completion = completion;
        NSTimeInterval targetPosition = position.doubleValue;
        if (![strongSelf validateSeekPosition:targetPosition]) {
          [strongSelf finishPendingOperation:operation
                                        code:kUIMethodParamInvalid
                                        data:@"position out of range"];
          return;
        }
        [strongSelf.videoView seekTo:position.doubleValue
                          completion:^(BOOL success, NSString *_Nullable errorMsg) {
                            [strongSelf finishPendingOperation:operation
                                                          code:success ? kUIMethodSuccess
                                                                       : kUIMethodOperationError
                                                          data:success ? nil : errorMsg];
                          }];
      }
      cancelHandler:^{
        __strong typeof(weakSelf) strongSelf = weakSelf;
        [strongSelf finishPendingOperation:operation
                                      code:kUIMethodOperationError
                                      data:kLynxVideoSourceChangeCancelMessage];
      }];
}

- (LynxVideoPendingOperation *)pendingOperationWithCallback:(LynxUIMethodCallbackBlock)callback {
  LynxVideoPendingOperation *operation = [[LynxVideoPendingOperation alloc] init];
  operation.callback = callback;
  return operation;
}

- (BOOL)validateSeekPosition:(NSTimeInterval)position {
  NSTimeInterval duration = self.videoView.duration;
  if (!LynxVideoIsRepresentableSeekPosition(position) || position < 0 ||
      (duration > 0 && position > duration)) {
    return NO;
  }
  return YES;
}

- (void)executePlayWithOperation:(LynxVideoPendingOperation *)operation
                      completion:(LynxVideoOperationCompletion)completion {
  operation.completion = completion;
  if (self.videoView.src.length == 0) {
    [self dispatchErrorWithCode:LynxVideoErrorInvalidSource message:@"missing video source"];
    [self finishPendingOperation:operation
                            code:kUIMethodOperationError
                            data:@"missing video source"];
    return;
  }

  [self ensurePlaybackAudioSession];
  if (self.videoView.isPlaying) {
    [self finishPendingOperation:operation code:kUIMethodSuccess data:nil];
    return;
  }

  [self.pendingPlayOperations addObject:operation];
  [self.videoView play];
}

- (void)ensurePlaybackAudioSession {
  AVAudioSession *session = [AVAudioSession sharedInstance];
  if (![session.category isEqualToString:AVAudioSessionCategoryPlayback]) {
    NSError *error = nil;
    if (![session setCategory:AVAudioSessionCategoryPlayback error:&error]) {
      LLogWarn(@"[LynxVideo] Failed to set AVAudioSession category to Playback: %@", error);
    }
  }
}

- (LynxVideoObjectFit)objectFitFromString:(NSString *)value {
  if ([value isEqualToString:@"cover"]) {
    return LynxVideoObjectFitCover;
  }
  if ([value isEqualToString:@"fill"]) {
    return LynxVideoObjectFitFill;
  }
  return LynxVideoObjectFitContain;
}

- (LynxVideoOperationMode)operationModeFromString:(NSString *)value {
  if ([value isEqualToString:@"direct"]) {
    return LynxVideoOperationModeDirect;
  }
  if ([value isEqualToString:@"latest"]) {
    return LynxVideoOperationModeLatest;
  }
  return LynxVideoOperationModeQueue;
}

- (void)invokeCallback:(LynxUIMethodCallbackBlock)callback code:(int)code data:(id)data {
  if (callback) {
    BOOL success = code == kUIMethodSuccess;
    id payload = data;
    if (success && !payload) {
      payload = @{@"success" : @YES};
    } else if (!success) {
      payload = @{@"success" : @NO, @"errorCode" : @(code), @"msg" : data ?: @"request failed"};
    }
    callback(code, payload);
  }
}

- (void)finishPendingOperation:(LynxVideoPendingOperation *)operation code:(int)code data:(id)data {
  if (!operation || operation.completed) {
    return;
  }
  operation.completed = YES;
  [self invokeCallback:operation.callback code:code data:data];
  if (operation.completion) {
    operation.completion();
  }
}

- (void)finishPendingPlayOperationsWithCode:(int)code data:(id)data {
  NSArray<LynxVideoPendingOperation *> *operations = [self.pendingPlayOperations copy];
  [self.pendingPlayOperations removeAllObjects];
  for (LynxVideoPendingOperation *operation in operations) {
    [self finishPendingOperation:operation code:code data:data];
  }
}

- (void)finishPendingStopOperationsWithCode:(int)code data:(id)data {
  NSArray<LynxVideoPendingOperation *> *operations = [self.pendingStopOperations copy];
  [self.pendingStopOperations removeAllObjects];
  for (LynxVideoPendingOperation *operation in operations) {
    [self finishPendingOperation:operation code:code data:data];
  }
}

- (void)cancelPendingOperationsWithMessage:(NSString *)message {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    NSString *messageCopy = [message copy];
    [self dispatchOnMainThread:^{
      [weakSelf cancelPendingOperationsWithMessage:messageCopy];
    }];
    return;
  }
  [self.operationScheduler cancelAllOperations];
  [self finishPendingPlayOperationsWithCode:kUIMethodOperationError data:message];
  [self finishPendingStopOperationsWithCode:kUIMethodOperationError data:message];
}

- (void)sendEventWithName:(NSString *)name detail:(nullable NSDictionary *)detail {
  [self.context.eventEmitter sendCustomEvent:[[LynxDetailEvent alloc] initWithName:name
                                                                        targetSign:[self sign]
                                                                            detail:detail ?: @{}]];
}

- (void)dispatchErrorWithCode:(NSInteger)errorCode message:(NSString *)errorMsg {
  [self sendEventWithName:@"error"
                   detail:@{
                     @"errorCode" : @(errorCode),
                     @"errorMsg" : errorMsg ?: @"unknown video error"
                   }];
}

- (void)dispatchOnMainThread:(dispatch_block_t)block {
  if (!block) {
    return;
  }
  if ([NSThread isMainThread]) {
    block();
    return;
  }
  dispatch_async(dispatch_get_main_queue(), block);
}

#pragma mark - LynxVideoPlayableDelegate

- (void)playableDidLoadFirstFrame:(id<LynxVideoPlayable>)playable
                         duration:(NSTimeInterval)duration {
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf sendEventWithName:@"firstframe" detail:@{@"duration" : @(duration)}];
  }];
}

- (void)playableDidStartPlaying:(id<LynxVideoPlayable>)playable {
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf sendEventWithName:@"playing" detail:nil];
    [strongSelf finishPendingPlayOperationsWithCode:kUIMethodSuccess data:nil];
  }];
}

- (void)playableDidPause:(id<LynxVideoPlayable>)playable {
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf sendEventWithName:@"paused" detail:nil];
  }];
}

- (void)playableDidStop:(id<LynxVideoPlayable>)playable {
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf sendEventWithName:@"stopped" detail:nil];
    [strongSelf finishPendingStopOperationsWithCode:kUIMethodSuccess data:nil];
  }];
}

- (void)playableDidEnd:(id<LynxVideoPlayable>)playable {
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf sendEventWithName:@"ended" detail:nil];
  }];
}

- (void)playableDidLoop:(id<LynxVideoPlayable>)playable {
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf sendEventWithName:@"looped" detail:nil];
  }];
}

- (void)playable:(id<LynxVideoPlayable>)playable
    didUpdateTime:(NSTimeInterval)current
         duration:(NSTimeInterval)duration {
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf sendEventWithName:@"timeupdate"
                           detail:@{
                             @"current" : @(current),
                             @"duration" : @(duration)
                           }];
  }];
}

- (void)playable:(id<LynxVideoPlayable>)playable didUpdateBuffering:(NSTimeInterval)buffering {
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf sendEventWithName:@"buffering" detail:@{@"buffering" : @(buffering)}];
  }];
}

- (void)playable:(id<LynxVideoPlayable>)playable
    didFailWithCode:(NSInteger)errorCode
            message:(NSString *)errorMsg {
  __weak typeof(self) weakSelf = self;
  [self dispatchOnMainThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf dispatchErrorWithCode:errorCode message:errorMsg];
    [strongSelf finishPendingPlayOperationsWithCode:kUIMethodOperationError data:errorMsg];
  }];
}

@end
