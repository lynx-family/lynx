// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/DevToolSettings.h>
#import <Lynx/LynxErrorBehavior.h>
#import <Lynx/LynxLog.h>
#include <string>
#include <unordered_set>

#include "core/renderer/utils/lynx_env.h"

@interface DevToolSettings ()

@property(nonatomic, strong) NSUserDefaults *defaults;

@end

@implementation DevToolSettings {
  // Member variables for non-persisted settings
  BOOL _highlightTouchEnabled;
  BOOL _perfMetricsEnabled;
  BOOL _previewScreenshotEnabled;
}

static NSString *const SP_KEY_ACTIVATED_CDP_DOMAINS = @"activated_cdp_domains";
// TODO(mitchilling): confirm whether ignored-error filtering is still active before removing this.
static NSString *const SP_KEY_IGNORE_ERROR_TYPES = @"ignore_error_types";

static NSString *const CDP_DOMAIN_KEY_PREFIX = @"enable_cdp_domain_";

+ (instancetype)sharedInstance {
  static DevToolSettings *_instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[DevToolSettings alloc] init];
  });
  return _instance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _defaults = [NSUserDefaults standardUserDefaults];

    // Initialize default values for non-persisted settings
    _highlightTouchEnabled = NO;
    _perfMetricsEnabled = NO;
    _previewScreenshotEnabled = YES;

    [self syncToNative];
  }
  return self;
}

#pragma mark - Properties

- (BOOL)devToolEnabled {
  /*!
   devToolEnabled
   @note Persistence: YES
   @note Sync to native: YES
   @note Default: NO
   */
  return [self boolForKey:SP_KEY_ENABLE_DEVTOOL defaultValue:NO];
}

- (void)setDevToolEnabled:(BOOL)devToolEnabled {
  [self setBool:devToolEnabled forKey:SP_KEY_ENABLE_DEVTOOL];
  [self syncToNative:SP_KEY_ENABLE_DEVTOOL value:devToolEnabled];
}

- (BOOL)logBoxEnabled {
  /*!
   logBoxEnabled
   @note Persistence: YES
   @note Sync to native: YES
   @note Default: YES
   */
  return [self boolForKey:SP_KEY_ENABLE_LOGBOX defaultValue:YES];
}

- (void)setLogBoxEnabled:(BOOL)logBoxEnabled {
  [self setBool:logBoxEnabled forKey:SP_KEY_ENABLE_LOGBOX];
  [self syncToNative:SP_KEY_ENABLE_LOGBOX value:logBoxEnabled];
}

- (BOOL)highlightTouchEnabled {
  /*!
   highlightTouchEnabled
   @note Persistence: NO
   @note Sync to native: NO
   @note Default: NO
   */
  return _highlightTouchEnabled;
}

- (void)setHighlightTouchEnabled:(BOOL)highlightTouchEnabled {
  _highlightTouchEnabled = highlightTouchEnabled;
}

- (BOOL)launchRecordEnabled {
  /*!
   launchRecordEnabled
   @note Persistence: YES
   @note Sync to native: NO
   @note Default: NO
   */
  return [self boolForKey:SP_KEY_ENABLE_LAUNCH_RECORD defaultValue:NO];
}

- (void)setLaunchRecordEnabled:(BOOL)launchRecordEnabled {
  [self setBool:launchRecordEnabled forKey:SP_KEY_ENABLE_LAUNCH_RECORD];
}

- (BOOL)quickjsDebugEnabled {
  /*!
   quickjsDebugEnabled
   @note Persistence: YES
   @note Sync to native: YES
   @note Default: YES
   */
  return [self boolForKey:SP_KEY_ENABLE_QUICKJS_DEBUG defaultValue:YES];
}

- (void)setQuickjsDebugEnabled:(BOOL)quickjsDebugEnabled {
  [self setBool:quickjsDebugEnabled forKey:SP_KEY_ENABLE_QUICKJS_DEBUG];
  [self syncToNative:SP_KEY_ENABLE_QUICKJS_DEBUG value:quickjsDebugEnabled];
}

- (BOOL)domTreeEnabled {
  /*!
   domTreeEnabled
   @note Persistence: YES
   @note Sync to native: YES
   @note Default: YES
   */
  return [self boolForKey:SP_KEY_ENABLE_DOM_TREE defaultValue:YES];
}

- (void)setDomTreeEnabled:(BOOL)domTreeEnabled {
  [self setBool:domTreeEnabled forKey:SP_KEY_ENABLE_DOM_TREE];
  [self syncToNative:SP_KEY_ENABLE_DOM_TREE value:domTreeEnabled];
}

- (BOOL)perfMetricsEnabled {
  /*!
   perfMetricsEnabled
   @note Persistence: NO
   @note Sync to native: NO
   @note Default: NO
   */
  return _perfMetricsEnabled;
}

- (void)setPerfMetricsEnabled:(BOOL)perfMetricsEnabled {
  _perfMetricsEnabled = perfMetricsEnabled;
}

- (BOOL)fspScreenshotEnabled {
  /*!
   fspScreenshotEnabled
   @note Persistence: YES
   @note Sync to native: NO
   @note Default: NO
   */
  return [self boolForKey:SP_KEY_ENABLE_FSP_SCREENSHOT defaultValue:NO];
}

- (void)setFspScreenshotEnabled:(BOOL)fspScreenshotEnabled {
  [self setBool:fspScreenshotEnabled forKey:SP_KEY_ENABLE_FSP_SCREENSHOT];
}

- (BOOL)longPressMenuEnabled {
  /*!
   longPressMenuEnabled
   @note Persistence: YES
   @note Sync to native: NO
   @note Default: YES
   */
  return [self boolForKey:SP_KEY_ENABLE_LONG_PRESS_MENU defaultValue:YES];
}

- (void)setLongPressMenuEnabled:(BOOL)longPressMenuEnabled {
  [self setBool:longPressMenuEnabled forKey:SP_KEY_ENABLE_LONG_PRESS_MENU];
}

- (BOOL)previewScreenshotEnabled {
  /*!
   previewScreenshotEnabled
   @note Persistence: NO
   @note Sync to native: NO
   @note Default: YES
   */
  return _previewScreenshotEnabled;
}

- (void)setPreviewScreenshotEnabled:(BOOL)previewScreenshotEnabled {
  _previewScreenshotEnabled = previewScreenshotEnabled;
}

#pragma mark - Helper Methods

- (BOOL)boolForKey:(NSString *)key defaultValue:(BOOL)defaultValue {
  if ([_defaults objectForKey:key]) {
    return [_defaults boolForKey:key];
  }
  return defaultValue;
}

- (void)setBool:(BOOL)value forKey:(NSString *)key {
  [_defaults setBool:value forKey:key];
}

- (NSSet<NSString *> *)stringSetForArrayKey:(NSString *)key {
  NSArray *values = [_defaults arrayForKey:key];
  if (![values isKindOfClass:[NSArray class]]) {
    return [NSSet set];
  }
  NSMutableSet<NSString *> *set = [[NSMutableSet alloc] init];
  for (id value in values) {
    if ([value isKindOfClass:[NSString class]]) {
      [set addObject:value];
    }
  }
  return [set copy];
}

- (void)setStringSet:(NSSet<NSString *> *)values forArrayKey:(NSString *)key {
  [_defaults setObject:[values allObjects] forKey:key];
}

- (NSSet<NSString *> *)stringSetForDictionaryKey:(NSString *)key {
  NSDictionary *values = [_defaults dictionaryForKey:key];
  if (![values isKindOfClass:[NSDictionary class]]) {
    return [NSSet set];
  }
  return [NSSet setWithArray:[values allKeys]];
}

- (void)setStringSet:(NSSet<NSString *> *)values forDictionaryKey:(NSString *)key {
  NSMutableDictionary *dictionary = [[NSMutableDictionary alloc] init];
  for (NSString *value in values) {
    dictionary[value] = @YES;
  }
  [_defaults setObject:dictionary forKey:key];
}

- (void)syncToNative:(NSString *)key value:(BOOL)value {
  lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv([key UTF8String], value ? true : false);
}

- (void)syncEnabledCDPDomainsToNative:(NSSet<NSString *> *)domains {
  std::unordered_set<std::string> domainSet;
  for (NSString *domain in domains) {
    domainSet.insert(std::string([domain UTF8String]));
  }
  lynx::tasm::LynxEnv::GetInstance().SetGroupedEnv(domainSet,
                                                   [SP_KEY_ACTIVATED_CDP_DOMAINS UTF8String]);
}

- (BOOL)verifyCDPDomainKey:(NSString *)key {
  if (![key hasPrefix:CDP_DOMAIN_KEY_PREFIX]) {
    LLogError(@"Invalid CDP domain key: %@", key);
    return NO;
  }
  return YES;
}

- (void)syncToNative {
  [self syncToNative:SP_KEY_ENABLE_DEVTOOL value:self.devToolEnabled];
  [self syncToNative:SP_KEY_ENABLE_LOGBOX value:self.logBoxEnabled];
  [self syncToNative:SP_KEY_ENABLE_QUICKJS_DEBUG value:self.quickjsDebugEnabled];
  [self syncToNative:SP_KEY_ENABLE_DOM_TREE value:self.domTreeEnabled];
  [self syncEnabledCDPDomainsToNative:[self enabledCDPDomains]];
}

- (BOOL)isCSSErrorIgnored {
  /*!
   isCSSErrorIgnored
   @note Persistence: YES
   @note Sync to native: NO
   @note Default: NO
   */
  return [self isErrorTypeIgnored:EBLynxCSS];
}

- (void)setCSSErrorIgnored:(BOOL)ignored {
  [self setErrorType:EBLynxCSS ignored:ignored];
}

- (NSSet<NSString *> *)ignoredErrorTypes {
  /*!
   ignoredErrorTypes
   @note Persistence: YES
   @note Sync to native: NO
   @note Default: empty set
   */
  return [self stringSetForDictionaryKey:SP_KEY_IGNORE_ERROR_TYPES];
}

- (void)setIgnoredErrorTypes:(NSSet<NSString *> *)errorTypes {
  // Take ownership of a stable snapshot before persistence.
  NSSet<NSString *> *errorTypeSnapshot = [errorTypes copy];
  [self setStringSet:errorTypeSnapshot forDictionaryKey:SP_KEY_IGNORE_ERROR_TYPES];
}

- (BOOL)isErrorTypeIgnored:(NSInteger)errorType {
  return [[self ignoredErrorTypes] containsObject:@(errorType).stringValue];
}

- (void)setErrorType:(NSInteger)errorType ignored:(BOOL)ignored {
  NSMutableSet<NSString *> *ignoredErrorTypes = [[self ignoredErrorTypes] mutableCopy];
  NSString *errorTypeKey = @(errorType).stringValue;
  BOOL changed = NO;
  if (ignored) {
    changed = ![ignoredErrorTypes containsObject:errorTypeKey];
    [ignoredErrorTypes addObject:errorTypeKey];
  } else {
    changed = [ignoredErrorTypes containsObject:errorTypeKey];
    [ignoredErrorTypes removeObject:errorTypeKey];
  }
  if (!changed) {
    return;
  }
  [self setStringSet:ignoredErrorTypes forDictionaryKey:SP_KEY_IGNORE_ERROR_TYPES];
}

- (NSSet<NSString *> *)enabledCDPDomains {
  /*!
   enabledCDPDomains
   @note Persistence: YES
   @note Sync to native: YES
   @note Default: empty set
   */
  return [self stringSetForArrayKey:SP_KEY_ACTIVATED_CDP_DOMAINS];
}

- (void)setEnabledCDPDomains:(NSSet<NSString *> *)domains {
  // Take ownership of a stable snapshot before validation, persistence, and native sync.
  NSSet<NSString *> *domainSnapshot = [domains copy];
  for (NSString *key in domainSnapshot) {
    if (![self verifyCDPDomainKey:key]) {
      return;
    }
  }
  [self setStringSet:domainSnapshot forArrayKey:SP_KEY_ACTIVATED_CDP_DOMAINS];
  [self syncEnabledCDPDomainsToNative:domainSnapshot];
}

- (BOOL)isCDPDomainEnabled:(NSString *)key {
  if (![self verifyCDPDomainKey:key]) {
    return NO;
  }
  return [[self enabledCDPDomains] containsObject:key];
}

- (void)setCDPDomain:(NSString *)key enabled:(BOOL)enabled {
  if (![self verifyCDPDomainKey:key]) {
    return;
  }
  NSMutableSet<NSString *> *enabledDomains = [[self enabledCDPDomains] mutableCopy];
  BOOL changed =
      enabled ? ![enabledDomains containsObject:key] : [enabledDomains containsObject:key];
  if (enabled) {
    [enabledDomains addObject:key];
  } else {
    [enabledDomains removeObject:key];
  }
  if (!changed) {
    return;
  }
  [self setStringSet:enabledDomains forArrayKey:SP_KEY_ACTIVATED_CDP_DOMAINS];
  [self syncEnabledCDPDomainsToNative:enabledDomains];
}

@end
