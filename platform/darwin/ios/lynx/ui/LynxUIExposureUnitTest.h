// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIExposure+Internal.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

@interface LynxUI ()
@property(nonatomic, weak) LynxUIContext *context;

- (BOOL)isVisible;
@end

@interface LynxUIExposureDetail : NSObject
@property(nonatomic, weak) LynxUI *ui;
@end

@interface LynxUIExposure ()

@property(nonatomic) BOOL isStopExposure;
@property(nonatomic, weak) LynxRootUI *rootUI;
@property(nonatomic) CADisplayLink *displayLink;
@property(nonatomic) NSMutableSet<LynxUIExposureDetail *> *uiInWindowMapBefore;
@property(nonatomic) NSMutableDictionary<NSString *, LynxUIExposureDetail *> *exposedLynxUIMap;

- (void)exposureHandler:(CADisplayLink *)sender;
- (void)sendEvent:(NSMutableSet<LynxUIExposureDetail *> *)uiSet eventName:(NSString *)eventName;
- (void)addExposureToRunLoop;
- (void)removeFromRunLoop;
- (CGRect)borderOfExposureScreen:(LynxUI *)ui;

@end

@interface LynxUIExposureUnitTest : XCTestCase
@end
