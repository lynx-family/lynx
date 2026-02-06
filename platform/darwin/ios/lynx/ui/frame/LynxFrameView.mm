// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxFrameView.h>

#import <Lynx/LynxFrameShadowNode.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxUIContext.h>
#import "LynxTraceEventDef.h"
#import "LynxUIRendererProtocol.h"

#include "base/trace/native/trace_defines.h"
#include "base/trace/native/trace_event.h"

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
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _isIntrinsicSizeConsumed = YES;
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
    _contentRect = contentFrame;
    [self setNeedsLayout];
  }
}

- (void)setInitData:(nullable LynxTemplateData *)initData {
  _initData = initData;
}

- (void)setGlobalProps:(nullable LynxTemplateData *)globalProps {
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
}

- (void)setIntrinsicContentSize:(CGSize)size {
  if (!CGSizeEqualToSize(_intrinsicContentSize, size)) {
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
    targetRect = CGRectMake(0.f, 0.f, _intrinsicContentSize.width, _intrinsicContentSize.height);
    _isIntrinsicSizeConsumed = YES;
  }

  [_render updateFrame:targetRect];
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
