// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxEventEmitter.h"

@class LynxUI;
@class LynxUIOwner;
@class LynxContext;
@class LynxUIIntersectionObserverManager;

@interface IntersectionObserverEntry : NSObject

@property(nonatomic, assign) CGRect relativeRect;
@property(nonatomic, assign) CGRect boundingClientRect;
@property(nonatomic, assign) CGRect intersectionRect;
@property(nonatomic, assign) float intersectionRatio;
@property(nonatomic, assign) BOOL isIntersecting;
@property(nonatomic, assign) double time;
@property(nonatomic, assign) NSString* relativeToId;

- (void)update;

- (NSDictionary*)toDictionary;

@end

@interface LynxUIObservationTarget : NSObject

@property(nonatomic, weak) LynxUI* ui;
@property(nonatomic, assign) NSInteger jsCallbackId;
@property(nonatomic, strong) IntersectionObserverEntry* entry;

@end

@interface LynxUIIntersectionObserver : NSObject

@property(nonatomic, weak, readonly) LynxUI* attachedUI;

- (instancetype)initWithManager:(LynxUIIntersectionObserverManager*)manager
                     observerId:(NSInteger)observerId
                    componentId:(NSString*)componentId
                        options:(NSDictionary*)options;

- (instancetype)initWithOptions:(NSDictionary*)options
                        manager:(LynxUIIntersectionObserverManager*)manager
                     attachedUI:(LynxUI*)attachedUI;

// only support id selector
- (void)relativeTo:(NSString*)selector margins:(NSDictionary*)margins;
- (void)relativeToViewportWithMargins:(NSDictionary*)margins;
- (void)relativeToScreenWithMargins:(NSDictionary*)margins;

// only support id selector
- (void)observe:(NSString*)targetSelector callbackId:(NSInteger)callbackId;
- (void)disconnect;

- (void)checkForIntersections;

@end

@interface LynxUIIntersectionObserverManager : NSObject <LynxEventObserver>

@property(nonatomic, weak) LynxUIOwner* uiOwner;
@property(nonatomic, readonly) BOOL enableNewIntersectionObserver;

- (instancetype)initWithLynxContext:(LynxContext*)context;

- (void)addIntersectionObserver:(LynxUIIntersectionObserver*)observer;
- (void)removeIntersectionObserver:(NSInteger)observerId;
- (void)removeAttachedIntersectionObserver:(LynxUI*)attachedUI;
- (LynxUIIntersectionObserver*)getObserverById:(NSInteger)observerId;
- (void)notifyObservers;

- (void)didMoveToWindow:(BOOL)windowIsNil;
- (void)addIntersectionObserverToRunLoop;
- (void)destroyIntersectionObserver;

- (void)reset;

@end
