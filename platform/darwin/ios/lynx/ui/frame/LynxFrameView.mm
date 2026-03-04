// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxFrameView.h>

#import <Lynx/LynxFrameShadowNode.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxViewBuilder.h>
#import <Lynx/LynxViewEnum.h>
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
  LynxEmbeddedMode _embeddedMode;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _isIntrinsicSizeConsumed = YES;
    _embeddedMode = LynxEmbeddedModeUnset;
  }
  return self;
}

- (void)initWithRootView:(UIView<LUIBodyView> *)rootView {
  if ([rootView isKindOfClass:[LynxView class]]) {
    _rootView = rootView;
  } else if ([rootView isKindOfClass:[LynxFrameView class]]) {
    _rootView = [(LynxFrameView *)rootView getRootView];
  }
}

- (void)setAppBundle:(LynxTemplateBundle *)bundle {
  [self ensureRenderCreated];

  LynxLoadMeta *loadMeta = [self buildLoadMetaWithBundle:bundle];
  [_render loadTemplate:loadMeta];
  _isBundleLoad = YES;
}

- (void)ensureRenderCreated {
  if (_render) {
    return;
  }

  __weak typeof(self) weakSelf = self;
  UIView<LUIBodyView> *rootView = _rootView;
  LynxViewBuilderBlock originalBlock = [rootView getLynxViewBuilderBlock];
  _render = [[LynxTemplateRender alloc]
      initWithBuilderBlock:^(LynxViewBuilder *builder) {
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (originalBlock) {
          originalBlock(builder);
        }
        [builder setEnablePreUpdateData:YES];
        if (strongSelf) {
          [builder setEmbeddedMode:strongSelf->_embeddedMode];
        }
      }
             containerView:self];
}

- (LynxLoadMeta *)buildLoadMetaWithBundle:(LynxTemplateBundle *)bundle {
  LynxLoadMeta *loadMeta = [[LynxLoadMeta alloc] init];
  loadMeta.url = _url;
  loadMeta.templateBundle = bundle;
  loadMeta.initialData = _initData;
  loadMeta.globalProps = _globalProps;
  _initData = nil;
  _globalProps = nil;
  return loadMeta;
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
  if (!_isBundleLoad || !_render) {
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

- (void)setEmbeddedMode:(LynxEmbeddedMode)embeddedMode {
  if (_embeddedMode == LynxEmbeddedModeUnset) {
    _embeddedMode = embeddedMode;
  }
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
  if (!_isBundleLoad || !_render) {
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
  if (!_render) {
    return;
  }

  [_render setAttachLynxPageUICallback:callback];
}

- (void)setIsChildLynxPage:(BOOL)isChildLynxPage {
  _isChildLynxPage = isChildLynxPage;
}

- (BOOL)isChildLynxPage {
  return _isChildLynxPage;
}

- (LynxViewBuilderBlock)getLynxViewBuilderBlock {
  if (!_render) {
    return nil;
  }

  return [_render getLynxViewBuilderBlock];
}

@end
