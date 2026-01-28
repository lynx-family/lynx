// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXBASECONFIGURATOR_H_
#define PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXBASECONFIGURATOR_H_

#import <Lynx/LynxBackgroundRuntime.h>
#import <Lynx/LynxBaseConfigurator.h>
#import <Lynx/LynxBooleanOption.h>
#import <Lynx/LynxConfig.h>
#import <Lynx/LynxDynamicComponentFetcher.h>
#import <Lynx/LynxGenericResourceFetcher.h>
#import <Lynx/LynxGroup.h>
#import <Lynx/LynxMediaResourceFetcher.h>
#import <Lynx/LynxTemplateResourceFetcher.h>
#import <Lynx/LynxUIRendererCreatorProtocol.h>
#import <Lynx/LynxViewEnum.h>

@interface LynxBaseConfigurator : NSObject {
 @protected
  LynxThreadStrategyForRender _threadStrategy;
  BOOL _hasThreadStrategySet;
  LynxConfig* _config;
  BOOL _hasPendingJsTaskSet;

  LynxBooleanOption _enableGenericResourceFetcher;
  id<LynxGenericResourceFetcher> _genericResourceFetcher;
  id<LynxMediaResourceFetcher> _mediaResourceFetcher;
  id<LynxTemplateResourceFetcher> _templateResourceFetcher;
}

@property(nonatomic, nullable) LynxConfig* config;
@property(nonatomic, nullable) LynxGroup* group;

@property(nonatomic, assign) BOOL enableLayoutSafepoint;
@property(nonatomic, assign) BOOL enableAutoExpose;
@property(nonatomic, assign) BOOL enableTextNonContiguousLayout;
@property(nonatomic, assign) BOOL enableLayoutOnly;
@property(nonatomic, assign) BOOL enableUIOperationQueue;
@property(nonatomic, assign) BOOL enablePendingJSTaskOnLayout;
@property(nonatomic, assign) BOOL enableJSRuntime;
@property(nonatomic, assign) BOOL enableAirStrictMode;
@property(nonatomic, assign) BOOL enableAsyncCreateRender;
@property(nonatomic, assign) BOOL enableSyncFlush;
@property(nonatomic, assign) BOOL enableMultiAsyncThread;
@property(nonatomic, assign) BOOL enableVSyncAlignedMessageLoop;
// Run the hydration process in a async thread.
@property(nonatomic, assign) BOOL enableAsyncHydration;
@property(nonatomic, assign) BOOL enableMTSModule;
@property(nonatomic, assign) CGFloat fontScale;
@property(nonatomic, assign) LynxBooleanOption enableGenericResourceFetcher;

// generic resource fetcher api.
@property(nonatomic, nonnull) id<LynxGenericResourceFetcher> genericResourceFetcher;
@property(nonatomic, nonnull) id<LynxMediaResourceFetcher> mediaResourceFetcher;
@property(nonatomic, nonnull) id<LynxTemplateResourceFetcher> templateResourceFetcher;
// enable LynxMediaResourceFetcher::fetchUIImage
@property(nonatomic, assign) BOOL enableFetchUIImage;

/**
 * You can set a virtual screen size to lynxview by this way.
 * Generally, you don't need to set it.The screen size of lynx is the real device size by default.
 * It will be useful for the split-window, this case, it can make some css properties based on rpx
 * shows better.
 * screenSize.width (dp)
 * screenSize.height (dp)
 */
@property(nonatomic, assign) CGSize screenSize;

/**
 * indicates lynx view can be debug or not
 * when switch enableDevtool is disabled and
 * switch enableDevtoolForDebuggableView is enabled
 */
@property(nonatomic, assign) BOOL debuggable;

/**
 * Control whether updateData can take effect before loadTemplate
 */
@property(nonatomic, assign) BOOL enablePreUpdateData;

/**
 * Set backgroundJSRuntime js engine type.
 */
@property(nonatomic, assign) LynxBackgroundJsRuntimeType backgroundJsRuntimeType;

/**
 * Only when backgroundJsRuntimeType == LynxBackgroundJsRuntimeTypeQuickjs,it will take effect.
 * Enable bytecode for quickjs.
 */
@property(nonatomic, assign) BOOL enableBytecode;

/**
 * Set enableUnifiedPipeline explicitly, It's now used for testing;
 * DO NOT USE IT !!!
 */
@property(nonatomic, assign) BOOL enableUnifiedPipeline;

/**
 * Enable SharedModule In LynxViewGroup
 */
@property(nonatomic, assign) BOOL enableSharedModule;

/** Only when enableBytecode is YES, it will take effect.
 * Set bytecode key for current lynxview.
 */
@property(nonatomic, strong, nullable) NSString* bytecodeUrl;

@property(nonatomic, nullable) id<LynxUIRendererCreatorProtocol> uiRendererCreator;

- (void)setThreadStrategyForRender:(LynxThreadStrategyForRender)threadStrategy;
- (LynxThreadStrategyForRender)getThreadStrategyForRender;

/**
 * embeddedMode is an experimental switch. When embeddedMode is set,
 * we offer optimal performance for embedded scenarios,
 * but it will restrict business flexibility.
 * For now, you can just ignore this switch.
 * Please DO NOT enable this switch on your own for now.
 * Contact the Lynx team for more information.
 */
- (void)setEmbeddedMode:(LynxEmbeddedMode)embeddedMode;
- (LynxEmbeddedMode)getEmbeddedMode;

- (void)addLynxResourceProvider:(NSString* _Nonnull)resType
                       provider:(id<LynxResourceProvider> _Nonnull)provider;

/**
 * You can use the following interface to create an alias for system fonts or registered fonts
 * (fontFamily) that can be used in Lynx style sheets. This is an instance level interface and can
 * override settings in the global interface.
 */
- (void)registerFont:(UIFont* _Nonnull)font forName:(NSString* _Nonnull)name;
- (void)registerFamilyName:(NSString* _Nonnull)fontFamilyName
             withAliasName:(NSString* _Nonnull)aliasName;

@end

#endif  // PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXBASECONFIGURATOR_H_
