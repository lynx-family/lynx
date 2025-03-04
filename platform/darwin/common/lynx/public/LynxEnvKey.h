// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXENVKEY_H_
#define DARWIN_COMMON_LYNX_LYNXENVKEY_H_

#import <Foundation/Foundation.h>

static NSString *const SP_KEY_ENABLE_AUTOMATION = @"enable_automation";

// Keys for devtool.
static NSString *const KEY_LYNX_DEBUG = @"enable_lynx_debug";
static NSString *const KEY_DEVTOOL_COMPONENT_ATTACH = @"devtool_component_attach";
static NSString *const SP_KEY_ENABLE_DEVTOOL = @"enable_devtool";
static NSString *const SP_KEY_ENABLE_DEVTOOL_FOR_DEBUGGABLE_VIEW =
    @"enable_devtool_for_debuggable_view";
static NSString *const SP_KEY_ENABLE_LOGBOX = @"enable_logbox";
static NSString *const SP_KEY_ENABLE_HIGHLIGHT_TOUCH = @"enable_highlight_touch";
static NSString *const SP_KEY_ENABLE_LAUNCH_RECORD = @"enable_launch_record";
static NSString *const SP_KEY_ENABLE_DOM_TREE = @"enable_dom_tree";
static NSString *const SP_KEY_ENABLE_LONG_PRESS_MENU = @"enable_long_press_menu";
static NSString *const SP_KEY_IGNORE_ERROR_TYPES = @"ignore_error_types";
static NSString *const SP_KEY_ENABLE_IGNORE_ERROR_CSS = @"error_code_css";
static NSString *const SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT = @"enable_preview_screen_shot";
static NSString *const SP_KEY_ACTIVATED_CDP_DOMAINS = @"activated_cdp_domains";
static NSString *const SP_KEY_ENABLE_CDP_DOMAIN_DOM = @"enable_cdp_domain_dom";
static NSString *const SP_KEY_ENABLE_CDP_DOMAIN_CSS = @"enable_cdp_domain_css";
static NSString *const SP_KEY_ENABLE_CDP_DOMAIN_PAGE = @"enable_cdp_domain_page";
static NSString *const SP_KEY_DEVTOOL_CONNECTED = @"devtool_connected";
static NSString *const SP_KEY_ENABLE_QUICKJS_DEBUG = @"enable_quickjs_debug";
// deprecated after Lynx2.9
static NSString *const SP_KEY_SHOW_DEVTOOL_BADGE = @"show_devtool_badge";

typedef NS_ENUM(uint64_t, LynxEnvKey) {
  LynxEnvSwitchRunloopThread = 0,
  LynxEnvEnableComponentStatisticReport,
  LynxEnvEnableLynxDetailLog,
  LynxEnvFreeImageMemory,
  LynxEnvFreeImageMemoryForce,
  LynxEnvUseNewImage,
  LynxEnvEnableImageExposure,
  LynxEnvEnableMultiTASMThread,
  LynxEnvEnableMultiLayoutThread,
  LynxEnvTextRenderCacheLimit,
  LynxEnvEnableTextRenderCacheHitRate,
  LynxEnvEnableImageMonitor,
  LynxEnvEnableTextLayerRender,
  LynxEnvEnableCreateUIAsync,
  LynxEnvEnableImageEventReport,
  LynxEnvEnableGenericResourceFetcher,
  LynxEnvEnableAnimationSyncTimeOpt,
  LynxEnvFixNewImageDownSampling,
  LynxEnvCachesExpirationDurationInDays,
  LynxEnvEnableLifecycleTimeReport,
  LynxEnvCachesCleanupUntrackedFiles,
  LynxEnvEnableTextContainerOpt,

  // Please add new enum values above
  LynxEnvKeyEndMark,  // Keep this as the last enum value, and do not use
};

#endif  // DARWIN_COMMON_LYNX_LYNXENVKEY_H_
