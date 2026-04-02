// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIRendererProtocol.h"
#if defined(__cplusplus)
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"
#endif

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIRenderer : NSObject <LynxUIRendererProtocol>

#if defined(__cplusplus)
- (void)onPageConfigUpdate:(const std::shared_ptr<lynx::tasm::PageConfig> &)pageConfig;
#endif

@end

NS_ASSUME_NONNULL_END
