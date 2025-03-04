// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchReplayDataModule.h"

// "Invoked Method Data" field from record json file
static NSArray *functionCall;
// "Callback" field from record json file
static NSDictionary *callbackData;

static NSArray *jsbIgnoredInfo;

static NSDictionary *jsbSettings;

static int64_t replayTime;

@implementation TestBenchReplayDataModule

+ (void)setTime:(int64_t)time {
  replayTime = time;
}

+ (NSString *)name {
  return @"TestBenchReplayDataModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"getData" : NSStringFromSelector(@selector(getData:)),
  };
}

+ (void)addFunctionCallArray:(NSArray *)responseData {
  functionCall = responseData;
}
+ (void)addCallbackDictionary:(NSDictionary *)callbackDictionary {
  callbackData = callbackDictionary;
}

+ (void)setJSbIgnoredInfo:(NSArray *)info {
  jsbIgnoredInfo = info;
}

+ (void)setJsbSettings:(NSDictionary *)settings {
  jsbSettings = settings;
}

- (void)getData:(LynxCallbackBlock)callback {
  NSMutableDictionary *json = [NSMutableDictionary new];
  [json setObject:[self getRecordData] forKey:@"RecordData"];
  [json setObject:[self getJsbSettings] forKey:@"JsbSettings"];
  [json setObject:[self getJsbIgnoredInfo] forKey:@"JsbIgnoredInfo"];
  NSData *jsonData = [NSJSONSerialization dataWithJSONObject:json options:0 error:0];
  NSString *strData = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  callback(strData);
}

- (NSString *)getRecordData {
  NSMutableDictionary<NSString *, NSMutableArray *> *json = [NSMutableDictionary new];
  for (int index = 0; index < [functionCall count]; index++) {
    NSDictionary *funcInvoke = functionCall[index];
    NSString *moduleName = [funcInvoke objectForKey:@"Module Name"];
    if (![json objectForKey:moduleName]) {
      [json setObject:[NSMutableArray new] forKey:moduleName];
    }

    NSMutableDictionary *methodLookUp = [NSMutableDictionary new];

    NSString *methodName = [funcInvoke objectForKey:@"Method Name"];

    long requestTime = [[funcInvoke objectForKey:@"Record Time"] doubleValue] * 1000;

    if ([funcInvoke objectForKey:@"RecordMillisecond"]) {
      requestTime = [[funcInvoke objectForKey:@"RecordMillisecond"] doubleValue];
    }

    NSMutableDictionary *params = [funcInvoke objectForKey:@"Params"];

    NSArray *callbackIDs = [params objectForKey:@"callback"];

    NSMutableArray *callbackReturnValues = [NSMutableArray new];

    NSString *functionInvokeLabel = @"";
    for (int i = 0; i < callbackIDs.count; i++) {
      NSDictionary *callbackInfo = [callbackData objectForKey:callbackIDs[i]];
      if (callbackInfo != nil) {
        long responseTime = [[callbackInfo objectForKey:@"Record Time"] doubleValue] * 1000;
        if ([callbackInfo objectForKey:@"RecordMillisecond"]) {
          responseTime = [[callbackInfo objectForKey:@"RecordMillisecond"] doubleValue];
        }
        NSDictionary *callbackKernel = @{
          @"Value" : [callbackInfo objectForKey:@"Params"],
          @"Delay" : @(responseTime - requestTime)
        };
        [callbackReturnValues addObject:callbackKernel];
      }
      functionInvokeLabel = [functionInvokeLabel stringByAppendingFormat:@"%@_", callbackIDs[i]];
    }

    [methodLookUp setObject:methodName forKey:@"Method Name"];
    [methodLookUp setObject:params forKey:@"Params"];
    [methodLookUp setObject:callbackReturnValues forKey:@"Callback"];
    [methodLookUp setObject:functionInvokeLabel forKey:@"Label"];

    if ([funcInvoke objectForKey:@"SyncAttributes"]) {
      [methodLookUp setObject:[funcInvoke objectForKey:@"SyncAttributes"] forKey:@"SyncAttributes"];
    }

    [[json objectForKey:moduleName] addObject:methodLookUp];
  }
  NSData *jsonData = [NSJSONSerialization dataWithJSONObject:json options:0 error:0];
  NSString *strData = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  return strData;
}

+ (NSString *)replayTimeEnvJScript {
  NSString *jsonPath = [[NSBundle mainBundle] pathForResource:@"Resource/testBench/testBench"
                                                       ofType:@"js"];
  NSData *data = [NSData dataWithContentsOfFile:jsonPath];
  NSString *script = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];

  script =
      [script stringByReplacingOccurrencesOfString:@"###TESTBENCH_REPLAY_TIME###"
                                        withString:[NSString stringWithFormat:@"%lld", replayTime]];

  NSString *docPath =
      [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];

  NSString *filePath = [docPath stringByAppendingPathComponent:@"testBench.js"];

  NSFileManager *fileManager = [NSFileManager defaultManager];

  [fileManager createFileAtPath:filePath
                       contents:[script dataUsingEncoding:NSUTF8StringEncoding]
                     attributes:nil];

  return [@"file://" stringByAppendingString:filePath];
}

- (NSString *)getJsbIgnoredInfo {
  if (jsbIgnoredInfo != nil) {
    NSData *reData = [NSJSONSerialization dataWithJSONObject:jsbIgnoredInfo options:0 error:0];
    NSString *infoData = [[NSString alloc] initWithData:reData encoding:NSUTF8StringEncoding];
    return infoData;
  } else {
    return @"[]";
  }
}

- (NSString *)getJsbSettings {
  if (jsbSettings != nil) {
    NSData *reData = [NSJSONSerialization dataWithJSONObject:jsbSettings options:0 error:0];
    NSString *settingData = [[NSString alloc] initWithData:reData encoding:NSUTF8StringEncoding];
    return settingData;
  } else {
    return @"{}";
  }
}

@end
