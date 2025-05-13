// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRuntimeLifecycleListener.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxTemplateRenderDelegate.h>
#import <Lynx/LynxViewEnum.h>
#import <Lynx/TemplateRenderCallbackProtocol.h>

#include <memory>

#include "core/renderer/ui_wrapper/painting/ios/ui_delegate_darwin.h"
#include "core/runtime/bindings/jsi/modules/ios/module_factory_darwin.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"

@class LynxTemplateData;
@class LynxEventHandler;
@class LynxEventEmitter;
@class LynxKeyboardEventDispatcher;
@class LynxTheme;
@class LynxSSRHelper;
@class LynxView;
@class LynxViewBuilder;
typedef NS_ENUM(NSInteger, LynxBooleanOption);

NS_ASSUME_NONNULL_BEGIN

@interface LynxTemplateRender () <TemplateRenderCallbackProtocol> {
 @protected
  BOOL _enableAsyncDisplayFromNative;
  BOOL _enableImageDownsampling;
  BOOL _enableTextNonContiguousLayout;

  BOOL _hasStartedLoad;
  BOOL _enableLayoutSafepoint;
  BOOL _enableAutoExpose;
  BOOL _enableAirStrictMode;

  LynxTheme* _localTheme;
  LynxTemplateData* _globalProps;
  LynxSSRHelper* _lynxSSRHelper;
  CGSize _intrinsicContentSize;
  std::shared_ptr<lynx::tasm::PageConfig> pageConfig_;
  // property
  NSMutableDictionary<NSString*, id>* _originLynxViewConfig;
  long _initStartTiming;
  long _initEndTiming;
  __weak LynxView* _lynxView;

  __weak id<LynxTemplateRenderDelegate> _delegate;

  LynxViewSizeMode _layoutWidthMode;
  LynxViewSizeMode _layoutHeightMode;
  CGFloat _preferredMaxLayoutWidth;
  CGFloat _preferredMaxLayoutHeight;
  CGFloat _preferredLayoutWidth;
  CGFloat _preferredLayoutHeight;
  CGRect _frameOfLynxView;
  BOOL _isDestroyed;
  BOOL _hasRendered;
  NSString* _url;
  BOOL _enablePrePainting;
  BOOL _enableDumpElement;
  BOOL _enableRecycleTemplateBundle;

  BOOL _enableGenericResourceFetcher;
}

- (lynx::piper::ModuleFactoryDarwin*)getModuleFactory;

@end

NS_ASSUME_NONNULL_END
