// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXENVKEY_H_
#define DARWIN_COMMON_LYNX_LYNXENVKEY_H_

#import <Foundation/Foundation.h>
#import <Lynx/DevToolSettings.h>

static NSString *const SP_KEY_ENABLE_AUTOMATION = @"enable_automation";

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
  LynxEnvEnableImageAsyncLayout,
  LynxEnvEnableImageCancelRequest,
  LynxEnvEnableImageCIGaussianBlur,
  LynxEnvEnableGenericResourceFetcher,
  LynxEnvEnableAnimationSyncTimeOpt,
  LynxEnvFixNewImageDownSampling,
  LynxEnvCachesExpirationDurationInDays,
  LynxEnvCachesCleanupUntrackedFiles,
  LynxEnvEnableTextContainerOpt,
  LynxEnvEnableJSGroupThreadByDefault,
  LynxEnvEnableTextLayoutCache,
  LynxEnvEnableForceMemoryMonitorOnOom,
  LynxEnvEnableTextGradientOpt,
  LynxEnvEnableTextFontCascadeOpt,
  LynxEnvGlobalMemoryReportThresholdMB,
  LynxEnvFSPEnable,
  LynxEnvFSPConfigJsonString,

  // Internal-only Lynx settings key. Do not mention graphics backend details in the key.
  LynxEnvSetupCanvasSurfaceEarlier,

  // Please add new enum values above
  LynxEnvKeyEndMark,  // Keep this as the last enum value, and do not use
};

#endif  // DARWIN_COMMON_LYNX_LYNXENVKEY_H_
