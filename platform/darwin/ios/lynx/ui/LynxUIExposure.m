// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIExposure.h"
#import "LynxEnv.h"
#import "LynxGlobalObserver+Internal.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxTraceEvent.h"
#import "LynxTraceEventWrapper.h"
#import "LynxUI+Internal.h"
#import "LynxUIKitAPIAdapter.h"
#import "LynxUnitUtils.h"
#import "LynxView+Internal.h"
#import "LynxWeakProxy.h"

@interface LynxUIExposureDetail : NSObject
@property(nonatomic, copy) NSString *exposureScene;
@property(nonatomic, copy) NSString *exposureID;
@property(nonatomic, copy) NSString *internalSignature;
@property(nonatomic) NSInteger sign;
@property(nonatomic, weak) LynxUI *ui;
@property(nonatomic) NSDictionary *dataSet;
@property(nonatomic) NSString *uniqueID;
@property(nonatomic) NSDictionary *extraData;
@property(nonatomic) NSDictionary *useOptions;
@end

@implementation LynxUIExposureDetail
- (instancetype)initWithUI:(LynxUI *)ui
          uniqueIdentifier:(NSString *_Nullable)uniqueID
                 extraData:(NSDictionary *_Nullable)data
                useOptions:(NSDictionary *_Nullable)options {
  if (self = [super init]) {
    self.exposureID = ui.exposureID ?: @"";
    // When user not set ui.exposure-scene, we use the default value: "".
    self.exposureScene = ui.exposureScene ?: @"";
    self.sign = ui.sign;
    self.ui = ui;
    self.internalSignature = ui.internalSignature ?: @"";
    self.dataSet = [NSDictionary dictionaryWithDictionary:ui.dataset];
    self.uniqueID = uniqueID ? uniqueID : @"";
    self.extraData = data ? data : [NSDictionary dictionaryWithDictionary:data];
    self.useOptions = options;
  }
  return self;
}

- (BOOL)isEqual:(id)object {
  if (self == object) {
    return YES;
  }
  if (![object isKindOfClass:[LynxUIExposureDetail class]]) {
    return NO;
  }
  return [self isEqualToExposureDetail:object];
}

- (BOOL)isEqualToExposureDetail:(LynxUIExposureDetail *)other {
  return [_exposureID isEqualToString:other.exposureID] &&
         [_exposureScene isEqualToString:other.exposureScene] &&
         [_internalSignature isEqualToString:other.internalSignature] && _sign == other.sign &&
         [_uniqueID isEqualToString:other.uniqueID];
}

- (NSUInteger)hash {
  return _ui.hash;
}

- (NSDictionary *)toDictionary {
  NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];
  [dict setObject:self.exposureID forKey:@"exposure-id"];
  [dict setObject:self.exposureScene forKey:@"exposure-scene"];
  [dict setObject:self.internalSignature forKey:@"internal-signature"];
  [dict setObject:self.dataSet forKey:@"dataset"];
  [dict setValue:self.uniqueID forKey:@"unique-id"];
  [dict setObject:self.extraData forKey:@"extra-data"];

  return dict;
}
@end

@interface LynxUIExposure ()
// A System resource for call the exposure handler cyclical
@property(nonatomic) CADisplayLink *displayLink;
// Global Event sender
@property(nonatomic, weak) LynxRootUI *rootUI;
// A map hold all effective UIs which be assigned exposured property,
@property(nonatomic) NSMutableDictionary<NSString *, LynxUIExposureDetail *> *exposedLynxUIMap;
// A map hold every UI which has exposure property and stay inside window.
@property(nonatomic) NSMutableSet<LynxUIExposureDetail *> *uiInWindowMapNow;
// ditto, but UIs from previous moment
@property(nonatomic) NSMutableSet<LynxUIExposureDetail *> *uiInWindowMapBefore;
// A map record the UI disexposured current circle
@property(nonatomic) NSMutableSet<LynxUIExposureDetail *> *disappearSet;
// A map record the UI sexposured current circle
@property(nonatomic) NSMutableSet<LynxUIExposureDetail *> *appearSet;
// The frame of LynxView in last runloop.
@property(nonatomic) CGRect lynxViewOldFrame;
// Be in charge of executing callback to modify the flag when the UI has changed.
@property(nonatomic) LynxGlobalObserver *observer;
// The flag that reflects whether the UI has changed.
@property(nonatomic) BOOL flag;
// The switch controlling whether to enable exposure detection optimization.
@property(nonatomic) BOOL enableCheckExposureOptimize;
// The flag that reflects whether the stopExposure is called.
@property(nonatomic) BOOL isStopExposure;
@end

@implementation LynxUIExposure {
  // Detection frequency, the default value is 20fps.
  int32_t _frameRate;
  // The callback that changes flag to YES reflecting the UI has changed.
  void (^_callback)(NSDictionary *);
}

- (void)didExposure {
  _flag = NO;
}

- (instancetype)init {
  if (self = [super init]) {
    _exposedLynxUIMap = [[NSMutableDictionary alloc] init];
    _uiInWindowMapNow = [[NSMutableSet alloc] init];
    _uiInWindowMapBefore = [[NSMutableSet alloc] init];
    _disappearSet = [[NSMutableSet alloc] init];
    _appearSet = [[NSMutableSet alloc] init];
    _frameRate = 20;
    _lynxViewOldFrame = CGRectZero;
    _flag = NO;
    _enableCheckExposureOptimize = NO;
  }
  return self;
}

- (instancetype)initWithObserver:observer {
  if (self = [self init]) {
    __weak typeof(self) weakSelf = self;
    _callback = ^void(NSDictionary *dict) {
      // Use weak reference to avoid retain cycle.
      __strong typeof(weakSelf) strongSelf = weakSelf;
      strongSelf.flag = YES;
    };

    _observer = observer;
    [_observer addAnimationObserver:_callback];
    [_observer addLayoutObserver:_callback];
    [_observer addScrollObserver:_callback];
    [_observer addPropertyObserver:_callback];
  }
  return self;
}

- (void)dealloc {
  [_observer removeAnimationObserver:_callback];
  [_observer removeLayoutObserver:_callback];
  [_observer removeScrollObserver:_callback];
  [_observer removePropertyObserver:_callback];
}

- (void)setRootUI:(LynxRootUI *)rootUI {
  _rootUI = rootUI;
}

// set frame rate by page config
- (void)setObserverFrameRate:(int32_t)rate {
  // If rate < 0, do not use this negative value.
  if (rate < 0) {
    return;
  }
  // If rate > 60, let _frameRate = 60.
  if (rate > 60) {
    _frameRate = 60;
    return;
  }
  _frameRate = rate;
}

// set frame rate by js api
- (void)setObserverFrameRateDynamic:(NSDictionary *)options {
}

- (void)setEnableCheckExposureOptimize:(BOOL)enableCheckExposureOptimize {
  _enableCheckExposureOptimize = enableCheckExposureOptimize;
}

- (CGFloat)getIntersectionAreaRatio:(CGRect)targetRect otherRect:(CGRect)otherRect {
  CGRect intersectionRect = CGRectIntersection(targetRect, otherRect);
  if (!CGRectIsNull(intersectionRect)) {
    CGFloat originArea = targetRect.size.height * targetRect.size.width;
    CGFloat intersectionArea = intersectionRect.size.height * intersectionRect.size.width;
    return intersectionArea / originArea;
  } else {
    return 0;
  }
}

- (BOOL)checkIntersect:(CGRect)targetRect otherRect:(CGRect)otherRect ratio:(CGFloat)ratio {
  // ratio's default value is 0, and when ratio = 0, only calculate whether two rect are
  // intersected.
  if (ratio != 0) {
    return [self getIntersectionAreaRatio:targetRect otherRect:otherRect] >= ratio;
  } else {
    return CGRectIntersectsRect(targetRect, otherRect);
  }
}

- (CGRect)frameOfUIInWindow:(LynxUI *)ui {
  CGRect frameOfUIInWindow = [ui getBoundingClientRectToScreen];
  if ([ui enableExposureUIMargin]) {
    // get UI's frame, calculate the needed rect with exposureUIMargin
    CGFloat width = frameOfUIInWindow.size.width;
    CGFloat height = frameOfUIInWindow.size.height;
    LynxScreenMetrics *screenMetrics = [LynxScreenMetrics getDefaultLynxScreenMetrics];

    NSString *left = ui.exposureUIMarginLeft;
    CGFloat left_ = [LynxUnitUtils toPtWithScreenMetrics:screenMetrics
                                               unitValue:left
                                            rootFontSize:0
                                             curFontSize:0
                                               rootWidth:0
                                              rootHeight:0
                                                viewSize:width
                                           withDefaultPt:0];

    NSString *right = ui.exposureUIMarginRight;
    CGFloat right_ = [LynxUnitUtils toPtWithScreenMetrics:screenMetrics
                                                unitValue:right
                                             rootFontSize:0
                                              curFontSize:0
                                                rootWidth:0
                                               rootHeight:0
                                                 viewSize:width
                                            withDefaultPt:0];

    NSString *top = ui.exposureUIMarginTop;
    CGFloat top_ = [LynxUnitUtils toPtWithScreenMetrics:screenMetrics
                                              unitValue:top
                                           rootFontSize:0
                                            curFontSize:0
                                              rootWidth:0
                                             rootHeight:0
                                               viewSize:height
                                          withDefaultPt:0];

    NSString *bottom = ui.exposureUIMarginBottom;
    CGFloat bottom_ = [LynxUnitUtils toPtWithScreenMetrics:screenMetrics
                                                 unitValue:bottom
                                              rootFontSize:0
                                               curFontSize:0
                                                 rootWidth:0
                                                rootHeight:0
                                                  viewSize:height
                                             withDefaultPt:0];

    // uiRect's area < 0
    if (width + left_ + right_ <= 0 || height + top_ + bottom_ <= 0) {
      return CGRectNull;
    }

    frameOfUIInWindow.origin.x -= left_;
    frameOfUIInWindow.origin.y -= top_;
    frameOfUIInWindow.size.width += left_ + right_;
    frameOfUIInWindow.size.height += top_ + bottom_;

  } else {
    // old logic, when exposureMargin > 0, calculate ui's rect side instead of screen's rect side
    frameOfUIInWindow.origin.x -= ui.exposureMarginRight > 0 ? ui.exposureMarginRight : 0;
    frameOfUIInWindow.origin.y -= ui.exposureMarginBottom > 0 ? ui.exposureMarginBottom : 0;
    frameOfUIInWindow.size.width += (ui.exposureMarginRight > 0 ? ui.exposureMarginRight : 0) +
                                    (ui.exposureMarginLeft > 0 ? ui.exposureMarginLeft : 0);
    frameOfUIInWindow.size.height += (ui.exposureMarginBottom > 0 ? ui.exposureMarginBottom : 0) +
                                     (ui.exposureMarginTop > 0 ? ui.exposureMarginTop : 0);
  }
  return frameOfUIInWindow;
}

- (CGRect)borderOfExposureScreen:(LynxUI *)ui {
  if ([ui enableExposureUIMargin]) {
    // get screen's frame, calculate the needed rect with exposureMargin (not exposureUIMargin)
    CGRect borderOfExposureScreen = [LynxUIKitAPIAdapter getKeyWindow].bounds;

    // screenRect's area < 0
    if (borderOfExposureScreen.size.width + ui.exposureMarginLeft + ui.exposureMarginRight <= 0 ||
        borderOfExposureScreen.size.height + ui.exposureMarginTop + ui.exposureMarginBottom <= 0) {
      return CGRectNull;
    }
    borderOfExposureScreen.origin.x -= ui.exposureMarginLeft;
    borderOfExposureScreen.origin.y -= ui.exposureMarginTop;
    borderOfExposureScreen.size.width += ui.exposureMarginLeft + ui.exposureMarginRight;
    borderOfExposureScreen.size.height += ui.exposureMarginTop + ui.exposureMarginBottom;
    return borderOfExposureScreen;
  } else {
    // old logic, when exposureMargin < 0, calculate screen's rect side
    CGRect borderOfExposureScreen =
        CGRectMake([UIScreen mainScreen].bounds.origin.x -
                       (ui.exposureMarginLeft < 0 ? ui.exposureMarginLeft : 0),
                   [UIScreen mainScreen].bounds.origin.y -
                       (ui.exposureMarginTop < 0 ? ui.exposureMarginTop : 0),
                   [UIScreen mainScreen].bounds.size.width +
                       (ui.exposureMarginLeft < 0 ? ui.exposureMarginLeft : 0) +
                       (ui.exposureMarginRight < 0 ? ui.exposureMarginRight : 0),
                   [UIScreen mainScreen].bounds.size.height +
                       (ui.exposureMarginTop < 0 ? ui.exposureMarginTop : 0) +
                       (ui.exposureMarginBottom < 0 ? ui.exposureMarginBottom : 0));
    return borderOfExposureScreen;
  }
}

- (BOOL)uiInWindow:(LynxUI *)ui {
  // if need to support Animation, release next code
  /*
      CGRect frameOfUIInWindow =
      [[ui getParent].view convertRect:[ui.view.layer presentationLayer].frame toView:nil];
  */
  // if ui's height == 0 || ui's width == 0, return NO, see issue:#7731
  if (ui.frame.size.width == 0 || ui.frame.size.height == 0) {
    return NO;
  }

  LynxUI *parent = ui;
  NSMutableArray<LynxUI *> *parentAry = [NSMutableArray new];
  BOOL isInOverlay = NO;
  while (parent != nil && parent != _rootUI) {
    if (![parent isVisible]) {
      return NO;
    }
    if ([parent isScrollContainer]) {
      [parentAry addObject:parent];
    }
    LynxUI *uiParent = parent.parent;
    while (uiParent.view != nil && ![parent.view isDescendantOfView:uiParent.view]) {
      uiParent = uiParent.parent;
    }
    if ([parent isOverlay]) {
      isInOverlay = YES;
      break;
    }
    parent = uiParent;
  }
  __block BOOL uiInWindow = YES;
  CGRect frameOfUIInWindow = [self frameOfUIInWindow:ui];
  CGFloat exposureAreaRatio = 0;
  if ([LynxUnitUtils isPercentage:ui.exposureArea]) {
    exposureAreaRatio =
        [[ui.exposureArea substringToIndex:ui.exposureArea.length - 1] floatValue] / 100;
  }

  [parentAry enumerateObjectsUsingBlock:^(LynxUI *parent, NSUInteger idx, BOOL *stop) {
    CGRect frameOfParent = [parent getBoundingClientRectToScreen];
    if (![self checkIntersect:frameOfUIInWindow otherRect:frameOfParent ratio:exposureAreaRatio]) {
      uiInWindow = NO;
      *stop = YES;
    }
  }];

  if (!uiInWindow) {
    return NO;
  }

  CGRect frameOfRootUIInWindow = [_rootUI getBoundingClientRectToScreen];
  CGRect borderOfExposureScreen = [self borderOfExposureScreen:ui];
  BOOL isIntersectWithRoot = !isInOverlay ? [self checkIntersect:frameOfUIInWindow
                                                       otherRect:frameOfRootUIInWindow
                                                           ratio:exposureAreaRatio]
                                          : YES;
  BOOL isIntersectWithScreen = [self checkIntersect:frameOfUIInWindow
                                          otherRect:borderOfExposureScreen
                                              ratio:exposureAreaRatio];
  return isIntersectWithRoot && isIntersectWithScreen;
}

- (BOOL)isLynxViewChanged {
  CGRect lynxViewNewFrame = [_rootUI.rootView convertRect:_rootUI.rootView.bounds toView:nil];

  // It means LynxView doesn't change when the new frame is equal to the old frame.
  if (CGRectEqualToRect(_lynxViewOldFrame, lynxViewNewFrame)) {
    _lynxViewOldFrame = lynxViewNewFrame;
    return NO;
  }
  _lynxViewOldFrame = lynxViewNewFrame;
  return YES;
}

- (void)exposureHandler:(CADisplayLink *)sender {
  // Avoid performing exposure detection tasks after calling stopExposure.
  if (_isStopExposure) {
    return;
  }

  [LynxTraceEvent beginSection:LYNX_TRACE_CATEGORY_WRAPPER
                      withName:@"LynxUIExposure.exposureHandler"];

  // There is no need to detect exposure when UI and LynxView haven't changed.
  if (_enableCheckExposureOptimize && !_flag && ![self isLynxViewChanged]) {
    [LynxTraceEvent endSection:LYNX_TRACE_CATEGORY_WRAPPER];
    return;
  }
  // The flag should be reset to NO before the next runloop.
  [self didExposure];

  // step 1: select UI in the screen from all exposured UI
  for (LynxUIExposureDetail *detail in [_exposedLynxUIMap allValues]) {
    if (detail.ui != nil) {
      LynxUI *lynxUI = detail.ui;
      if ([self uiInWindow:lynxUI]) {
        [_uiInWindowMapNow addObject:detail];
      }
    }
  }
  NSMutableSet<LynxUIExposureDetail *> *appearUIDetail =
      [NSMutableSet setWithSet:_uiInWindowMapNow];
  // step 2: in _uiInWindowMapNow, not in _uiInWindowMapBefore means the UI appear
  [appearUIDetail minusSet:_uiInWindowMapBefore];

  // step 3: in _uiInWindowMapBefore, not in _uiInWindowMapNow means the UI disappear
  NSMutableSet<LynxUIExposureDetail *> *disappearUIDetail =
      [NSMutableSet setWithSet:_uiInWindowMapBefore];
  [disappearUIDetail minusSet:_uiInWindowMapNow];

  _uiInWindowMapBefore = _uiInWindowMapNow;
  _uiInWindowMapNow = [[NSMutableSet alloc] init];

  // TODO(hexionghui): delete step 4
  // step 4: if a specific UI create exposure and disexposure Continuously, which is a bad case. so
  // we need an Buffering mechanism。
  // step 4.1 This UI disexposured at previous moment,and exposures at this moment, we think this is
  // a bad case. should be ignored
  NSSet<LynxUIExposureDetail *> *tmpArray = [NSSet setWithSet:appearUIDetail];
  if (_disappearSet != nil) {
    [tmpArray enumerateObjectsUsingBlock:^(LynxUIExposureDetail *detail, BOOL *stop) {
      if ([_disappearSet containsObject:detail]) {
        [_disappearSet removeObject:detail];
        [appearUIDetail removeObject:detail];
      }
    }];
  }

  // step 4.2 This UI exposured at previous moment,and disexposures at this moment, we think this is
  // a bad case. should be ignored
  tmpArray = [NSMutableSet setWithSet:disappearUIDetail];
  if (_appearSet != nil) {
    [tmpArray enumerateObjectsUsingBlock:^(LynxUIExposureDetail *detail, BOOL *stop) {
      if ([_appearSet containsObject:detail]) {
        [_appearSet removeObject:detail];
        [disappearUIDetail removeObject:detail];
      }
    }];
  }

  // Put the detection and sending of the exposure event in the same runloop.
  _disappearSet = disappearUIDetail;
  _appearSet = appearUIDetail;

  [self sendEvent:_disappearSet eventName:@"disexposure"];
  [self sendEvent:_appearSet eventName:@"exposure"];

  [_disappearSet removeAllObjects];
  [_appearSet removeAllObjects];

  [LynxTraceEvent endSection:LYNX_TRACE_CATEGORY_WRAPPER];
}

- (void)stopExposure:(NSDictionary *)options {
  _isStopExposure = YES;
  [self removeFromRunLoop];
  // Use the sendEvent field in options to control whether to send disexposure events.
  if ([[options valueForKey:@"sendEvent"] boolValue]) {
    [self sendEvent:_uiInWindowMapBefore eventName:@"disexposure"];
    [_uiInWindowMapBefore removeAllObjects];
  }
}

- (void)resumeExposure {
  _isStopExposure = NO;
  [self addExposureToRunLoop];
}

- (void)sendEvent:(NSMutableSet<LynxUIExposureDetail *> *)uiSet eventName:(NSString *)eventName {
  if ([uiSet count] != 0) {
    NSMutableArray<NSDictionary<NSString *, NSString *> *> *params = [[NSMutableArray alloc] init];
    NSMutableDictionary<NSString *,
                        NSMutableDictionary<NSString *, NSMutableArray<LynxUIExposureDetail *> *> *>
        *customParamMap = [[NSMutableDictionary alloc] init];
    BOOL enableGlobalEventSending = !_rootUI.lynxView ||
                                    [_rootUI.lynxView.templateRender enableJSRuntime] ||
                                    [_rootUI.lynxView.templateRender enableAirStrictMode];
    if (enableGlobalEventSending) {
      for (LynxUIExposureDetail *detail in uiSet) {
        if (detail.internalSignature && [detail.ui respondsToSelector:@selector(targetOffScreen)]) {
          [detail.ui targetOffScreen];
        }

        if ([[detail.useOptions valueForKey:@"sendCustom"] boolValue]) {
          if ([[detail.useOptions valueForKey:@"specifyTarget"] boolValue]) {
            [self addDetailToCustomParamMap:customParamMap detail:detail];
            continue;
          }
          LynxUI *ui = detail.ui;
          if (ui != nil) {
            NSString *transEventName = @"";
            if ([eventName isEqualToString:@"exposure"]) {
              transEventName = @"uiappear";
            } else {
              transEventName = @"uidisappear";
            }
            LynxDetailEvent *event = [[LynxDetailEvent alloc] initWithName:transEventName
                                                                targetSign:ui.sign
                                                                    params:[detail toDictionary]];
            [ui.context.eventEmitter sendCustomEvent:event];
          }
          continue;
        }

        if (detail.exposureID) {
          [params addObject:[self createParam:detail]];
        }
      }

      [self sendCustomParamMapEvent:customParamMap eventName:eventName];

      if (params.count > 0) {
        [_rootUI.rootView sendGlobalEvent:eventName withParams:@[ params ]];
      }
    } else {
      // Just use for Air 1.0, forward compatible is required
      for (LynxUIExposureDetail *detail in uiSet) {
        LynxUI *ui = [_rootUI.context.uiOwner findUIBySign:detail.sign];
        if (ui != nil) {
          LynxDetailEvent *event = [[LynxDetailEvent alloc] initWithName:eventName
                                                              targetSign:ui.sign
                                                                  params:[detail toDictionary]];
          [ui.context.eventEmitter sendCustomEvent:event];
        }
      }
    }
  }
}

- (void)
    sendCustomParamMapEvent:
        (NSMutableDictionary<
            NSString *, NSMutableDictionary<NSString *, NSMutableArray<LynxUIExposureDetail *> *> *>
             *)customParamMap
                  eventName:(NSString *)eventName {
  if ([customParamMap count] == 0) {
    return;
  }
  [customParamMap
      enumerateKeysAndObjectsUsingBlock:^(NSString *key, NSDictionary *map, BOOL *stop) {
        LynxUI *receiveTarget = [self.rootUI.context.uiOwner findUIBySign:[key intValue]];
        [map enumerateKeysAndObjectsUsingBlock:^(NSString *name, NSArray *array, BOOL *stop) {
          __block NSMutableArray *childrenInfo = [[NSMutableArray alloc] init];
          [array enumerateObjectsUsingBlock:^(LynxUIExposureDetail *item, NSUInteger index,
                                              BOOL *stop) {
            [childrenInfo addObject:[self createChildUIResult:item]];
          }];
          NSMutableDictionary *eventDetail = [[NSMutableDictionary alloc] init];
          [eventDetail setValue:@([eventName isEqualToString:@"exposure"]) forKey:@"isExposure"];
          [eventDetail setValue:childrenInfo forKey:@"childrenInfo"];
          LynxDetailEvent *event = [[LynxDetailEvent alloc] initWithName:name
                                                              targetSign:receiveTarget.sign
                                                                  params:eventDetail];
          [receiveTarget.context.eventEmitter sendCustomEvent:event];
        }];
      }];
}

- (void)
    addDetailToCustomParamMap:
        (NSMutableDictionary<
            NSString *, NSMutableDictionary<NSString *, NSMutableArray<LynxUIExposureDetail *> *> *>
             *)customParamMap
                       detail:(LynxUIExposureDetail *)detail {
  LynxUI *receiveTarget = detail.ui;
  if (receiveTarget == nil) {
    return;
  }
  receiveTarget = [receiveTarget getExposeReceiveTarget];
  NSString *bindEventName = [[detail.useOptions valueForKey:@"bindEventName"] stringValue];
  if (receiveTarget != nil && receiveTarget.eventSet != nil && bindEventName != nil &&
      [receiveTarget.eventSet objectForKey:bindEventName]) {
    NSString *key = [@(receiveTarget.sign) stringValue];
    if ([customParamMap objectForKey:key]) {
      NSMutableDictionary *map = [customParamMap objectForKey:key];
      if ([map objectForKey:bindEventName]) {
        NSMutableArray *array = [map objectForKey:bindEventName];
        [array addObject:detail];
      } else {
        NSMutableArray *array = [[NSMutableArray alloc] init];
        [array addObject:detail];
        [map setValue:array forKey:bindEventName];
      }
    } else {
      NSMutableArray *array = [[NSMutableArray alloc] init];
      [array addObject:detail];
      NSMutableDictionary *map = [[NSMutableDictionary alloc] init];
      [map setValue:array forKey:bindEventName];
      [customParamMap setValue:map forKey:key];
    }
  }
}

- (NSDictionary *)createChildUIResult:(LynxUIExposureDetail *)detail {
  return @{@"extra-data" : detail.extraData};
}

- (NSDictionary<NSString *, NSString *> *)createParam:(LynxUIExposureDetail *)detail {
  NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];
  [dict setValue:detail.exposureID forKey:@"exposure-id"];
  [dict setValue:detail.exposureID forKey:@"exposureID"];
  [dict setValue:detail.exposureScene forKey:@"exposure-scene"];
  [dict setValue:detail.exposureScene forKey:@"exposureScene"];
  [dict setValue:detail.internalSignature forKey:@"internal-signature"];
  [dict setValue:[NSNumber numberWithInteger:detail.sign] forKey:@"sign"];
  [dict setValue:detail.dataSet forKey:@"dataSet"];
  [dict setValue:detail.dataSet forKey:@"dataset"];
  [dict setValue:detail.uniqueID forKey:@"unique-id"];
  [dict setValue:detail.extraData forKey:@"extra-data"];

  return dict;
}

- (void)willMoveToWindow:(BOOL)windowIsNil {
  if (windowIsNil) {
    [self removeFromRunLoop];
  }
}

// This interface be used when didMoveToWindow occured.
- (void)didMoveToWindow:(BOOL)windowIsNil {
  // This page be covered
  if (!windowIsNil) {
    // This page is discover
    if ([_exposedLynxUIMap count] != 0) {
      [self addExposureToRunLoop];
    }
  }
}

- (void)addExposureToRunLoop {
  // After calling stopExposure, the exposure detection task should not be started in
  // didMoveToWindow, but will only be started in resumeExposure.
  if (!_isStopExposure && _displayLink == nil) {
    _displayLink = [CADisplayLink displayLinkWithTarget:[LynxWeakProxy proxyWithTarget:self]
                                               selector:@selector(exposureHandler:)];
    if (@available(iOS 10.0, *)) {
      _displayLink.preferredFramesPerSecond = _frameRate;  // ms = 1000 / 20 = 50ms
    } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      _displayLink.frameInterval = 60 / _frameRate;  // ms =(1000/60) * 3 = 50ms
#pragma clang diagnostic pop
    }
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  }
}

- (BOOL)addLynxUI:(LynxUI *)ui
    withUniqueIdentifier:(NSString *)uniqueID
               extraData:(NSDictionary *)data
              useOptions:(NSDictionary *)options {
  if (uniqueID || ui.exposureID || ui.internalSignature) {
    NSString *key;
    if (uniqueID) {
      key = [NSString stringWithFormat:@"%@_%@_%@", uniqueID, ui.exposureScene, ui.exposureID];
    } else {
      key = [NSString stringWithFormat:@"%@_%@_%ld_%@", ui.exposureScene, ui.exposureID,
                                       (long)ui.sign, ui.internalSignature];
    }
    [_exposedLynxUIMap setObject:[[LynxUIExposureDetail alloc] initWithUI:ui
                                                         uniqueIdentifier:uniqueID
                                                                extraData:data
                                                               useOptions:options]
                          forKey:key];
    if ([_exposedLynxUIMap count] == 1) {
      [self addExposureToRunLoop];
    }
    return YES;
  }

  return NO;
}

- (void)removeLynxUI:(LynxUI *)ui withUniqueIdentifier:(NSString *)uniqueID {
  if (uniqueID || ui.exposureID || ui.internalSignature) {
    NSString *key;
    if (uniqueID) {
      key = [NSString stringWithFormat:@"%@_%@_%@", uniqueID, ui.exposureScene, ui.exposureID];
    } else {
      key = [NSString stringWithFormat:@"%@_%@_%ld_%@", ui.exposureScene, ui.exposureID,
                                       (long)ui.sign, ui.internalSignature];
    }
    LynxUIExposureDetail *detail = [_exposedLynxUIMap objectForKey:key];
    if (detail) {
      [_exposedLynxUIMap removeObjectForKey:key];
    }
    // if have no exposuredUI, you should free the system resource
    if ([_exposedLynxUIMap count] == 0) {
      [self destroyExposure];
    }
  }
}

- (void)destroyExposure {
  [self removeFromRunLoop];
  [_exposedLynxUIMap removeAllObjects];

  // Before reloading the page, we need to disexposure the previous page.
  [self sendEvent:_uiInWindowMapBefore eventName:@"disexposure"];
  [_uiInWindowMapBefore removeAllObjects];
}

- (void)removeFromRunLoop {
  if (_displayLink) {
    [_displayLink invalidate];
    _displayLink = nil;
  }
}

@end
