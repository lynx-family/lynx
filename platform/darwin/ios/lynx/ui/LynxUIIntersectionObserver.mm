// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIIntersectionObserver.h"
#import "LynxUIIntersectionObserver+Internal.h"

#import <objc/runtime.h>
#import "LynxContext+Internal.h"
#import "LynxRootUI.h"
#import "LynxUI+Internal.h"
#import "LynxUIOwner.h"
#import "LynxUnitUtils.h"
#import "LynxWeakProxy.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/value_wrapper/darwin/value_impl_darwin.h"

using namespace lynx;

@interface LynxUIIntersectionObserverManager ()

@property(nonatomic, weak) LynxContext* context;
@property(nonatomic) CADisplayLink* displayLink;

@end

@implementation IntersectionObserverEntry

- (void)update {
  float targetArea = _boundingClientRect.size.width * _boundingClientRect.size.height;
  float intersectionArea = _intersectionRect.size.width * _intersectionRect.size.height;

  if (targetArea > 0) {
    _intersectionRatio = intersectionArea / targetArea;
  } else {
    _intersectionRatio = 0;
  }
}

- (NSDictionary*)rectToDictionary:(CGRect)rect {
  return @{
    @"left" : @(int(rect.origin.x)),
    @"right" : @(int(rect.origin.x + rect.size.width)),
    @"top" : @(int(rect.origin.y)),
    @"bottom" : @(int(rect.origin.y + rect.size.height))
  };
}

- (NSDictionary*)toDictionary {
  return @{
    @"boundingClientRect" : [self rectToDictionary:_boundingClientRect],
    @"intersectionRect" : [self rectToDictionary:_intersectionRect],
    @"relativeRect" : [self rectToDictionary:_relativeRect],
    @"intersectionRatio" : @(_intersectionRatio),
    @"isIntersecting" : @(_isIntersecting),
    @"time" : @(_time),
    @"relativeToId" : _relativeToId,
    @"observerId" : _relativeToId
  };
}

@end

@implementation LynxUIObservationTarget
@end

@interface LynxUIIntersectionObserver ()

// set from js
@property(nonatomic, assign) NSInteger observerId;

// relativeTo(root, margins) or relativeViewport(margins), default is zero
@property(nonatomic, assign) float marginLeft;
@property(nonatomic, assign) float marginRight;
@property(nonatomic, assign) float marginTop;
@property(nonatomic, assign) float marginBottom;

// default is [0]
@property(nonatomic, strong) NSArray* thresholds;

// default is 0
@property(nonatomic, assign) NSInteger initialRatio;

// default is false
@property(nonatomic, assign) BOOL observeAll;  // TODO: not support now

@property(nonatomic, assign) BOOL isCustomEventObserver;

@property(nonatomic, weak) LynxUIIntersectionObserverManager* manager;

@property(nonatomic, weak) LynxUI* container;

// root will be set using relativeTo,  root will be nil if using relativeViewport
@property(nonatomic, weak) LynxUI* root;

@property(nonatomic) BOOL relativeToScreen;

// key is target id in js, value is LynxUI
@property(nonatomic, strong) NSMutableArray<LynxUIObservationTarget*>* observationTargets;

@property(nonatomic) BOOL enableNewIntersectionObserver;

@end

@implementation LynxUIIntersectionObserver

- (instancetype)initWithManager:(LynxUIIntersectionObserverManager*)manager
                     observerId:(NSInteger)observerId
                    componentId:(NSString*)componentId
                        options:(NSDictionary*)options {
  self = [super init];
  if (self) {
    _manager = manager;
    _observerId = observerId;
    _thresholds = [NSArray arrayWithArray:options[@"thresholds"] ?: @[ @(0) ]];
    _enableNewIntersectionObserver = manager.enableNewIntersectionObserver;

    NSNumber* initialRatio = [options objectForKey:@"initialRatio"];
    _initialRatio = initialRatio ? initialRatio.intValue : 0;

    NSNumber* observeAll = [options objectForKey:@"observeAll"];
    _observeAll = observeAll ? observeAll.boolValue : NO;
    _relativeToScreen = NO;

    if ([componentId isEqualToString:kDefaultComponentID]) {
      _container = (LynxUI*)_manager.uiOwner.rootUI;
    } else {
      _container = [_manager.uiOwner findUIByComponentId:componentId];
    }

    _observationTargets = [[NSMutableArray alloc] init];

    _isCustomEventObserver = NO;
  }

  return self;
}

- (instancetype)initWithOptions:(NSDictionary*)options
                        manager:(LynxUIIntersectionObserverManager*)manager
                     attachedUI:(LynxUI*)attachedUI {
  self = [self initWithManager:manager
                    observerId:-1
                   componentId:kDefaultComponentID
                       options:options];
  if (self) {
    _manager = manager;
    _enableNewIntersectionObserver = manager.enableNewIntersectionObserver;

    NSString* relativeToIdSelector = options[@"relativeToIdSelector"];
    _relativeToScreen = [options[@"relativeToScreen"] boolValue];
    _marginLeft = [LynxUnitUtils toPtFromUnitValue:[(options[@"marginLeft"] ?: @"0") description]];
    _marginRight =
        [LynxUnitUtils toPtFromUnitValue:[(options[@"marginRight"] ?: @"0") description]];
    _marginTop = [LynxUnitUtils toPtFromUnitValue:[(options[@"marginTop"] ?: @"0") description]];
    _marginBottom =
        [LynxUnitUtils toPtFromUnitValue:[(options[@"marginBottom"] ?: @"0") description]];
    _attachedUI = attachedUI;
    _isCustomEventObserver = YES;
    if (!relativeToIdSelector && [relativeToIdSelector hasPrefix:@"#"]) {
      _root =
          [_manager.uiOwner findUIByIdSelectorInParent:[relativeToIdSelector substringFromIndex:1]
                                                 child:attachedUI];
    }

    LynxUIObservationTarget* observationTarget = [[LynxUIObservationTarget alloc] init];
    observationTarget.ui = attachedUI;
    [_observationTargets addObject:observationTarget];

    // check root first
    CGRect rootRect = [self getRootRect];
    [self checkForIntersectionWithTarget:observationTarget rootRect:rootRect isInitial:YES];
  }

  return self;
}

// only support id selector
- (void)relativeTo:(NSString*)selector margins:(NSDictionary*)margins {
  if (![selector hasPrefix:@"#"]) return;
  _root = [_manager.uiOwner findUIByIdSelector:[selector substringFromIndex:1] withinUI:_container];
  // for historical reason, to avoid break, if not found in container, finding in all element
  if (_root == nil) {
    _root = [_manager.uiOwner uiWithIdSelector:[selector substringFromIndex:1]];
  }

  [self parseMargin:margins];
}

- (void)relativeToViewportWithMargins:(NSDictionary*)margins {
  _root = nil;
  [self parseMargin:margins];
}

- (void)relativeToScreenWithMargins:(NSDictionary*)margins {
  _root = nil;
  _relativeToScreen = YES;
  [self parseMargin:margins];
}

// only support id selector
- (void)observe:(NSString*)targetSelector callbackId:(NSInteger)callbackId {
  if (![targetSelector hasPrefix:@"#"]) return;
  LynxUI* target = [_manager.uiOwner findUIByIdSelector:[targetSelector substringFromIndex:1]
                                               withinUI:_container];
  // for historical reason, to avoid break, if not found in container, finding in all element
  if (target == nil) {
    target = [_manager.uiOwner uiWithIdSelector:[targetSelector substringFromIndex:1]];
  }

  if (target) {
    for (size_t i = 0; i < _observationTargets.count; i++) {
      if (_observationTargets[i].ui == target) {
        return;
      }
    }
    LynxUIObservationTarget* observationTarget = [[LynxUIObservationTarget alloc] init];
    observationTarget.jsCallbackId = callbackId;
    observationTarget.ui = target;
    [_observationTargets addObject:observationTarget];

    // check root first
    CGRect rootRect = [self getRootRect];
    [self checkForIntersectionWithTarget:observationTarget rootRect:rootRect isInitial:YES];
  }
}

- (void)disconnect {
  [_observationTargets removeAllObjects];
  [_manager removeIntersectionObserver:_observerId];
}

- (void)parseMargin:(NSDictionary*)margins {
  NSString* value = [margins objectForKey:@"left"];
  _marginLeft = value ? [LynxUnitUtils toPtFromUnitValue:value.description] : 0;
  value = [margins objectForKey:@"right"];
  _marginRight = value ? [LynxUnitUtils toPtFromUnitValue:value.description] : 0;
  value = [margins objectForKey:@"top"];
  _marginTop = value ? [LynxUnitUtils toPtFromUnitValue:value.description] : 0;
  value = [margins objectForKey:@"bottom"];
  _marginBottom = value ? [LynxUnitUtils toPtFromUnitValue:value.description] : 0;
}

- (void)checkForIntersections {
  if (_observationTargets.count == 0) return;
  CGRect rootRect = [self getRootRect];

  __weak __typeof(self) weakSelf = self;

  [_observationTargets
      enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        LynxUIObservationTarget* target = (LynxUIObservationTarget*)obj;
        [weakSelf checkForIntersectionWithTarget:target rootRect:rootRect isInitial:NO];
      }];
}

- (void)checkForIntersectionWithTarget:(LynxUIObservationTarget*)target
                              rootRect:(CGRect)rootRect
                             isInitial:(BOOL)isInitial {
  CGRect targetRect = [target.ui getBoundingClientRect];

  CGRect intersectionRect = [self computeTargetAndRootIntersection:target.ui
                                                        targetRect:targetRect
                                                          rootRect:rootRect];

  IntersectionObserverEntry* entry = [[IntersectionObserverEntry alloc] init];
  if (!CGRectEqualToRect(intersectionRect, CGRectZero)) {
    entry.isIntersecting = YES;
  }
  entry.time = 0;  // TODO: time is not used currently
  entry.intersectionRect = intersectionRect;
  entry.relativeRect = rootRect;
  entry.boundingClientRect = targetRect;
  entry.relativeToId = target.ui && target.ui.idSelector ? target.ui.idSelector : @"";
  [entry update];

  IntersectionObserverEntry* oldEntry = target.entry;
  target.entry = entry;

  BOOL needNotifyJS = isInitial ? _initialRatio < entry.intersectionRatio
                                : [self hasCrossedThreshold:oldEntry newEntry:entry];

  if (needNotifyJS) {
    if (_isCustomEventObserver) {
      LynxCustomEvent* event = [[LynxCustomEvent alloc] initWithName:@"intersection"
                                                          targetSign:target.ui.sign
                                                              params:[entry toDictionary]];
      [target.ui.context.eventEmitter sendCustomEvent:event];
    } else {
      // notify js the intersection rect
      if (_manager.context) {
        _manager.context->proxy_->CallJSIntersectionObserver(
            (int32_t)_observerId, (int32_t)target.jsCallbackId,
            std::make_unique<lynx::pub::ValueImplDarwin>([entry toDictionary]));
      }
    }
  }
}

- (CGRect)computeTargetAndRootIntersection:(LynxUI*)target
                                targetRect:(CGRect)targetRect
                                  rootRect:(CGRect)rootRect {
  if (![target isVisible]) {
    return CGRectZero;
  }

  CGRect intersectionRect = targetRect;
  LynxUI* parent = target.parent;
  BOOL atRoot = NO;

  LynxUI* root = _root ?: (LynxUI*)_manager.uiOwner.rootUI;

  while (!atRoot && parent) {
    CGRect parentRect = CGRectZero;
    if (![parent isVisible]) {
      return CGRectZero;
    }

    if (parent == root) {
      atRoot = YES;
      if (_relativeToScreen) {
        parentRect = [parent getBoundingClientRect];
      } else {
        parentRect = rootRect;
      }
    } else {
      if (parent.overflow == 0) {
        parentRect = [parent getBoundingClientRect];
      }
    }

    if (!CGRectEqualToRect(parentRect, CGRectZero)) {
      if (CGRectIntersectsRect(parentRect, intersectionRect)) {
        intersectionRect = CGRectIntersection(parentRect, intersectionRect);
      } else {
        intersectionRect = CGRectZero;
      }
    }

    if (CGRectEqualToRect(intersectionRect, CGRectZero)) {
      break;
    }

    LynxUI* uiParent = parent.parent;
    // to keep the old logic, set a switch here
    if (_enableNewIntersectionObserver) {
      while (uiParent.view != nil && ![parent.view isDescendantOfView:uiParent.view]) {
        uiParent = uiParent.parent;
      }
    }
    parent = uiParent;
  }

  // in loop, screen's rect is not interacted
  if (_relativeToScreen) {
    // from Lynx coordinate to screen coordinate
    targetRect = [_manager.uiOwner.rootUI.view convertRect:targetRect toView:nil];
    intersectionRect = [_manager.uiOwner.rootUI.view convertRect:intersectionRect toView:nil];
    intersectionRect = CGRectIntersection(intersectionRect, rootRect);
  }

  return intersectionRect;
}

- (CGRect)getRootRect {
  CGRect rootRect = CGRectZero;
  if (_root) {
    // relative to ui
    rootRect = [_root getBoundingClientRect];
  } else if (_relativeToScreen) {
    // relative to screen
    rootRect = [UIScreen mainScreen].bounds;
  } else {
    // relative to LynxView
    CGRect rootFrame = _manager.uiOwner.rootUI.view.frame;
    rootRect = CGRectMake(0, 0, rootFrame.size.width, rootFrame.size.height);
  }

  // extend margin
  CGRect newRect = CGRectMake(rootRect.origin.x - _marginLeft, rootRect.origin.y - _marginTop,
                              rootRect.size.width + (_marginLeft + _marginRight),
                              rootRect.size.height + (_marginTop + _marginBottom));

  return newRect;
}

- (BOOL)hasCrossedThreshold:(IntersectionObserverEntry*)oldEntry
                   newEntry:(IntersectionObserverEntry*)newEntry {
  // To make comparing easier, an entry that has a ratio of 0
  // but does not actually intersect is given a value of -1
  float oldRatio = oldEntry && !CGRectEqualToRect(oldEntry.intersectionRect, CGRectZero)
                       ? oldEntry.intersectionRatio
                       : -1;
  float newRatio =
      !CGRectEqualToRect(newEntry.intersectionRect, CGRectZero) ? newEntry.intersectionRatio : -1;

  if (oldRatio == newRatio) return NO;

  for (size_t i = 0; i < _thresholds.count; i++) {
    float threshold = [_thresholds[i] floatValue];
    // Return true if an entry matches a threshold or if the new ratio
    // and the old ratio are on the opposite sides of a threshold.
    if (threshold == oldRatio || threshold == newRatio ||
        (threshold < oldRatio != threshold < newRatio)) {
      return YES;
    }
  }

  return NO;
}

@end

@implementation LynxUIIntersectionObserverManager {
  NSMutableArray<LynxUIIntersectionObserver*>* observers_;
}

- (instancetype)initWithLynxContext:(LynxContext*)context {
  self = [super init];
  if (self) {
    observers_ = [[NSMutableArray alloc] init];
    _context = context;
  }

  return self;
}

- (void)setEnableNewIntersectionObserver:(BOOL)enable {
  _enableNewIntersectionObserver = enable;
}

- (void)addIntersectionObserver:(LynxUIIntersectionObserver*)observer {
  if ([observers_ count] == 0 && _enableNewIntersectionObserver) {
    [self addIntersectionObserverToRunLoop];
  }
  if ([observers_ containsObject:observer]) return;
  [observers_ addObject:observer];
}

- (void)removeIntersectionObserver:(NSInteger)observerId {
  for (size_t i = 0; i < observers_.count; i++) {
    if (observers_[i].observerId == observerId) {
      [observers_ removeObjectAtIndex:i];
      if (observers_.count == 0 && _enableNewIntersectionObserver) {
        [self destroyIntersectionObserver];
      }
      return;
    }
  }
}

- (void)reset {
  [observers_ removeAllObjects];
}

- (void)removeAttachedIntersectionObserver:(LynxUI*)attachedUI {
  for (size_t i = 0; i < observers_.count; i++) {
    if (observers_[i].attachedUI == attachedUI) {
      [observers_ removeObjectAtIndex:i];
      if (observers_.count == 0 && _enableNewIntersectionObserver) {
        [self destroyIntersectionObserver];
      }
      return;
    }
  }
}

- (LynxUIIntersectionObserver*)getObserverById:(NSInteger)observerId {
  for (size_t i = 0; i < observers_.count; i++) {
    if (observers_[i].observerId == observerId) {
      return observers_[i];
    }
  }
  return nil;
}

- (void)onLynxEvent:(LynxInnerEventType)type event:(LynxEvent*)event {
  if (observers_.count == 0) return;

  BOOL shouldHandle = NO;
  if (type == LynxEventTypeLayoutEvent) {
    shouldHandle = YES;
  } else if (type == LynxEventTypeCustomEvent) {
    LynxCustomEvent* customEvent = (LynxCustomEvent*)event;
    NSArray* scrollEvents = @[ @"scroll", @"scrolltoupper", @"scrolltolower" ];
    if ([scrollEvents containsObject:customEvent.eventName]) {
      shouldHandle = YES;
    }
  }

  if (!shouldHandle) return;
  [self notifyObservers];
}

- (void)notifyObservers {
  [observers_ enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
    [((LynxUIIntersectionObserver*)obj) checkForIntersections];
  }];
}

// below functions are for new intersection observer

// This interface be used when didMoveToWindow occured.
- (void)didMoveToWindow:(BOOL)windowIsNil {  // be controlled
  // This page be covered
  if (windowIsNil) {
    [self removeFromRunLoop];
  } else {
    // This page is discover
    if ([observers_ count] != 0) {
      [self addIntersectionObserverToRunLoop];
    }
  }
}

- (void)addIntersectionObserverToRunLoop {  // be controlled
  if (_displayLink == nil) {
    _displayLink = [CADisplayLink displayLinkWithTarget:[LynxWeakProxy proxyWithTarget:self]
                                               selector:@selector(intersectionObserverHandler:)];
    if (@available(iOS 10.0, *)) {
      _displayLink.preferredFramesPerSecond = 20;  // ms = 1000 / 20 = 50ms
    } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      _displayLink.frameInterval = 3;  // ms =(1000/60) * 3 = 50ms
#pragma clang diagnostic pop
    }
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  }
}

- (void)destroyIntersectionObserver {  // be controlled
  [self removeFromRunLoop];
  [observers_ removeAllObjects];
}

- (void)removeFromRunLoop {  // be controlled
  if (_displayLink) {
    [_displayLink invalidate];
    _displayLink = nil;
  }
}

- (void)intersectionObserverHandler:(CADisplayLink*)sender {  // be controlled
  [self notifyObservers];
}

@end
