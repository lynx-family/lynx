// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxFrameRender.h>

#import <Lynx/LynxBaseTemplateRender+Internal.h>
#import <Lynx/LynxUIRenderer.h>
#import <Lynx/TemplateRenderCallbackProtocol.h>

#pragma mark - LynxFrameRender

@implementation LynxFrameRender {
  __weak UIView<LUIBodyView> *_containerView;
}

- (instancetype)init {
  if (self = [super init]) {
    // TODO(zhoupeng.z): get context and screenSize from root view
    [self setUpWithBuilder:nil screenSize:[UIScreen mainScreen].bounds.size];
  }
  return self;
}

- (void)dealloc {
  [_lynxUIRenderer reset];
  // ios block cannot capture std::unique_ptr, tricky...
  auto *shell = shell_.release();
  // FrameView maybe release in main flow of FrameRender,
  // so need just release LynxShell delay, avoid crash
  dispatch_async(dispatch_get_main_queue(), ^{
    delete shell;
  });
}

- (UIView<LUIBodyView> *)containerView {
  return _containerView;
}

#pragma mark - LynxErrorReceiverProtocol

- (void)onErrorOccurred:(LynxError *)error {
}

#pragma mark - TemplateRenderCallbackProtocol

// TODO(zhoupeng.z): implement TemplateRenderCallbackProtocol, some of them are useless for
// LynxFrameView.

@end
