// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxScrollFluency.h"
#import "LynxUIContext.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIContext ()
@property(nonatomic, assign) BOOL imageMonitorEnabled;
@property(nonatomic, assign) BOOL devtoolEnabled;
@property(nonatomic, assign) BOOL fixNewImageDownSampling;
@property(nonatomic, strong, readwrite) LynxScrollFluency *fluencyInnerListener;

- (void)setDefaultOverflowVisible:(BOOL)enable;
- (void)setDefaultImplicitAnimation:(BOOL)enable;
- (void)setEnableTextRefactor:(BOOL)enable;
- (void)setEnableTextOverflow:(BOOL)enable;
- (void)setEnableNewClipMode:(BOOL)enable;
- (void)setEnableEventRefactor:(BOOL)enable;
- (void)setEnableA11yIDMutationObserver:(BOOL)enable;
- (void)setEnableEventThrough:(BOOL)enable;
- (void)setEnableBackgroundShapeLayer:(BOOL)enable;
- (void)setEnableFiberArch:(BOOL)enable;
- (void)setEnableNewGesture:(BOOL)enable;
- (void)setEnableExposureUIMargin:(BOOL)enable;
- (void)setEnableTextLanguageAlignment:(BOOL)enable;
- (void)setEnableXTextLayoutReused:(BOOL)enable;
- (void)setCSSAlignWithLegacyW3c:(BOOL)algin;
- (void)setTargetSdkVersion:(NSString *)version;
- (void)setEnableTextLayerRender:(BOOL)enable;
- (void)setEnableTextNonContiguousLayout:(BOOL)enable;
- (void)setEnableImageDownsampling:(BOOL)enable;
- (void)setEnableNewImage:(BOOL)enable;
- (void)setTrailUseNewImage:(BOOL)enable;
- (void)setLogBoxImageSizeWarningThreshold:(NSInteger)threshold;

- (void)didReceiveResourceError:(LynxError *)error
                     withSource:(NSString *)resourceUrl
                           type:(NSString *)type;
- (void)reportLynxError:(LynxError *)error;

- (int32_t)instanceId;

@end

NS_ASSUME_NONNULL_END
