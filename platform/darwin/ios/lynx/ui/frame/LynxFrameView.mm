// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxFrameView.h>

#import <Lynx/LynxFrameShadowNode.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxViewEnum.h>
#import "LynxTraceEventDef.h"
#import "LynxUIRendererProtocol.h"

#include "base/trace/native/trace_defines.h"
#include "base/trace/native/trace_event.h"

#define IS_ZERO(num) ((num) < 1e-5)

#pragma mark - LynxFrameView

@implementation LynxFrameView {
  LynxTemplateRender *_render;
  __weak UIView<LUIBodyView> *_rootView;
  NSString *_url;
  BOOL _isChildLynxPage;
  CGSize _intrinsicContentSize;
  BOOL _isBundleLoad;
  CGRect _contentRect;
  BOOL _isIntrinsicSizeConsumed;
  LynxTemplateData *_initData;
  LynxTemplateData *_globalProps;
  LynxViewSizeMode _widthMode;
  LynxViewSizeMode _heightMode;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _isIntrinsicSizeConsumed = YES;
    _contentRect = CGRectNull;
  }
  return self;
}

- (void)initWithRootView:(UIView<LUIBodyView> *)rootView {
  if ([rootView isKindOfClass:[LynxView class]]) {
    _rootView = rootView;
  } else if ([rootView isKindOfClass:[LynxFrameView class]]) {
    _rootView = [(LynxFrameView *)rootView getRootView];
  }
  _render = [[LynxTemplateRender alloc] initWithBuilderBlock:[rootView getLynxViewBuilderBlock]
                                               containerView:self];
}

- (void)setAppBundle:(LynxTemplateBundle *)bundle {
  [_render updateViewport];

  LynxLoadMeta *loadMeta = [[LynxLoadMeta alloc] init];
  loadMeta.url = _url;
  loadMeta.templateBundle = bundle;
  if (_initData) {
    loadMeta.initialData = _initData;
    _initData = nil;
  }
  if (_globalProps) {
    loadMeta.globalProps = _globalProps;
    _globalProps = nil;
  }
  [_render loadTemplate:loadMeta];
  _isBundleLoad = YES;
}

- (void)updateFrame:(CGRect)frame contentFrame:(CGRect)contentFrame {
  [super setFrame:frame];
  if (!CGRectEqualToRect(contentFrame, _contentRect)) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_FRAME_VIEW_UPDATE_LAYOUT, "widthMeasureSpec",
                std::to_string(contentFrame.size.width).c_str(), "heightMeasureSpec",
                std::to_string(contentFrame.size.height).c_str());
    if (!_isBundleLoad || CGRectIsNull(_contentRect)) {
      _widthMode =
          IS_ZERO(contentFrame.size.width) ? LynxViewSizeModeUndefined : LynxViewSizeModeExact;
      _heightMode =
          IS_ZERO(contentFrame.size.height) ? LynxViewSizeModeUndefined : LynxViewSizeModeExact;
      [_render setLayoutWidthMode:_widthMode];
      [_render setLayoutHeightMode:_heightMode];
    }
    [_render setPreferredLayoutWidth:contentFrame.size.width];
    [_render setPreferredLayoutHeight:contentFrame.size.height];
    _contentRect = contentFrame;
    [self setNeedsLayout];
  }
}

- (void)setInitData:(nullable LynxTemplateData *)initData {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_FRAME_VIEW_SET_INIT_DATA);
  _initData = initData;
}

- (void)setGlobalProps:(nullable LynxTemplateData *)globalProps {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_FRAME_VIEW_SET_GLOBAL_PROPS);
  _globalProps = globalProps;
}

- (void)propsDidUpdate {
  if (!_isBundleLoad) {
    return;
  }
  if (!_initData && !_globalProps) {
    return;
  }

  LynxUpdateMeta *updateMeta = [[LynxUpdateMeta alloc] init];
  [updateMeta setData:_initData];
  [updateMeta setGlobalProps:_globalProps];
  [_render updateMetaData:updateMeta];

  _initData = nil;
  _globalProps = nil;
}

- (void)setUrl:(NSString *)url {
  _url = url;
}

- (UIView<LUIBodyView> *_Nullable)getRootView {
  return _rootView;
}

- (void)dealloc {
  if (_render) {
    _LogI(@"LynxFrameView %p: destroy", self);
    TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_FRAME_VIEW_DESTROY);
    [_render.lynxUIRenderer reset];
    _render = nil;
  }
}

// TODO(zhoupeng.z): implement following methods, some of them are useless for LynxFrameView.
// Optimize it later

#pragma mark - LUIErrorHandling

- (void)didReceiveResourceError:(LynxError *_Nullable)error
                     withSource:(NSString *_Nullable)resourceUrl
                           type:(NSString *_Nullable)type {
}

- (void)reportError:(nonnull NSError *)error {
}

- (void)reportLynxError:(LynxError *_Nullable)error {
}

#pragma mark - LUIBodyView

- (BOOL)enableAsyncDisplay {
  return NO;
}

- (void)setEnableAsyncDisplay:(BOOL)enableAsyncDisplay {
}

- (NSString *)url {
  return _url;
}

- (int32_t)instanceId {
  return -1;
}

- (void)sendGlobalEvent:(nonnull NSString *)name withParams:(nullable NSArray *)params {
  [_render sendGlobalEvent:name withParams:params];
}

- (void)setIntrinsicContentSize:(CGSize)size {
  if (!CGSizeEqualToSize(_intrinsicContentSize, size)) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_FRAME_VIEW_SET_INTRINSIC_CONTENT_SIZE, "width",
                std::to_string(size.width).c_str(), "height", std::to_string(size.height).c_str());
    _intrinsicContentSize = size;
    _isIntrinsicSizeConsumed = NO;
    [self.context
        findShadowNodeAndRunTask:self.sign
                            task:^(LynxShadowNode *node) {
                              [(LynxFrameShadowNode *)node updateIntrinsicContentSize:size];
                            }];
    [self setNeedsLayout];
  }
}

- (void)layoutSubviews {
  if (!_isBundleLoad) {
    [super layoutSubviews];
    return;
  }
  CGRect targetRect = _contentRect;
  if (!_isIntrinsicSizeConsumed) {
    if (_widthMode != LynxViewSizeModeExact) {
      targetRect.size.width = _intrinsicContentSize.width;
    }
    if (_heightMode != LynxViewSizeModeExact) {
      targetRect.size.height = _intrinsicContentSize.height;
    }
    _isIntrinsicSizeConsumed = YES;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_FRAME_VIEW_ON_MEASURE_TARGET, "widthMeasureSpec",
              std::to_string(targetRect.size.width).c_str(), "heightMeasureSpec",
              std::to_string(targetRect.size.height).c_str());

  [_render setPreferredLayoutWidth:targetRect.size.width];
  [_render setPreferredLayoutHeight:targetRect.size.height];
  [_render updateViewport];
  [super layoutSubviews];
  [_render triggerLayoutInTick];
}

- (BOOL)enableTextNonContiguousLayout {
  return YES;
}

- (void)runOnTasmThread:(dispatch_block_t)task {
}

// TODO(zhoupeng.z):implement it by frame render
- (LynxThreadStrategyForRender)getThreadStrategyForRender {
  return LynxThreadStrategyForRenderAllOnUI;
}

- (void)setAttachLynxPageUICallback:(attachLynxPageUI _Nonnull)callback {
  [_render setAttachLynxPageUICallback:callback];
}

- (void)setIsChildLynxPage:(BOOL)isChildLynxPage {
  _isChildLynxPage = isChildLynxPage;
}

- (BOOL)isChildLynxPage {
  return _isChildLynxPage;
}

- (LynxViewBuilderBlock)getLynxViewBuilderBlock {
  return [_render getLynxViewBuilderBlock];
}

@end
