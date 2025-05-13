// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

NS_ASSUME_NONNULL_BEGIN

/**
 * hold necessary members for LynxTemplateRender and LynxFrameRender to setup
 */
@interface LynxTemplateRenderContext : NSObject

@end

@protocol LynxTemplateRenderContextProtocol <NSObject>

- (LynxTemplateRenderContext*)getRenderContext;

- (void)attachRenderContext:(LynxTemplateRenderContext*)context;

@end

NS_ASSUME_NONNULL_END
