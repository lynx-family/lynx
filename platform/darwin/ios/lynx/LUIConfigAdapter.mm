// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LUIConfigAdapter.h>
#import "LynxEnv+Internal.h"

@implementation LUIConfigAdapter {
  lynx::tasm::PageConfig* _config;
}

- (instancetype)initWithConfig:(lynx::tasm::PageConfig*)config {
  self = [super init];
  if (self) {
    _config = config;
  }
  return self;
}

- (bool)CSSAlignWithLegacyW3C {
  return _config->GetCSSAlignWithLegacyW3C();
}

- (bool)defaultOverflowVisible {
  return _config->GetDefaultOverflowVisible();
}

- (bool)devtoolEnabled {
  return [[LynxEnv sharedInstance] devtoolEnabled];
}

- (bool)enableA11yIDMutationObserver {
  return _config->GetEnableA11yIDMutationObserver();
}

- (bool)enableBackgroundShapeLayer {
  return _config->GetEnableBackgroundShapeLayer();
}

- (bool)enableEventRefactor {
  return _config->GetEnableEventRefactor();
}

- (bool)enableEventThrough {
  return _config->GetEnableEventThrough();
}

- (bool)enableExposureUIMargin {
  return _config->GetEnableExposureUIMargin();
}

- (bool)enableFiberArch {
  return _config->GetEnableFiberArch();
}

- (bool)enableImageDownsampling {
  return _config->GetEnableImageDownsampling();
}

- (bool)enableNewClipMode {
  return _config->GetEnableNewClipMode();
}

- (bool)enableNewGesture {
  return _config->GetEnableNewGesture();
}

- (bool)enablePlatformGesture {
  return _config->GetEnablePlatformGesture();
}

- (bool)enableNewImage {
  return _config->GetEnableNewImage();
}

- (bool)enableTextLanguageAlignment {
  return _config->GetEnableTextLanguageAlignment();
}

- (bool)enableTextLayerRenderer {
  if (_config->GetEnableTextLayerRender() == lynx::tasm::TernaryBool::UNDEFINE_VALUE) {
    auto new_value = [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableTextLayerRender
                                                      defaultValue:NO];
    _config->SetEnableTextLayerRender(new_value ? lynx::tasm::TernaryBool::TRUE_VALUE
                                                : lynx::tasm::TernaryBool::FALSE_VALUE);
  }

  return _config->GetEnableTextLayerRender() == lynx::tasm::TernaryBool::TRUE_VALUE;
}

- (bool)enableTextLayoutCache {
  if (_config->GetEnableTextLayoutCache() == lynx::tasm::TernaryBool::UNDEFINE_VALUE) {
    auto new_value = [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableTextLayoutCache
                                                      defaultValue:YES];
    _config->SetEnableTextLayoutCache(new_value ? lynx::tasm::TernaryBool::TRUE_VALUE
                                                : lynx::tasm::TernaryBool::FALSE_VALUE);
  }

  return _config->GetEnableTextLayoutCache() == lynx::tasm::TernaryBool::TRUE_VALUE;
}

- (bool)enableTextNonContiguousLayout {
  return _config->GetEnableTextNonContiguousLayout();
}

- (bool)enableTextOverflow {
  return _config->GetEnableTextOverflow();
}

- (bool)enableTextRefactor {
  return _config->GetEnableTextRefactor();
}

- (bool)enableXTextLayoutReused {
  return _config->GetEnableXTextLayoutReused();
}

- (bool)fixNewImageDownSampling {
  return [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvFixNewImageDownSampling
                                          defaultValue:YES];
}

- (CGFloat)fluencyPageConfigProbability {
  return _config->GetEnableScrollFluencyMonitor();
}

- (bool)globalImplicit {
  return _config->GetGlobalImplicit();
}

- (bool)imageMonitorEnabled {
  return [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableImageMonitor defaultValue:YES];
}

- (NSUInteger)logBoxImageSizeWarningThreshold {
  return _config->GetLogBoxImageSizeWarningThreshold();
}

- (NSString*)targetSdkVersion {
  return [NSString stringWithUTF8String:_config->GetTargetSDKVersion().c_str()];
}

- (bool)trailUseNewImage {
  return _config->GetTrailNewImage() == lynx::tasm::TernaryBool::TRUE_VALUE;
}

- (BOOL)enableTextGradientOpt {
  return _config->GetEnableTextGradientOpt() == lynx::tasm::TernaryBool::UNDEFINE_VALUE
             ? [[LynxEnv sharedInstance] enableTextGradientOpt]
             : _config->GetEnableTextGradientOpt() == lynx::tasm::TernaryBool::TRUE_VALUE;
}

- (BOOL)enableExposureWhenReload {
  return _config->GetEnableExposureWhenReload();
}

- (BOOL)enableiOSAnimationLayerForExposure {
  return _config->GetEnableiOSAnimationLayerForExposure();
}

@end
