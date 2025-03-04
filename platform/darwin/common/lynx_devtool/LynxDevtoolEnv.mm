// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxDevtoolEnv.h"
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxEnvKey.h>
#import <Lynx/LynxError.h>
#import <Lynx/LynxErrorBehavior.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxUIKitAPIAdapter.h>
#if OS_IOS
#import "DevtoolWebSocketModule.h"
#endif

#include "core/renderer/utils/lynx_env.h"
#include "platform/darwin/common/lynx_devtool/LynxDebugBridge.h"

NSString *const ERROR_CODE_KEY_PREFIX = @"error_code";
NSString *const CDP_DOMAIN_KEY_PREFIX = @"enable_cdp_domain";
static NSString *const ENABLE_PERF_METRICS = @"enable_perf_metrics";

enum KeyType { NORMAL_KEY, ERROR_KEY, CDP_DOMAIN_KEY };

@implementation LynxDevtoolEnv {
  dispatch_queue_t _read_write_queue;
  NSMutableDictionary *_switchMasks;
  NSMutableDictionary *_groupDics;
  NSDictionary *_errorCodeDic;
  NSMutableDictionary *_switchNotPersist;
  NSDictionary *_switchAttrDic;
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
    _switchMasks = [[NSMutableDictionary alloc] init];
    _switchNotPersist = [[NSMutableDictionary alloc] init];

    [self initSwitchAttribute];
    [self initSyncToNative];
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
  }
  return self;
}

- (void)initSwitchAttribute {
  /**
   _switchAttrDic: A dictionary indicating all switches' attributes.
   key: switch name.
   value: an array of bool values indicating attributes of current switch.
      The meaning of each value in array is as follows:
        whether needs to be persisted
        whether needs to be synchronized to native
        default value
   */
  _switchAttrDic = @{
    SP_KEY_ENABLE_DEVTOOL : @[ @YES, @YES, @NO ],
    SP_KEY_ENABLE_LOGBOX : @[ @YES, @YES, @YES ],
    SP_KEY_ENABLE_HIGHLIGHT_TOUCH : @[ @NO, @NO, @NO ],
    SP_KEY_ENABLE_LAUNCH_RECORD : @[ @YES, @NO, @NO ],
#if OS_IOS
    SP_KEY_ENABLE_LONG_PRESS_MENU : @[ @YES, @NO, @YES ],
    SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT : @[ @NO, @NO, @YES ],
#endif
    SP_KEY_ENABLE_QUICKJS_DEBUG : @[ @YES, @YES, @YES ],
    SP_KEY_ENABLE_DOM_TREE : @[ @YES, @YES, @YES ],
    SP_KEY_ENABLE_DEVTOOL_FOR_DEBUGGABLE_VIEW : @[ @NO, @YES, @NO ],
    SP_KEY_DEVTOOL_CONNECTED : @[ @NO, @YES, @NO ],
    ENABLE_PERF_METRICS : @[ @NO, @NO, @NO ]
  };
}

- (void)initSyncToNative {
  [self syncToNative:[self get:SP_KEY_ENABLE_DEVTOOL withDefaultValue:NO]
              forKey:SP_KEY_ENABLE_DEVTOOL];
  [self syncToNative:[self domTreeEnabled] forKey:SP_KEY_ENABLE_DOM_TREE];
  [self syncToNative:[self get:SP_KEY_ENABLE_LOGBOX withDefaultValue:YES]
              forKey:SP_KEY_ENABLE_LOGBOX];
  [self syncToNative:[self quickjsDebugEnabled] forKey:SP_KEY_ENABLE_QUICKJS_DEBUG];
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
    case NORMAL_KEY: {
      [self setSwitch:value forKey:key needPersist:persist syncToNative:sync];
      break;
    }
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
      break;
  }
}

- (BOOL)get:(NSString *)key withDefaultValue:(BOOL)value {
  KeyType keyType = [self keyType:key];
  switch (keyType) {
    case NORMAL_KEY: {
      return [self getSwitch:key withDefaultValue:value];
    }
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
      return NO;
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
           forKey:(NSString *)key
      needPersist:(BOOL)persist
     syncToNative:(BOOL)sync {
  BOOL curValue = [self getSwitch:key withDefaultValue:[self getDefaultValue:key]];

  if (persist) {
    NSUserDefaults *preference = [NSUserDefaults standardUserDefaults];
    [preference setBool:value forKey:key];
    [preference synchronize];
  } else {
    [_switchNotPersist setValue:[NSNumber numberWithBool:value] forKey:key];
  }
  if (sync) {
    [self syncToNative:value forKey:key];
  }

  [self killAppIfChangeJsEngine:key withValue:curValue];
}

- (BOOL)getSwitch:(NSString *)key withDefaultValue:(BOOL)value {
  NSUserDefaults *preference = [NSUserDefaults standardUserDefaults];
  BOOL retValue = value;
  if ([preference objectForKey:key]) {
    retValue = [preference boolForKey:key];
  } else if ([_switchNotPersist objectForKey:key]) {
    retValue = [[_switchNotPersist objectForKey:key] boolValue];
  }
  return retValue && [self getSwitchMask:key];
}

- (void)setSwitchMask:(BOOL)value forKey:(NSString *)key {
  dispatch_barrier_async(_read_write_queue, ^{
    [self->_switchMasks setValue:[NSNumber numberWithBool:value] forKey:key];
  });
  [self syncMaskToNative:value forKey:key];
}

- (BOOL)getSwitchMask:(NSString *)key {
  __block NSNumber *value;
  dispatch_sync(_read_write_queue, ^{
    value = [_switchMasks valueForKey:key];
  });
  if (value) {
    return [value boolValue];
  }
  return YES;
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

- (void)syncMaskToNative:(BOOL)value forKey:(NSString *)key {
  lynx::tasm::LynxEnv::GetInstance().SetEnvMask([key UTF8String], value ? true : false);
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
    case NORMAL_KEY: {
      NSArray *arr = [_switchAttrDic objectForKey:key];
      if (arr && [arr objectAtIndex:0]) {
        return [[arr objectAtIndex:0] boolValue];
      }
    }
    default:
      return NO;
  }
}

- (BOOL)needSyncToNative:(NSString *)key {
  KeyType keyType = [self keyType:key];
  switch (keyType) {
    case CDP_DOMAIN_KEY:
      return YES;
    case NORMAL_KEY: {
      NSArray *arr = [_switchAttrDic objectForKey:key];
      if (arr && [arr objectAtIndex:1]) {
        return [[arr objectAtIndex:1] boolValue];
      }
    }
    default:
      return NO;
  }
}

- (BOOL)getDefaultValue:(NSString *)key {
  NSArray *arr = [_switchAttrDic objectForKey:key];
  if (arr && [arr objectAtIndex:2]) {
    return [[arr objectAtIndex:2] boolValue];
  }
  return NO;
}

- (void)prepareConfig:(LynxConfig *)config {
#if OS_IOS
  [config registerModule:[DevtoolWebSocketModule class]];
#endif
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
  [self set:enableLongPressMenu forKey:SP_KEY_ENABLE_LONG_PRESS_MENU];
#endif
}

- (BOOL)longPressMenuEnabled {
#if OS_IOS
  return [self get:SP_KEY_ENABLE_LONG_PRESS_MENU withDefaultValue:YES];
#else
  return NO;
#endif
}

- (void)setDomTreeEnabled:(BOOL)enableDomTree {
  [self set:enableDomTree forKey:SP_KEY_ENABLE_DOM_TREE];
}

- (BOOL)domTreeEnabled {
  return [self get:SP_KEY_ENABLE_DOM_TREE withDefaultValue:YES];
}

- (BOOL)previewScreenshotEnabled {
#if OS_IOS
  return [self get:SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT withDefaultValue:YES];
#else
  return NO;
#endif
}

- (void)setQuickjsDebugEnabled:(BOOL)quickjsDebugEnabled {
  [self set:quickjsDebugEnabled forKey:SP_KEY_ENABLE_QUICKJS_DEBUG];
}

- (BOOL)quickjsDebugEnabled {
  return [self get:SP_KEY_ENABLE_QUICKJS_DEBUG withDefaultValue:YES];
}

- (void)killAppIfChangeJsEngine:(NSString *)key withValue:(BOOL)value {
#if OS_IOS
  if ([key isEqualToString:SP_KEY_ENABLE_DEVTOOL]) {
    BOOL curValue = [self get:SP_KEY_ENABLE_DEVTOOL withDefaultValue:NO];
    if (curValue != value) {
      dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertController *alert = [UIAlertController
            alertControllerWithTitle:@""
                             message:@"JS engine has changed, app needs to be restarted!"
                      preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"Confirm"
                                                  style:UIAlertActionStyleDefault
                                                handler:^(UIAlertAction *action) {
                                                  [self KillApp];
                                                }]];
        UIViewController *controller = [LynxUIKitAPIAdapter getKeyWindow].rootViewController;
        UIViewController *presentedController = controller.presentedViewController;
        while (presentedController && ![presentedController isBeingDismissed]) {
          controller = presentedController;
          presentedController = controller.presentedViewController;
        }
        [controller presentViewController:alert animated:YES completion:nil];
      });
    }
  }
#endif
}

- (void)KillApp {
#if OS_IOS
  dispatch_async(dispatch_get_main_queue(), ^{
    UIApplication *app = [UIApplication sharedApplication];
    [app performSelector:@selector(suspend)];
    exit(0);
  });
#endif
}

- (void)setPerfMetricsEnabled:(BOOL)enable {
  [self set:enable forKey:ENABLE_PERF_METRICS];
}

- (BOOL)perfMetricsEnabled {
  return [self get:ENABLE_PERF_METRICS withDefaultValue:NO];
}

@end
