// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxSettingPlatformDarwin.h"

#import <Lynx/LynxService.h>

namespace {

id<LynxServiceTrailProtocol> TrailService() {
  id<LynxServiceTrailProtocol> service = LynxTrail;
  if (![service respondsToSelector:@selector(getLayeredValues)]) {
    return nil;
  }
  return service;
}

NSString *JsonString(NSDictionary *object, NSString *__autoreleasing *errorMessage) {
  NSError *error = nil;
  NSData *data = [NSJSONSerialization dataWithJSONObject:object options:0 error:&error];
  if (!data) {
    if (errorMessage) {
      *errorMessage = error.localizedDescription ?: @"Failed to serialize LynxSetting result";
    }
    return @"{}";
  }
  return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] ?: @"{}";
}

NSArray<NSDictionary *> *LayerDictionaries(NSArray<LynxTrailValueLayer *> *layers) {
  NSMutableArray<NSDictionary *> *result = [NSMutableArray arrayWithCapacity:layers.count];
  for (LynxTrailValueLayer *layer in layers) {
    [result addObject:@{
      @"name" : layer.name,
      @"updatedAt" : @(layer.updatedAt),
      @"values" : layer.values,
    }];
  }
  return result;
}

NSDictionary<NSString *, NSString *> *MergedValues(NSArray<LynxTrailValueLayer *> *layers) {
  NSMutableDictionary<NSString *, NSString *> *result = [NSMutableDictionary dictionary];
  for (LynxTrailValueLayer *layer in layers) {
    [layer.values enumerateKeysAndObjectsUsingBlock:^(NSString *key, NSString *value, BOOL *stop) {
      if (!result[key]) {
        result[key] = value;
      }
    }];
  }
  return result;
}

NSDictionary *ValueResult(id<LynxServiceTrailProtocol> service, NSString *key) {
  NSArray<LynxTrailValueLayer *> *layers = [service getLayeredValues];
  NSMutableArray<NSDictionary *> *layerResults = [NSMutableArray arrayWithCapacity:layers.count];
  NSString *source = @"none";
  NSString *value = nil;
  for (LynxTrailValueLayer *layer in layers) {
    NSString *layerValue = layer.values[key];
    [layerResults addObject:@{
      @"name" : layer.name,
      @"updatedAt" : @(layer.updatedAt),
      @"values" : layerValue ? @{key : layerValue} : @{},
    }];
    if (!value && layerValue) {
      source = layer.name;
      value = layerValue;
    }
  }
  return @{
    @"key" : key,
    @"source" : source,
    @"value" : value ?: NSNull.null,
    @"layers" : layerResults,
  };
}

NSDictionary *FetchInfoResult(id<LynxServiceTrailProtocol> service) {
  NSInteger settingsTime = 0;
  for (LynxTrailValueLayer *layer in [service getLayeredValues]) {
    if (![layer.name isEqualToString:@"mock"]) {
      settingsTime = MAX(settingsTime, layer.updatedAt);
    }
  }
  return @{@"settingsTime" : @(settingsTime)};
}

}  // namespace

@implementation LynxSettingPlatformDarwin

+ (void)handleMethod:(NSString *)method
                 key:(NSString *)key
               value:(NSString *)value
            callback:(LynxSettingPlatformDarwinCallback)callback {
  id<LynxServiceTrailProtocol> service = TrailService();
  if (!service) {
    callback(@"{}", @"Lynx Trail Service does not support LynxSetting");
    return;
  }
  if ([method isEqualToString:@"LynxSetting.setMockValue"] ||
      [method isEqualToString:@"LynxSetting.removeMockValue"] ||
      [method isEqualToString:@"LynxSetting.clearMockValues"]) {
    BOOL success = NO;
    if ([method isEqualToString:@"LynxSetting.setMockValue"] &&
        [service respondsToSelector:@selector(setMockValue:forKey:)]) {
      success = [service setMockValue:value forKey:key];
    } else if ([method isEqualToString:@"LynxSetting.removeMockValue"] &&
               [service respondsToSelector:@selector(removeMockValueForKey:)]) {
      success = [service removeMockValueForKey:key];
    } else if ([method isEqualToString:@"LynxSetting.clearMockValues"] &&
               [service respondsToSelector:@selector(clearMockValues)]) {
      success = [service clearMockValues];
    }
    callback(@"{}", success ? nil : @"Failed to persist mock value");
    return;
  }
  if ([method isEqualToString:@"LynxSetting.fetchLatest"]) {
    if (![service respondsToSelector:@selector(fetchLatestSettings:)]) {
      callback(@"{}", @"Latest settings fetch is not supported");
      return;
    }
    [service fetchLatestSettings:^(BOOL success, NSString *errorMessage) {
      if (!success) {
        callback(@"{}", errorMessage ?: @"Fetch latest settings failed");
        return;
      }
      [self handleMethod:@"LynxSetting.getValues" key:@"" value:@"" callback:callback];
    }];
    return;
  }

  NSDictionary *result = nil;
  if ([method isEqualToString:@"LynxSetting.getValues"]) {
    NSArray<LynxTrailValueLayer *> *layers = [service getLayeredValues];
    result = @{@"values" : MergedValues(layers)};
  } else if ([method isEqualToString:@"LynxSetting.getLayeredValues"]) {
    NSArray<LynxTrailValueLayer *> *layers = [service getLayeredValues];
    result = @{
      @"layers" : LayerDictionaries(layers),
      @"merged" : MergedValues(layers),
    };
  } else if ([method isEqualToString:@"LynxSetting.getValue"]) {
    result = ValueResult(service, key);
  } else if ([method isEqualToString:@"LynxSetting.getFetchInfo"]) {
    result = FetchInfoResult(service);
  } else {
    callback(@"{}", [@"Not implemented: " stringByAppendingString:method]);
    return;
  }
  NSString *errorMessage = nil;
  NSString *json = JsonString(result, &errorMessage);
  callback(json, errorMessage);
}

@end
