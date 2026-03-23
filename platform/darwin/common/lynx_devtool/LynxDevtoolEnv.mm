// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/DevToolSettings.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxEnvKey.h>
#import <Lynx/LynxError.h>
#import <Lynx/LynxErrorBehavior.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxUIKitAPIAdapter.h>
#import <LynxDevtool/LynxDevtoolEnv.h>

#include <LynxDevtool/LynxDebugBridge.h>
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/lynx_env.h"

NSString *const ERROR_CODE_KEY_PREFIX = @"error_code";
NSString *const CDP_DOMAIN_KEY_PREFIX = @"enable_cdp_domain";

enum KeyType { NORMAL_KEY, ERROR_KEY, CDP_DOMAIN_KEY };

@implementation LynxDevtoolEnv {
  dispatch_queue_t _read_write_queue;
  NSMutableDictionary *_groupDics;
  NSDictionary *_errorCodeDic;
}

+ (instancetype)sharedInstance {
  static LynxDevtoolEnv *_instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[LynxDevtoolEnv alloc] init];
  });

  return _instance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _read_write_queue =
        dispatch_queue_create("DevtoolEnv.read_write_queue", DISPATCH_QUEUE_CONCURRENT);

    /**
     * [self setDefaultAppInfo]
     *   -> [LynxDebugBridge singleton]
     *   -> [LynxInspectorOwner init]
     *   -> [DevtoolRuntimeManagerDarwinDelegate init]
     *   -> [LynxDevtoolEnv sharedInstance]
     *
     * NSOperationQueue is used to avoid above recursive call
     */
    [[NSOperationQueue mainQueue] addOperationWithBlock:^{
      [self setDefaultAppInfo];
    }];
    [self initErrorsCanBeIgnored];
    [self initGroupDictionaries];
    lynx::tasm::DevToolLifecycle::GetInstance().OnInitialized();
  }
  return self;
}

- (void)setDefaultAppInfo {
  NSDictionary *info = [[NSBundle mainBundle] infoDictionary];
  NSString *appName =
      info[@"CFBundleDisplayName"] ? info[@"CFBundleDisplayName"] : info[@"CFBundleName"];
  [[LynxDebugBridge singleton] setAppInfo:appName ? @{@"App" : appName} : @{}];
}

- (void)initErrorsCanBeIgnored {
  _errorCodeDic = @{SP_KEY_ENABLE_IGNORE_ERROR_CSS : @(EBLynxCSS).stringValue};
}

- (void)initGroupDictionaries {
  NSUserDefaults *preference = [NSUserDefaults standardUserDefaults];
  NSDictionary *storedErrorDic = [preference objectForKey:SP_KEY_IGNORE_ERROR_TYPES];
  NSMutableDictionary *ignoredErrors = [[NSMutableDictionary alloc] init];
  if (storedErrorDic) {
    [ignoredErrors addEntriesFromDictionary:storedErrorDic];
  }

  _groupDics = [[NSMutableDictionary alloc] init];
  [_groupDics setObject:ignoredErrors forKey:SP_KEY_IGNORE_ERROR_TYPES];
}

- (void)set:(BOOL)value forKey:(NSString *)key {
  if (!key) {
    LLogError(@"setDevtoolEnv error: nil key!");
    return;
  }
  BOOL persist = [self needPersist:key];
  BOOL sync = [self needSyncToNative:key];
  KeyType keyType = [self keyType:key];
  switch (keyType) {
    case CDP_DOMAIN_KEY: {
      [self setSwitch:value
          forSwitchKey:key
             groupName:SP_KEY_ACTIVATED_CDP_DOMAINS
           needPersist:persist
          syncToNative:sync];
      break;
    }
    case ERROR_KEY: {
      NSString *errorCode = [_errorCodeDic objectForKey:key];
      if (errorCode) {
        [self setSwitch:value
            forSwitchKey:errorCode
               groupName:SP_KEY_IGNORE_ERROR_TYPES
             needPersist:persist
            syncToNative:sync];
      }
      break;
    }
    default:
      LLogWarn(@"setDevtoolEnv unsupported key: %@", key);
      break;
  }
}

- (BOOL)get:(NSString *)key withDefaultValue:(BOOL)value {
  KeyType keyType = [self keyType:key];
  switch (keyType) {
    case CDP_DOMAIN_KEY: {
      return [self getSwitch:key withDefaultValue:value groupName:SP_KEY_ACTIVATED_CDP_DOMAINS];
    }
    case ERROR_KEY: {
      NSString *errorCode = [_errorCodeDic objectForKey:key];
      if (errorCode) {
        return [self getSwitch:errorCode
              withDefaultValue:value
                     groupName:SP_KEY_IGNORE_ERROR_TYPES];
      }
      return NO;
    }
    default:
      LLogWarn(@"getDevtoolEnv unsupported key: %@", key);
      return value;
  }
}

- (void)set:(NSSet *)newGroupValues forGroup:(NSString *)groupKey {
  if (!groupKey) {
    LLogError(@"setDevtoolEnv error: nil groupKey!");
    return;
  }
  if (!newGroupValues || [newGroupValues count] == 0) {
    return;
  }
  NSMutableDictionary *dic = [_groupDics objectForKey:groupKey];
  if (dic) {
    [dic removeAllObjects];
  } else {
    dic = [[NSMutableDictionary alloc] init];
    [_groupDics setObject:dic forKey:groupKey];
  }
  for (id key in newGroupValues) {
    if (![key isKindOfClass:[NSString class]]) {
      continue;
    }
    [dic setObject:[NSNumber numberWithBool:YES] forKey:key];
  }
  NSString *key = [newGroupValues anyObject];
  if ([self needPersist:key]) {
    NSUserDefaults *preference = [NSUserDefaults standardUserDefaults];
    [preference setObject:dic forKey:groupKey];
    [preference synchronize];
  }
  if ([self needSyncToNative:key]) {
    [self syncToNative:dic forGroup:groupKey];
  }
}

- (NSSet *)getGroup:(NSString *)groupKey {
  NSSet *retSet = nil;
  NSMutableDictionary *dic = [_groupDics objectForKey:groupKey];
  if (dic) {
    retSet = [[NSSet alloc] initWithArray:[dic allKeys]];
  }
  return retSet;
}

- (void)setSwitch:(BOOL)value
     forSwitchKey:(NSString *)switchKey
        groupName:(NSString *)groupKey
      needPersist:(BOOL)persist
     syncToNative:(BOOL)sync {
  NSMutableDictionary *dic = [_groupDics objectForKey:groupKey];
  if (!dic) {
    dic = [[NSMutableDictionary alloc] init];
    [_groupDics setObject:dic forKey:groupKey];
  }
  if (value) {
    [dic setValue:[NSNumber numberWithBool:value] forKey:switchKey];
  } else {
    [dic removeObjectForKey:switchKey];
  }
  if (persist) {
    NSUserDefaults *preference = [NSUserDefaults standardUserDefaults];
    [preference setObject:dic forKey:groupKey];
    [preference synchronize];
  }
  if (sync) {
    [self syncToNative:value forKey:switchKey groupName:groupKey];
  }
}

- (BOOL)getSwitch:(NSString *)switchKey
    withDefaultValue:(BOOL)defaultValue
           groupName:(NSString *)groupKey {
  NSMutableDictionary *dic = [_groupDics objectForKey:groupKey];
  if (dic) {
    id value = [dic valueForKey:switchKey];
    return value ? YES : NO;
  }
  return defaultValue;
}

- (void)syncToNative:(BOOL)value forKey:(NSString *)key {
  lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv([key UTF8String], value ? true : false);
}

- (void)syncToNative:(BOOL)value
              forKey:(nonnull NSString *)key
           groupName:(nonnull NSString *)groupKey {
  lynx::tasm::LynxEnv::GetInstance().SetGroupedEnv([key UTF8String], value ? true : false,
                                                   [groupKey UTF8String]);
}

- (void)syncToNative:(nonnull NSDictionary *)groupValues forGroup:(nonnull NSString *)groupKey {
  std::unordered_set<std::string> groupSet;
  for (NSString *key in groupValues) {
    groupSet.insert(std::string([key UTF8String]));
  }
  lynx::tasm::LynxEnv::GetInstance().SetGroupedEnv(groupSet, [groupKey UTF8String]);
}

- (BOOL)isErrorTypeIgnored:(NSInteger)errCode {
  return [self getSwitch:@(errCode).stringValue
        withDefaultValue:NO
               groupName:SP_KEY_IGNORE_ERROR_TYPES];
}

- (KeyType)keyType:(NSString *)key {
  if ([key hasPrefix:ERROR_CODE_KEY_PREFIX]) {
    return ERROR_KEY;
  } else if ([key hasPrefix:CDP_DOMAIN_KEY_PREFIX]) {
    return CDP_DOMAIN_KEY;
  } else {
    return NORMAL_KEY;
  }
}

- (BOOL)needPersist:(NSString *)key {
  KeyType keyType = [self keyType:key];
  switch (keyType) {
    case ERROR_KEY:
      return YES;
    default:
      return NO;
  }
}

- (BOOL)needSyncToNative:(NSString *)key {
  KeyType keyType = [self keyType:key];
  switch (keyType) {
    case CDP_DOMAIN_KEY:
      return YES;
    default:
      return NO;
  }
}

- (void)setShowDevtoolBadge:(BOOL)show __attribute__((deprecated("Deprecated after Lynx2.9"))) {
}

- (BOOL)showDevtoolBadge __attribute__((deprecated("Deprecated after Lynx2.9"))) {
  return NO;
}

- (void)setV8Enabled:(BOOL)enableV8 __attribute__((deprecated("Deprecated after Lynx3.1"))) {
}

- (BOOL)v8Enabled __attribute__((deprecated("Deprecated after Lynx3.1"))) {
  return NO;
}

- (void)setLongPressMenuEnabled:(BOOL)enableLongPressMenu {
#if OS_IOS
  [DevToolSettings sharedInstance].longPressMenuEnabled = enableLongPressMenu;
#endif
}

- (BOOL)longPressMenuEnabled {
#if OS_IOS
  return [DevToolSettings sharedInstance].longPressMenuEnabled;
#else
  return NO;
#endif
}

- (void)setDomTreeEnabled:(BOOL)enableDomTree {
  [DevToolSettings sharedInstance].domTreeEnabled = enableDomTree;
}

- (BOOL)domTreeEnabled {
  return [DevToolSettings sharedInstance].domTreeEnabled;
}

- (BOOL)previewScreenshotEnabled {
#if OS_IOS
  return [DevToolSettings sharedInstance].previewScreenshotEnabled;
#else
  return NO;
#endif
}

- (void)setQuickjsDebugEnabled:(BOOL)quickjsDebugEnabled {
  [DevToolSettings sharedInstance].quickjsDebugEnabled = quickjsDebugEnabled;
}

- (BOOL)quickjsDebugEnabled {
  return [DevToolSettings sharedInstance].quickjsDebugEnabled;
}

- (void)setPerfMetricsEnabled:(BOOL)enable {
  [DevToolSettings sharedInstance].perfMetricsEnabled = enable;
}

- (BOOL)perfMetricsEnabled {
  return [DevToolSettings sharedInstance].perfMetricsEnabled;
}

@end
