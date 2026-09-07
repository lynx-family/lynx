// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxModule.h>

NS_ASSUME_NONNULL_BEGIN

/// Explorer-local implementation of the platform test card's pure JSB contract.
///
/// Keep the Objective-C class name Explorer-specific so embedding Explorer next
/// to another test host cannot introduce duplicate Objective-C classes. The
/// Lynx module name intentionally remains `JSBTestModule` for card compatibility.
@interface ExplorerJSBTestModule : NSObject <LynxModule>

- (NSString *)name;
- (int)getByte:(int)param;
- (short int)getShort:(short int)param;
- (BOOL)getBoolean:(BOOL)param;
- (NSString *)getChar:(NSString *)param;
- (double)getDouble:(double)param;
- (NSData *)getArrayBuffer:(NSData *)buffer;
- (NSString *)getString:(NSString *)string;
- (long)getBigInt:(long)param;
- (NSDictionary *)getMap:(NSDictionary *)param;
- (NSArray *)getArray:(NSArray *)param;
- (void)testAsyncCallBack:(NSArray *)array callback:(LynxCallbackBlock)callback;
- (void)testSyncCallBack:(NSArray *)array callback:(LynxCallbackBlock)callback;
- (void)testAsyncMultiCallBack:(NSArray *)array
                      callback:(LynxCallbackBlock)firstCallback
                      callback:(LynxCallbackBlock)secondCallback;
- (void)testSyncMultiCallBack:(NSArray *)array
                     callback:(LynxCallbackBlock)firstCallback
                     callback:(LynxCallbackBlock)secondCallback;
- (void)testPromise:(BOOL)ok
            resolve:(LynxPromiseResolveBlock)resolve
             reject:(LynxPromiseRejectBlock)reject;

@end

/// Explorer-local implementation of the platform test `bridge` timing contract.
@interface ExplorerJSBTimingTestModule : NSObject <LynxModule>

- (void)call:(NSString *)name params:(NSDictionary *)params callback:(LynxCallbackBlock)callback;

@end

NS_ASSUME_NONNULL_END
