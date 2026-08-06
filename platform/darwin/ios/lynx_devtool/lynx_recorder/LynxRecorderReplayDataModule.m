// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxRecorderReplayDataModule.h>

@interface LynxRecorderReplayDataModule ()
@property NSArray<NSDictionary *> *functionCall;
@property NSDictionary<NSString *, id> *callbackData;
@property NSArray *jsbIgnoredInfo;
@property NSDictionary *jsbSettings;
@property NSDictionary *sharedData;
@property(nonatomic, weak) LynxContext *context;
@end

static NSString *LynxRecorderJSONString(id value, NSString *fallback) {
  if (![NSJSONSerialization isValidJSONObject:value]) {
    return fallback;
  }
  NSError *error = nil;
  NSData *data = [NSJSONSerialization dataWithJSONObject:value options:0 error:&error];
  if (data.length == 0 || error != nil) {
    return fallback;
  }
  NSString *result = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
  return result ?: fallback;
}

static int64_t LynxRecorderRecordTimeInMilliseconds(NSDictionary *record) {
  id milliseconds = record[@"RecordMillisecond"];
  if ([milliseconds respondsToSelector:@selector(doubleValue)]) {
    return (int64_t)[milliseconds doubleValue];
  }
  id seconds = record[@"Record Time"];
  if ([seconds respondsToSelector:@selector(doubleValue)]) {
    return (int64_t)([seconds doubleValue] * 1000);
  }
  return 0;
}

@implementation LynxRecorderReplayDataModule

+ (NSString *)name {
  return @"LynxRecorderReplayDataModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"getData" : NSStringFromSelector(@selector(getData:)),
    @"getSharedData" : NSStringFromSelector(@selector(getSharedData:)),
    @"schedule" : NSStringFromSelector(@selector(schedule:token:)),
  };
}

- (id)initWithParam:(id)param {
  return [self initWithLynxContext:nil WithParam:param];
}

- (instancetype)initWithLynxContext:(LynxContext *)context {
  return [self initWithLynxContext:context WithParam:nil];
}

- (instancetype)initWithLynxContext:(LynxContext *)context WithParam:(id)param {
  if (self = [super init]) {
    _context = context;
    _functionCall = @[];
    _callbackData = @{};
    _jsbSettings = @{};
    _jsbIgnoredInfo = @[];
    _sharedData = @{};
    if ([param conformsToProtocol:@protocol(LynxRecorderReplayDataProvider)]) {
      id<LynxRecorderReplayDataProvider> provider = (id<LynxRecorderReplayDataProvider>)param;
      id functionCall = [provider getFunctionCall];
      id callbackData = [provider getCallbackData];
      id jsbSettings = [provider getJsbSettings];
      id jsbIgnoredInfo = [provider getJSbIgnoredInfo];
      id sharedData = [provider getSharedData];
      _functionCall = [functionCall isKindOfClass:[NSArray class]] ? functionCall : @[];
      _callbackData = [callbackData isKindOfClass:[NSDictionary class]] ? callbackData : @{};
      _jsbSettings = [jsbSettings isKindOfClass:[NSDictionary class]] ? jsbSettings : @{};
      _jsbIgnoredInfo = [jsbIgnoredInfo isKindOfClass:[NSArray class]] ? jsbIgnoredInfo : @[];
      _sharedData = [sharedData isKindOfClass:[NSDictionary class]] ? sharedData : @{};
    }
  }
  return self;
}

- (NSDictionary *)getSharedData:(NSString *)key {
  id value = key.length > 0 ? [_sharedData objectForKey:key] : nil;
  if (value == nil) {
    return @{@"value" : @""};
  }
  return @{@"value" : value};
}

- (void)schedule:(nullable NSNumber *)delay token:(nullable NSString *)token {
  if (token == nil || token.length == 0) {
    return;
  }
  NSTimeInterval delayInSeconds = delay == nil ? 0 : MAX(0, delay.doubleValue) / 1000.0;
  __weak LynxRecorderReplayDataModule *weakSelf = self;
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSeconds * NSEC_PER_SEC)),
                 dispatch_get_main_queue(), ^{
                   [weakSelf.context sendGlobalEvent:@"__lynx_recorder_replay_schedule__"
                                          withParams:@[ token ]];
                 });
}

- (void)getData:(LynxCallbackBlock)callback {
  NSDictionary *json = @{
    @"RecordData" : [self getRecordData],
    @"JsbSettings" : [self getJsbSettings],
    @"JsbIgnoredInfo" : [self getJsbIgnoredInfo],
  };
  if (callback != nil) {
    callback(LynxRecorderJSONString(json, @"{}"));
  }
}

- (NSString *)getRecordData {
  NSMutableDictionary<NSString *, NSMutableArray *> *json = [NSMutableDictionary new];
  for (id value in _functionCall) {
    if (![value isKindOfClass:[NSDictionary class]]) {
      continue;
    }
    NSDictionary *funcInvoke = value;
    NSString *moduleName = [funcInvoke[@"Module Name"] isKindOfClass:[NSString class]]
                               ? funcInvoke[@"Module Name"]
                               : nil;
    NSString *methodName = [funcInvoke[@"Method Name"] isKindOfClass:[NSString class]]
                               ? funcInvoke[@"Method Name"]
                               : nil;
    if (moduleName.length == 0 || methodName.length == 0) {
      continue;
    }
    NSMutableArray *moduleCalls = json[moduleName];
    if (moduleCalls == nil) {
      moduleCalls = [NSMutableArray new];
      json[moduleName] = moduleCalls;
    }

    NSMutableDictionary *methodLookUp = [NSMutableDictionary new];
    int64_t requestTime = LynxRecorderRecordTimeInMilliseconds(funcInvoke);
    NSDictionary *params =
        [funcInvoke[@"Params"] isKindOfClass:[NSDictionary class]] ? funcInvoke[@"Params"] : @{};
    NSArray *callbackIDs =
        [params[@"callback"] isKindOfClass:[NSArray class]] ? params[@"callback"] : @[];

    NSMutableArray *callbackReturnValues = [NSMutableArray new];
    NSString *functionInvokeLabel = @"";
    for (id callbackID in callbackIDs) {
      NSString *callbackKey = [callbackID description];
      id recordedCallback = _callbackData[callbackKey];
      NSArray *callbackCandidates =
          [recordedCallback isKindOfClass:[NSArray class]]
              ? recordedCallback
              : ([recordedCallback isKindOfClass:[NSDictionary class]] ? @[ recordedCallback ]
                                                                       : @[]);
      NSDictionary *callbackInfo = nil;
      int64_t matchedResponseTime = INT64_MAX;
      for (id candidate in callbackCandidates) {
        if (![candidate isKindOfClass:[NSDictionary class]]) {
          continue;
        }
        NSString *candidateModule = [candidate[@"Module Name"] isKindOfClass:[NSString class]]
                                        ? candidate[@"Module Name"]
                                        : nil;
        NSString *candidateMethod = [candidate[@"Method Name"] isKindOfClass:[NSString class]]
                                        ? candidate[@"Method Name"]
                                        : nil;
        BOOL hasOwnerMetadata = candidateModule.length > 0 || candidateMethod.length > 0;
        if (hasOwnerMetadata && (![candidateModule isEqualToString:moduleName] ||
                                 ![candidateMethod isEqualToString:methodName])) {
          continue;
        }
        int64_t candidateTime = LynxRecorderRecordTimeInMilliseconds(candidate);
        if (candidateTime > 0 && requestTime > 0 && candidateTime < requestTime) {
          continue;
        }
        if (callbackInfo == nil || candidateTime < matchedResponseTime) {
          callbackInfo = candidate;
          matchedResponseTime = candidateTime;
        }
      }
      if (callbackInfo != nil) {
        int64_t responseTime = LynxRecorderRecordTimeInMilliseconds(callbackInfo);
        NSDictionary *callbackParams = [callbackInfo[@"Params"] isKindOfClass:[NSDictionary class]]
                                           ? callbackInfo[@"Params"]
                                           : @{};
        NSDictionary *callbackKernel =
            @{@"Value" : callbackParams, @"Delay" : @(MAX(0, responseTime - requestTime))};
        [callbackReturnValues addObject:callbackKernel];
      }
      functionInvokeLabel = [functionInvokeLabel stringByAppendingFormat:@"%@_", callbackKey];
    }

    methodLookUp[@"Method Name"] = methodName;
    methodLookUp[@"Params"] = params;
    methodLookUp[@"Callback"] = callbackReturnValues;
    methodLookUp[@"Label"] = functionInvokeLabel;

    if (funcInvoke[@"SyncAttributes"] != nil) {
      methodLookUp[@"SyncAttributes"] = funcInvoke[@"SyncAttributes"];
    }

    [moduleCalls addObject:methodLookUp];
  }

  return LynxRecorderJSONString(json, @"{}");
}

- (NSString *)getJsbIgnoredInfo {
  return LynxRecorderJSONString(_jsbIgnoredInfo, @"[]");
}

- (NSString *)getJsbSettings {
  return LynxRecorderJSONString(_jsbSettings, @"{}");
}

@end
