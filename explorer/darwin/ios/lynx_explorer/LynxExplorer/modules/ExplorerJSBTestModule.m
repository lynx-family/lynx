// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ExplorerJSBTestModule.h"

@implementation ExplorerJSBTestModule

+ (NSString *)name {
  return @"JSBTestModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"name" : NSStringFromSelector(@selector(name)),
    @"getByte" : NSStringFromSelector(@selector(getByte:)),
    @"getShort" : NSStringFromSelector(@selector(getShort:)),
    @"getBoolean" : NSStringFromSelector(@selector(getBoolean:)),
    @"getChar" : NSStringFromSelector(@selector(getChar:)),
    @"getDouble" : NSStringFromSelector(@selector(getDouble:)),
    @"getArrayBuffer" : NSStringFromSelector(@selector(getArrayBuffer:)),
    @"getString" : NSStringFromSelector(@selector(getString:)),
    @"getBigInt" : NSStringFromSelector(@selector(getBigInt:)),
    @"getMap" : NSStringFromSelector(@selector(getMap:)),
    @"getArray" : NSStringFromSelector(@selector(getArray:)),
    @"testAsyncCallBack" : NSStringFromSelector(@selector(testAsyncCallBack:callback:)),
    @"testSyncCallBack" : NSStringFromSelector(@selector(testSyncCallBack:callback:)),
    @"testAsyncMultiCallBack" : NSStringFromSelector(@selector(testAsyncMultiCallBack:
                                                                             callback:callback:)),
    @"testSyncMultiCallBack" : NSStringFromSelector(@selector(testSyncMultiCallBack:
                                                                           callback:callback:)),
    @"testPromise" : NSStringFromSelector(@selector(testPromise:resolve:reject:)),
  };
}

- (NSString *)name {
  return [self.class name];
}

- (int)getByte:(int)param {
  return param;
}

- (short int)getShort:(short int)param {
  return param;
}

- (BOOL)getBoolean:(BOOL)param {
  return param;
}

- (NSString *)getChar:(NSString *)param {
  return param;
}

- (double)getDouble:(double)param {
  return param;
}

- (NSData *)getArrayBuffer:(NSData *)buffer {
  return buffer;
}

- (NSString *)getString:(NSString *)string {
  return string;
}

- (long)getBigInt:(long)param {
  return param;
}

- (NSDictionary *)getMap:(NSDictionary *)param {
  return @{@"map" : param};
}

- (NSArray *)getArray:(NSArray *)param {
  return @[ param ];
}

- (void)testAsyncCallBack:(NSArray *)array callback:(LynxCallbackBlock)callback {
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
    if (callback) {
      callback(array);
    }
  });
}

- (void)testSyncCallBack:(NSArray *)array callback:(LynxCallbackBlock)callback {
  if (callback) {
    callback(array);
  }
}

- (void)testAsyncMultiCallBack:(NSArray *)array
                      callback:(LynxCallbackBlock)firstCallback
                      callback:(LynxCallbackBlock)secondCallback {
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
    if (firstCallback) {
      firstCallback(array);
    }
    if (secondCallback) {
      secondCallback(array);
    }
  });
}

- (void)testSyncMultiCallBack:(NSArray *)array
                     callback:(LynxCallbackBlock)firstCallback
                     callback:(LynxCallbackBlock)secondCallback {
  if (firstCallback) {
    firstCallback(array);
  }
  if (secondCallback) {
    secondCallback(array);
  }
}

- (void)testPromise:(BOOL)ok
            resolve:(LynxPromiseResolveBlock)resolve
             reject:(LynxPromiseRejectBlock)reject {
  if (ok) {
    resolve(@"resolve");
  } else {
    reject(@"code", @"message");
  }
}

@end

@implementation ExplorerJSBTimingTestModule

+ (NSString *)name {
  return @"bridge";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{@"call" : NSStringFromSelector(@selector(call:params:callback:))};
}

- (void)call:(NSString *)name params:(NSDictionary *)params callback:(LynxCallbackBlock)callback {
  if ([name isEqualToString:@"timing"]) {
    callback(params);
  } else if ([name isEqualToString:@"asyncTiming"]) {
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
      callback(params);
    });
  }
}

@end
