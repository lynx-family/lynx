// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_DEVTOOL_WRAPPER_DEVTOOLSETTINGS_H_
#define DARWIN_COMMON_LYNX_DEVTOOL_WRAPPER_DEVTOOLSETTINGS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

static NSString *const SP_KEY_ENABLE_DEVTOOL = @"enable_devtool";
static NSString *const SP_KEY_ENABLE_LOGBOX = @"enable_logbox";
static NSString *const SP_KEY_ENABLE_LAUNCH_RECORD = @"enable_launch_record";
static NSString *const SP_KEY_ENABLE_QUICKJS_DEBUG = @"enable_quickjs_debug";
static NSString *const SP_KEY_ENABLE_DOM_TREE = @"enable_dom_tree";
static NSString *const SP_KEY_ENABLE_LONG_PRESS_MENU = @"enable_long_press_menu";
static NSString *const SP_KEY_ENABLE_HIGHLIGHT_TOUCH = @"enable_highlight_touch";
static NSString *const SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT = @"enable_preview_screen_shot";
static NSString *const SP_KEY_ENABLE_FSP_SCREENSHOT = @"enable_fsp_screenshot";
static NSString *const SP_KEY_ENABLE_PERF_METRICS = @"enable_perf_metrics";

// The CDP domain keys are expected to be in the format of "enable_cdp_domain_{domain_name}".
// They need to stay public for callers to get/set enabled CDP domains.
static NSString *const SP_KEY_ENABLE_CDP_DOMAIN_CSS = @"enable_cdp_domain_css";
static NSString *const SP_KEY_ENABLE_CDP_DOMAIN_DEBUGGER = @"enable_cdp_domain_debugger";
static NSString *const SP_KEY_ENABLE_CDP_DOMAIN_DOM = @"enable_cdp_domain_dom";
static NSString *const SP_KEY_ENABLE_CDP_DOMAIN_OVERLAY = @"enable_cdp_domain_overlay";
static NSString *const SP_KEY_ENABLE_CDP_DOMAIN_PAGE = @"enable_cdp_domain_page";
static NSString *const SP_KEY_ENABLE_CDP_DOMAIN_RUNTIME = @"enable_cdp_domain_runtime";

/**
 * A centralized manager for DevTool user preferences and settings.
 * Handles persistence via NSUserDefaults and synchronizes values with the native C++ layer.
 *
 * Architectural Note on Lifecycle Checks:
 * This class acts as a pure "Data Layer" representing the user's raw intent or saved preference.
 * It intentionally DOES NOT check `DevToolLifecycle` or other system states when returning values.
 * This prevents "loss of information" where a user's preference is masked by a transient system
 * state. To get the "Effective State" (User Preference + System Capability), callers should query
 * the facade layer (e.g., `LynxEnv`), which combines this data with the lifecycle state.
 */
@interface DevToolSettings : NSObject

+ (instancetype)sharedInstance;

// Switch properties
@property(nonatomic, assign) BOOL devToolEnabled;
@property(nonatomic, assign) BOOL logBoxEnabled;
@property(nonatomic, assign) BOOL launchRecordEnabled;
@property(nonatomic, assign) BOOL quickjsDebugEnabled;
@property(nonatomic, assign) BOOL domTreeEnabled;
@property(nonatomic, assign) BOOL longPressMenuEnabled;
@property(nonatomic, assign) BOOL highlightTouchEnabled;
@property(nonatomic, assign) BOOL previewScreenshotEnabled;
@property(nonatomic, assign) BOOL fspScreenshotEnabled;
@property(nonatomic, assign) BOOL perfMetricsEnabled;

- (BOOL)isCSSErrorIgnored;
- (void)setCSSErrorIgnored:(BOOL)ignored;

- (NSSet<NSString *> *)ignoredErrorTypes;
- (void)setIgnoredErrorTypes:(NSSet<NSString *> *)errorTypes;
- (BOOL)isErrorTypeIgnored:(NSInteger)errorType;
- (void)setErrorType:(NSInteger)errorType ignored:(BOOL)ignored;

- (NSSet<NSString *> *)enabledCDPDomains;
- (void)setEnabledCDPDomains:(NSSet<NSString *> *)domains;
- (BOOL)isCDPDomainEnabled:(NSString *)key;
- (void)setCDPDomain:(NSString *)key enabled:(BOOL)enabled;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_DEVTOOL_WRAPPER_DEVTOOLSETTINGS_H_
