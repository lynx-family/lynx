// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxFrameRender.h>

#import <Lynx/LynxTemplateRenderContext.h>
#import <Lynx/TemplateRenderCallbackProtocol.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxFrameRender () <TemplateRenderCallbackProtocol, LynxTemplateRenderContextProtocol>

@end

NS_ASSUME_NONNULL_END
