// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxLog.h>
#import <Lynx/LynxMemoryRecord.h>
#import <Lynx/LynxMemoryUsageQuery.h>
#import <Lynx/LynxMemoryUsageResult.h>
#import <LynxDevtool/LynxMemoryController.h>
#if OS_IOS
#import <DebugRouter/DebugRouter.h>
#endif

namespace {

NSString* LynxDevtoolSafeString(NSString* value) { return value ?: @""; }

bool LynxDevtoolIsValidJsonKey(NSString* key) { return key && key.length > 0; }

NSString* LynxDevtoolMemoryStatusString(LynxMemoryCollectionStatus status) {
  return status == LynxMemoryCollectionStatusTimeout ? @"timeout" : @"completed";
}

NSDictionary* LynxDevtoolStringMapToDictionary(NSDictionary<NSString*, NSString*>* map) {
  if (map.count == 0) {
    return @{};
  }
  NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
  [map enumerateKeysAndObjectsUsingBlock:^(NSString* key, NSString* value, BOOL* stop) {
    if (!LynxDevtoolIsValidJsonKey(key)) {
      return;
    }
    result[key] = LynxDevtoolSafeString(value);
  }];
  return result;
}

NSDictionary* LynxDevtoolMemoryRecordToDictionary(LynxMemoryRecord* record) {
  if (!record) {
    return @{@"category" : @"", @"sizeBytes" : @0, @"instanceCount" : @0, @"detail" : @{}};
  }
  return @{
    @"category" : LynxDevtoolSafeString(record.category),
    @"sizeBytes" : @(record.sizeBytes),
    @"instanceCount" : @(record.instanceCount),
    @"detail" : LynxDevtoolStringMapToDictionary(record.detail)
  };
}

NSDictionary* LynxDevtoolViewDetailToDictionary(
    NSDictionary<NSString*, LynxMemoryRecord*>* viewDetail) {
  if (viewDetail.count == 0) {
    return @{};
  }
  NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
  [viewDetail
      enumerateKeysAndObjectsUsingBlock:^(NSString* key, LynxMemoryRecord* record, BOOL* stop) {
        if (!LynxDevtoolIsValidJsonKey(key)) {
          return;
        }
        result[key] = LynxDevtoolMemoryRecordToDictionary(record);
      }];
  return result;
}

NSDictionary* LynxDevtoolInstanceMemoryUsageToDictionary(LynxInstanceMemoryUsage* instance) {
  return @{
    @"instanceId" : @(instance.instanceId),
    @"pageId" : LynxDevtoolSafeString(instance.pageId),
    @"url" : LynxDevtoolSafeString(instance.url),
    @"totalBytes" : @(instance.totalBytes),
    @"elementBytes" : @(instance.elementBytes),
    @"elementNodeCount" : @(instance.elementNodeCount),
    @"viewBytes" : @(instance.viewBytes),
    @"viewDetail" : LynxDevtoolViewDetailToDictionary(instance.viewDetail),
    @"mainThreadRuntimeBytes" : @(instance.mainThreadRuntimeBytes),
    @"backgroundThreadRuntimeBytes" : @(instance.backgroundThreadRuntimeBytes),
    @"btsRuntimeGroupId" : LynxDevtoolSafeString(instance.btsRuntimeGroupId)
  };
}

NSDictionary* LynxDevtoolGlobalMemoryUsageToDictionary(LynxGlobalMemoryUsageResult* result) {
  NSMutableArray* instances = [[NSMutableArray alloc] init];
  for (LynxInstanceMemoryUsage* instance in result.instances) {
    [instances addObject:LynxDevtoolInstanceMemoryUsageToDictionary(instance)];
  }
  return @{
    @"collectionStartMs" : @(result.collectionStartMs),
    @"collectionStatus" : LynxDevtoolMemoryStatusString(result.collectionStatus),
    @"collectionDurationMs" : @(result.collectionDurationMs),
    @"collectionTimeoutMs" : @(result.collectionTimeoutMs),
    @"expectedInstanceCount" : @(result.expectedInstanceCount),
    @"completedInstanceCount" : @(result.completedInstanceCount),
    @"totalBytes" : @(result.totalBytes),
    @"appBytes" : @(result.appBytes),
    @"ratioToApp" : @(result.ratioToApp),
    @"elementBytes" : @(result.elementBytes),
    @"elementNodeCount" : @(result.elementNodeCount),
    @"viewBytes" : @(result.viewBytes),
    @"mainThreadRuntimeBytes" : @(result.mainThreadRuntimeBytes),
    @"backgroundThreadRuntimeBytes" : @(result.backgroundThreadRuntimeBytes),
    @"instances" : instances
  };
}

NSString* LynxDevtoolGlobalMemoryUsageToJSONString(LynxGlobalMemoryUsageResult* result,
                                                   NSString** errorMessage) {
  NSDictionary* dictionary = LynxDevtoolGlobalMemoryUsageToDictionary(result);
  if (![NSJSONSerialization isValidJSONObject:dictionary]) {
    if (errorMessage) {
      *errorMessage = @"Failed to serialize Lynx memory usage result";
    }
    return nil;
  }
  NSError* error = nil;
  NSData* jsonData = [NSJSONSerialization dataWithJSONObject:dictionary options:0 error:&error];
  if (!jsonData) {
    if (errorMessage) {
      *errorMessage = error.localizedDescription ?: @"Failed to serialize Lynx memory usage result";
    }
    return nil;
  }
  return [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
}

}  // namespace

@implementation LynxMemoryController

+ (instancetype)shareInstance {
  static LynxMemoryController* instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[self alloc] init];
  });
  return instance;
}

- (void)uploadImageInfo:(NSDictionary*)data {
  static NSString* method = @"Memory.uploadImageInfo";
  static NSString* type = @"CDP";
  if (data == nil) {
    LLogWarn(@"LynxMemoryController uploadImageInfo warning: data is nil");
    return;
  }
  NSDictionary* param = [[NSMutableDictionary alloc] initWithDictionary:@{@"data" : data}];
  NSMutableDictionary* msg = [[NSMutableDictionary alloc] init];
  msg[@"params"] = param;
  msg[@"method"] = method;
  if ([NSJSONSerialization isValidJSONObject:msg]) {
    NSData* jsonData = [NSJSONSerialization dataWithJSONObject:msg options:0 error:nil];
    NSString* jsonStr = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
    [[DebugRouter instance] sendDataAsync:jsonStr WithType:type ForSession:-1];
  }
}

- (void)startMemoryTracing {
  [[LynxMemoryListener shareInstance] addMemoryReporter:self];
}

- (void)stopMemoryTracing {
  [[LynxMemoryListener shareInstance] removeMemoryReporter:self];
}

- (void)queryAllMemoryUsageWithTimeoutMs:(int64_t)timeoutMs
                                callback:(LynxMemoryUsageResultCallback)callback {
  if (!callback) {
    return;
  }
  @try {
    [[LynxMemoryUsageQuery sharedInstance]
        queryLynxGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult* result) {
          NSString* errorMessage = nil;
          NSString* resultJson = LynxDevtoolGlobalMemoryUsageToJSONString(result, &errorMessage);
          if (!resultJson) {
            callback(@"{}", errorMessage ?: @"Failed to serialize Lynx memory usage result");
            return;
          }
          callback(resultJson, @"");
        }
                              timeoutMs:timeoutMs];
  } @catch (NSException* exception) {
    NSString* reason = exception.reason ?: @"unknown error";
    callback(@"{}", [@"Failed to query Lynx memory usage: " stringByAppendingString:reason]);
  }
}
@end
