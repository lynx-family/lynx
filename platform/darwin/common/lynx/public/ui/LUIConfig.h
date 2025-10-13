// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

@protocol LUIConfig <NSObject>
- (bool)defaultOverflowVisible;
- (bool)enableTextRefactor;
- (bool)enableTextOverflow;
- (bool)enableNewClipMode;
- (bool)globalImplicit;
- (bool)enableEventRefactor;
- (bool)enableA11yIDMutationObserver;
- (bool)enableEventThrough;
- (bool)enableBackgroundShapeLayer;
- (bool)enableExposureUIMargin;
- (bool)enableTextLanguageAlignment;
- (bool)enableXTextLayoutReused;
- (bool)enableFiberArch;
- (bool)enableNewGesture;
- (bool)CSSAlignWithLegacyW3C;
- (NSString*)targetSdkVersion;
- (bool)imageMonitorEnabled;
- (bool)devtoolEnabled;
- (bool)fixNewImageDownSampling;
- (CGFloat)fluencyPageConfigProbability;
- (bool)enableTextLayerRenderer;
- (bool)enableTextNonContiguousLayout;
- (bool)enableImageDownsampling;
- (bool)enableNewImage;
- (bool)trailUseNewImage;
- (NSUInteger)logBoxImageSizeWarningThreshold;
- (bool)enableTextLayoutCache;
- (bool)enableTextGradientOpt;
- (bool)enableExposureWhenReload;
@end
