// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxUIOwner.h>

@class LynxUI;
@class LynxUIContext;

NS_ASSUME_NONNULL_BEGIN
@interface LynxEventEmitterUnitTestHelper : LynxEventEmitter
// Cache the most current event it received.
@property(nonatomic, nullable, readonly) LynxCustomEvent *event;
@property(nonatomic, nullable) NSMutableArray<LynxCustomEvent *> *cachedEvents;

- (void)clearEvent;
@end

@interface LynxUIMockContext : NSObject
@property(nonatomic) LynxUIOwner *UIOwner;
@property(nonatomic) LynxUI *mockUI;
@property(nonatomic) LynxUIContext *mockUIContext;
@property(nonatomic) LynxEventEmitterUnitTestHelper *mockEventEmitter;
@property(nonatomic) LynxView *rootView;
@end

@interface LynxUIUnitTestUtils : NSObject
+ (LynxUIMockContext *)initUIMockContextWithUI:(LynxUI *)ui;
+ (LynxUIMockContext *)updateUIMockContext:(nullable LynxUIMockContext *)mockContext
                                      sign:(NSInteger)sign
                                       tag:(NSString *)tagName
                                  eventSet:(NSSet *)eventSet
                             lepusEventSet:(NSSet *)lepusEventSet
                                     props:(nonnull NSDictionary *)props;
@end
NS_ASSUME_NONNULL_END
