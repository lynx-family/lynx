// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxUIAnimaX.h>

#import <AnimaX/AnimaXAnimationListener.h>
#import <AnimaX/AnimaXContext.h>
#import <AnimaX/AnimaXImageView.h>
#import <AnimaX/AnimaXView.h>
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxContext+Internal.h>
#import <Lynx/LynxContext.h>
#import <Lynx/LynxEnv+Internal.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxForegroundProtocol.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI+Private.h>
#import <Lynx/LynxUIContext+Internal.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxView+Internal.h>
#import <XElement/LynxAnimaXAbility.h>
#import <XElement/LynxAnimaXImageDecoderServiceImpl.h>
#import <XElement/LynxAnimaXMonitorServiceImpl.h>
#import <XElement/LynxAnimaXPropsPrioritySetter.h>
#import <XElement/LynxAnimaXResourceFactoryServiceImpl.h>
#import <XElement/LynxAnimaXResourceFetcherLoader.h>
#include "core/shell/lynx_shell.h"
#include "include/base/visibility_state.h"
#include "src/base/log/log.h"
#include "src/player/animax_event.h"
#include "src/resource/resource_loader/ios/resource_util.h"

static NSString *const DisplayModeImage = @"image";
static NSString *const DisplayModeSurface = @"surface";
static NSString *const DisplayModeAuto = @"auto";

@interface LynxUIAnimaX () <LynxForegroundProtocol, AnimaXAnimationListener>

@property(nonatomic, assign) BOOL ignoreLynxLifecycle;
@property(nonatomic, assign) BOOL enableLynxTapLayerEvent;
@property(nonatomic, assign) BOOL hasReportMotionEvent;
@property(nonatomic, assign) BOOL enableMultiThreadAccelerate;
@property(nonatomic, assign) BOOL ignoreAttachStatus;

@property(nonatomic, strong) NSString *displayMode;
@property(nonatomic, strong) NSString *tag;

@property(nonatomic, strong) LynxAnimaXPropsPrioritySetter *propsPrioritySetter;
@property(nonatomic, strong) AnimaXContext *animaxContext;
@property(nonatomic, strong, nullable) AnimaXPlayer *animaxPlayer;
@property(nonatomic, strong, nullable) UIView<AnimaXPlayerProtocol, AnimaXViewProtocol> *animaxView;

@end

@interface LynxUI (AnimaX)
- (void)setOpacity:(CGFloat)value requestReset:(BOOL)requestReset;
@end

@implementation LynxUIAnimaX

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("animax-view")
#else
LYNX_REGISTER_UI("animax-view")
#endif

- (void)reportMotionEvent:(NSString *)src {
  if (!src.length || !self.context || self.hasReportMotionEvent ||
      ![LynxEnv stringValueToBool:[[LynxEnv sharedInstance]
                                      _stringFromExternalEnv:@"enable_motion_ui_report"]
                     defaultValue:NO]) {
    return;
  }

  self.hasReportMotionEvent = YES;
  [LynxEventReporter onEvent:@"lynxsdk_motion_ui_event"
                  instanceId:[self.context.rootView instanceId]
                propsBuilder:^NSDictionary<NSString *, NSObject *> * {
                  return @{@"component_name" : @"animax-view", @"src" : src};
                }];
}

- (int64_t)memoryUsageBytes {
  return [self.animaxPlayer memoryUsageBytes];
}

#pragma mark Props Setter

LYNX_PROP_SETTER("muted", muted, BOOL) {
  [self.propsPrioritySetter enqueueTask:^(AnimaXPlayer *player) {
    [player setMuted:value];
  }];
}

LYNX_PROP_SETTER("enable-audio", enableAudio, BOOL) {
  [self.propsPrioritySetter
      enqueueTask:^(AnimaXPlayer *player) {
        [player setEnableAudio:value];
      }
         priority:AnimaXHigh];
}

LYNX_PROP_SETTER("tag", tag, NSString *) {
  if (![value isKindOfClass:[NSString class]]) {
    return;
  }
  self.tag = value;
  [[self.animaxContext.ability getMonitorDelegate] setTag:value];
}

LYNX_PROP_SETTER("display-mode", displayMode, NSString *) {
  if (![value isKindOfClass:[NSString class]]) {
    return;
  }
  self.displayMode = value;
}

LYNX_PROP_SETTER("multi-thread-accelerate", multiThreadAccelerate, BOOL) {
  self.enableMultiThreadAccelerate = value;
}

LYNX_PROP_SETTER("src", src, NSString *) {
  if (![value isKindOfClass:[NSString class]]) {
    return;
  }
  if (![value length]) {
    return;
  }
  __weak typeof(self) weakSelf = self;
  [self.propsPrioritySetter
      enqueueTask:^(AnimaXPlayer *player) {
        __strong typeof(weakSelf) self = weakSelf;
        if (!self) {
          return;
        }
        [player setSrc:value];
        [self reportMotionEvent:value];
      }
         priority:AnimaXLow];
}

LYNX_PROP_SETTER("src-format", srcFormat, NSString *) {
  if (![value isKindOfClass:[NSString class]]) {
    return;
  }
  __weak typeof(self) weakSelf = self;
  [self.propsPrioritySetter
      enqueueTask:^(AnimaXPlayer *player) {
        __strong typeof(weakSelf) self = weakSelf;
        if (!self) {
          return;
        }
        [player setSrc:value];
        [self reportMotionEvent:value];
      }
         priority:AnimaXLow];
}

LYNX_PROP_SETTER("src-polyfill", srcPolyfill, NSDictionary *) {
  if (![value isKindOfClass:[NSDictionary class]]) {
    return;
  }
  [self.propsPrioritySetter
      enqueueTask:^(AnimaXPlayer *player) {
        [player setPolyfill:value];
      }
         priority:AnimaXHigh];
}

LYNX_PROP_SETTER("json", json, NSString *) {
  if (![value isKindOfClass:[NSString class]]) {
    return;
  }
  __weak typeof(self) weakSelf = self;
  [self.propsPrioritySetter
      enqueueTask:^(AnimaXPlayer *player) {
        __strong typeof(weakSelf) self = weakSelf;
        if (!self) {
          return;
        }
        [player setJson:value];
        [self reportMotionEvent:value];
      }
         priority:AnimaXLow];
}

LYNX_PROP_SETTER("ignore-attach-status", ignoreAttachStatus, BOOL) {
  if (self.animaxView) {
    self.animaxView.ignoreAttachStatus = value;
  } else {
    self.ignoreAttachStatus = value;
  }
}

LYNX_PROP_SETTER("loop", loop, BOOL) {
  [self.propsPrioritySetter enqueueTask:^(AnimaXPlayer *player) {
    [player setLoop:value];
  }];
}

LYNX_PROP_SETTER("start-frame", startFrame, NSNumber *) {
  if (![value isKindOfClass:[NSNumber class]]) {
    return;
  }
  [self.propsPrioritySetter enqueueTask:^(AnimaXPlayer *player) {
    [player setStartFrame:[value doubleValue]];
  }];
}

LYNX_PROP_SETTER("end-frame", endFrame, NSNumber *) {
  if (![value isKindOfClass:[NSNumber class]]) {
    return;
  }
  [self.propsPrioritySetter enqueueTask:^(AnimaXPlayer *player) {
    [player setEndFrame:[value doubleValue]];
  }];
}

LYNX_PROP_SETTER("auto-reverse", autoReverse, BOOL) {
  [self.propsPrioritySetter enqueueTask:^(AnimaXPlayer *player) {
    [player setAutoReverse:value];
  }];
}

LYNX_PROP_SETTER("progress", progress, NSNumber *) {
  if (![value isKindOfClass:[NSNumber class]]) {
    return;
  }
  [self.propsPrioritySetter enqueueTask:^(AnimaXPlayer *player) {
    [player setProgress:[value doubleValue]];
  }];
}

LYNX_PROP_SETTER("loop-count", repeatCount, NSNumber *) {
  if (![value isKindOfClass:[NSNumber class]]) {
    return;
  }
  [self.propsPrioritySetter enqueueTask:^(AnimaXPlayer *player) {
    [player setLoopCount:[value intValue]];
  }];
}

LYNX_PROP_SETTER("objectfit", objectfit, NSString *) {
  if (![value isKindOfClass:[NSString class]]) {
    return;
  }
  [self.propsPrioritySetter enqueueTask:^(AnimaXPlayer *player) {
    [player setObjectfit:value];
  }];
}

LYNX_PROP_SETTER("object-position", objectposition, NSString *) {
  if (![value isKindOfClass:[NSString class]]) {
    return;
  }
  [self.propsPrioritySetter enqueueTask:^(AnimaXPlayer *player) {
    [player setObjectPosition:value];
  }];
}

LYNX_PROP_SETTER("autoplay", autoplay, BOOL) {
  [self.propsPrioritySetter enqueueTask:^(AnimaXPlayer *player) {
    [player setAutoplay:value];
  }];
}

LYNX_PROP_SETTER("speed", speed, NSNumber *) {
  if (![value isKindOfClass:[NSNumber class]]) {
    return;
  }
  [self.propsPrioritySetter enqueueTask:^(AnimaXPlayer *player) {
    [player setSpeed:[value doubleValue]];
  }];
}

LYNX_PROP_SETTER("fps-event-interval", interval, NSNumber *) {
  if (![value isKindOfClass:[NSNumber class]]) {
    return;
  }
  [self.propsPrioritySetter enqueueTask:^(AnimaXPlayer *player) {
    [player setFPSEventInterval:[value longValue]];
  }];
}

LYNX_PROP_SETTER("max-frame-rate", maxFrameRate, NSNumber *) {
  if (![value isKindOfClass:[NSNumber class]]) {
    return;
  }
  [self.propsPrioritySetter enqueueTask:^(AnimaXPlayer *player) {
    [player setMaxFrameRate:[value doubleValue]];
  }];
}

LYNX_PROP_SETTER("anti-aliasing", setAntiAlias, NSString *) {
  // Deprecated
}

LYNX_PROP_SETTER("dynamic-resource", dynamicResource, BOOL) {
  [self.propsPrioritySetter
      enqueueTask:^(AnimaXPlayer *player) {
        [player setDynamicResource:value];
      }
         priority:AnimaXHigh];
}

LYNX_PROP_SETTER("ignore-lynx-lifecycle", ignoreLynxLifecycle, BOOL) {
  self.ignoreLynxLifecycle = value;
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  if (!self.animaxPlayer) {
    [self createPlayer];
  }
  if (!self.animaxView) {
    [self createAnimaXView];
  }
  [self.propsPrioritySetter flush];
}

#pragma mark Layout

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with {
  [super updateFrame:(CGRect)frame
              withPadding:(UIEdgeInsets)padding
                   border:(UIEdgeInsets)border
                   margin:(UIEdgeInsets)margin
      withLayoutAnimation:(BOOL)with];
  UIEdgeInsets containerPadding =
      UIEdgeInsetsMake(padding.top + border.top, padding.left + border.left,
                       padding.bottom + border.bottom, padding.right + border.right);
  self.view.padding = containerPadding;
}

#pragma mark Touch

- (void)eventDidSet {
  [super eventDidSet];

  // Only enable layer event when an event starts with "taplayers".
  BOOL enableTapLayerEvent = NO;
  for (NSString *event in self.eventSet) {
    if ([event hasPrefix:@"taplayers"]) {
      enableTapLayerEvent = YES;
      break;
    }
  }
  self.enableLynxTapLayerEvent = enableTapLayerEvent;
}

- (bool)dispatchTouch:(NSString *const)touchType
              touches:(NSSet<UITouch *> *)touches
            withEvent:(UIEvent *)event {
  if (self.animaxView && self.enableLynxTapLayerEvent &&
      [touchType isEqualToString:LynxEventTouchEnd]) {
    [self.animaxView handleTouch:touches withEvent:event];
  }
  return [super dispatchTouch:touchType touches:touches withEvent:event];
}

#pragma mark UI Method

- (void)onNodeReload {
  [super onNodeReload];
  [self.animaxPlayer reload];
}

LYNX_UI_METHOD(play) {
  [self.animaxPlayer play];
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_UI_METHOD(resume) {
  [self.animaxPlayer resume];
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_UI_METHOD(stop) {
  [self.animaxPlayer stop];
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_UI_METHOD(pause) {
  [self.animaxPlayer pause];
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_UI_METHOD(getDuration) {
  if (callback) {
    callback(
        kUIMethodSuccess,
        @{@"data" : @([self.animaxPlayer durationInMS])});
  }
}

LYNX_UI_METHOD(isAnimating) {
  if (callback) {
    callback(
        kUIMethodSuccess,
        @{@"data" : @([self.animaxPlayer isAnimating])});
  }
}

LYNX_UI_METHOD(subscribeUpdateEvent) {
  NSNumber *frame = params[@"frame"];
  if ([frame isKindOfClass:[NSNumber class]]) {
    [self.animaxPlayer subscribeUpdateEvent:[frame intValue]];
    if (callback) {
      callback(kUIMethodSuccess, nil);
    }
  } else {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
  }
}

LYNX_UI_METHOD(subscribeUpdateEvents) {
  NSArray *frames = params[@"frames"];
  if ([frames isKindOfClass:[NSArray class]]) {
    [self.animaxPlayer subscribeUpdateEvents:frames subscribe:YES];
    if (callback) {
      callback(kUIMethodSuccess, nil);
    }
  } else {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
  }
}

LYNX_UI_METHOD(unsubscribeUpdateEvent) {
  NSNumber *frame = params[@"frame"];
  if ([frame isKindOfClass:[NSNumber class]]) {
    [self.animaxPlayer unsubscribeUpdateEvent:[frame intValue]];
    if (callback) {
      callback(kUIMethodSuccess, nil);
    }
  } else {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
  }
}

LYNX_UI_METHOD(unsubscribeUpdateEvents) {
  NSArray *frames = params[@"frames"];
  if ([frames isKindOfClass:[NSArray class]]) {
    [self.animaxPlayer subscribeUpdateEvents:frames subscribe:NO];
    if (callback) {
      callback(kUIMethodSuccess, nil);
    }
  } else {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
  }
}

LYNX_UI_METHOD(seek) {
  NSNumber *frame = params[@"frame"];
  if ([frame isKindOfClass:NSNumber.class]) {
    [self.animaxPlayer seekTo:[frame doubleValue]];
    if (callback) {
      callback(kUIMethodSuccess, nil);
    }
  } else {
    if (callback) {
      callback(kUIMethodParamInvalid, nil);
    }
  }
}

LYNX_UI_METHOD(getCurrentFrame) {
  if (callback) {
    callback(kUIMethodSuccess, @([self.animaxPlayer currentFrame]));
  }
}

LYNX_UI_METHOD(playSegment) {
  NSNumber *startFrame = params[@"startFrame"];
  NSNumber *endFrame = params[@"endFrame"];
  if ([startFrame isKindOfClass:NSNumber.class] && [endFrame isKindOfClass:NSNumber.class]) {
    if ([endFrame doubleValue] > 0 && [startFrame doubleValue] > [endFrame doubleValue]) {
      if (callback) {
        callback(kUIMethodParamInvalid, @"startFrame and endFrame are not valid!");
      }
      return;
    }
    [self.animaxPlayer playFrom:[startFrame doubleValue] to:[endFrame doubleValue]];

    if (callback) {
      callback(kUIMethodSuccess, nil);
    }
  } else {
    if (callback) {
      callback(kUIMethodParamInvalid, @"startFrame and endFrame type are not valid!");
    }
  }
}

#pragma mark Init

- (void)createAnimaXView {
  if (!self.animaxPlayer || !self.view) {
    return;
  }

  BOOL useImageDisplayMode =
      [self.displayMode isEqualToString:DisplayModeImage] ||
      ([self.displayMode isEqualToString:DisplayModeAuto] && self.tag.length > 0 &&
       [LynxEnv stringValueToBool:[[LynxEnv sharedInstance]
                                      _stringFromExternalEnv:
                                          [NSString stringWithFormat:@"ANIMAX_USE_IMAGE_VIEW_%@",
                                                                     self.tag]]
                     defaultValue:NO]);
  if (useImageDisplayMode) {
    ANIMAX_LOGI("create AnimaXImageView");
    [[self.animaxContext.ability getMonitorDelegate] setDisplayMode:DisplayModeImage];
    self.animaxView = [[AnimaXImageView alloc] initWithPlayer:self.animaxPlayer];
  } else {
    ANIMAX_LOGI("create AnimaXView");
    [[self.animaxContext.ability getMonitorDelegate] setDisplayMode:DisplayModeSurface];
    self.animaxView = [[AnimaXView alloc] initWithPlayer:self.animaxPlayer];
  }
  self.view.contentView = self.animaxView;
  self.animaxView.ignoreAttachStatus = self.ignoreAttachStatus;
  // Ensure platform AnimaXView do not send touch event to element.
  // Only LynxUIAnimaX can send event by dispatchTouch from LynxUI.
  self.animaxView.enableNativeTapLayerEvent = NO;
}

- (void)createPlayer {
  if (self.animaxContext == nil) {
    return;
  }
  self.animaxContext.enableMultiThreadAccelerate = self.enableMultiThreadAccelerate;
  self.animaxPlayer = [[AnimaXPlayer alloc] initWithContext:self.animaxContext];
  [self.propsPrioritySetter attachToPlayer:self.animaxPlayer];
}

- (LynxAnimaXContainerView *)createView {
  LynxAnimaXAbility *ability = [[LynxAnimaXAbility alloc] init];
  [ability addAnimationListener:self];
  AnimaXContext *animaxContext = [[AnimaXContext alloc] initWithAbility:ability];
  animaxContext.disablePlaybackOnAssetLoadFailure = [LynxEnv
      stringValueToBool:[[LynxEnv sharedInstance]
                            _stringFromExternalEnv:@"ANIMAX_DISABLE_PLAYBACK_ON_ASSET_LOAD_FAILURE"]
           defaultValue:NO];
  animaxContext.enableDownsampleVideo =
      [LynxEnv stringValueToBool:[[LynxEnv sharedInstance]
                                     _stringFromExternalEnv:@"ANIMAX_ENABLE_DOWNSAMPLE_VIDEO"]
                    defaultValue:NO];
  self.animaxContext = animaxContext;
  LynxAnimaXContainerView *container = [[LynxAnimaXContainerView alloc] init];
  return container;
}

- (void)setContext:(LynxUIContext *)context {
  [super setContext:context];

  if (context == nullptr) {
    return;
  }

  LynxAnimaXAbility *ability = (LynxAnimaXAbility *)self.animaxContext.ability;

  // Regsiter Monitor Service
  LynxAnimaXMonitorServiceImpl *monitorServiceImpl =
      [[LynxAnimaXMonitorServiceImpl alloc] initWithLynxUIContext:context];
  [ability registerService:@protocol(AnimaXMonitorService) withImpl:monitorServiceImpl];

  // Register Resource Factory Service
  LynxAnimaXResourceFactoryServiceImpl *resourceFactoryServiceImpl =
      [[LynxAnimaXResourceFactoryServiceImpl alloc] initWithLynxUIContext:context];
  [ability registerService:@protocol(AnimaXResourceFactoryService)
                  withImpl:resourceFactoryServiceImpl];

  // Register Image Decoder Service
  BOOL enableImageDecoderService =
      [LynxEnv stringValueToBool:[[LynxEnv sharedInstance]
                                     _stringFromExternalEnv:@"enable_image_decoder_service"]
                    defaultValue:YES];
  if (enableImageDecoderService) {
    [ability registerService:@protocol(AnimaXImageDecoderService)
                    withImpl:[LynxAnimaXImageDecoderServiceImpl new]];
  }
}

- (instancetype)init {
  if (self = [super init]) {
    _ignoreLynxLifecycle = NO;
    _propsPrioritySetter = [[LynxAnimaXPropsPrioritySetter alloc] init];
    _displayMode = DisplayModeSurface;
  }
  return self;
}

- (void)sendEventToJS:(NSString *)eventName params:(NSDictionary *)params {
  if (![eventName length]) {
    return;
  }
  LynxCustomEvent *customEvent = [[LynxDetailEvent alloc] initWithName:eventName
                                                            targetSign:[self sign]
                                                                detail:params];
  [self.context.eventEmitter sendCustomEvent:customEvent];
}

#pragma mark - LynxForegroundProtocol

// means LynxView enters foreground
- (void)onEnterForeground {
  if (self.ignoreLynxLifecycle) {
    return;
  }
  [self.animaxPlayer enterForeground];
}

// means LynxView enters background
- (void)onEnterBackground {
  if (self.ignoreLynxLifecycle) {
    return;
  }
  [self.animaxPlayer enterBackground];
}

#pragma mark - AnimaXAnimationListener

- (void)onCompletion:(NSDictionary *)params {
  [self sendEventToJS:@"completion" params:params];
}

- (void)onStart:(NSDictionary *)params {
  [self sendEventToJS:@"start" params:params];
}

- (void)onRepeat:(NSDictionary *)params {
  [self sendEventToJS:@"repeat" params:params];
}

- (void)onCancel:(NSDictionary *)params {
  [self sendEventToJS:@"cancel" params:params];
}

- (void)onReady:(NSDictionary *)params {
  [self sendEventToJS:@"ready" params:params];
}

- (void)onUpdate:(NSDictionary *)params {
  [self sendEventToJS:@"update" params:params];
}

- (void)onError:(NSDictionary *)params {
  [self sendEventToJS:@"error" params:params];
  [self showLogBox:params level:LynxErrorLevelError];
}

- (void)onWarning:(NSDictionary *)params {
  [self sendEventToJS:@"warning" params:params];
  [self showLogBox:params level:LynxErrorLevelWarn];
}

- (void)onFps:(NSDictionary *)params {
  [self sendEventToJS:@"fps" params:params];
}

- (void)onTapLayers:(NSDictionary *)params {
  [self sendEventToJS:@"taplayers" params:params];
}

- (void)onFirstFrame:(NSDictionary *)params {
  [self sendEventToJS:@"firstframe" params:params];
}

- (void)onCompositionReady:(NSDictionary *)params {
  [self sendEventToJS:@"compositionready" params:params];
}

- (void)showLogBox:(NSDictionary *)params level:(NSString *)level {
  NSNumber *code = [params objectForKey:@(lynx::animax::EventKeys::kCode)];
  NSString *message = [params objectForKey:@(lynx::animax::EventKeys::kMessage)];
  [self.context reportLynxError:[LynxError lynxErrorWithCode:code.intValue
                                                     message:message
                                               fixSuggestion:@""
                                                       level:level
                                                  customInfo:nil
                                                isLogBoxOnly:true]];
}

@end
