// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIRendererProtocol.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIRenderer : NSObject <LynxUIRendererProtocol>

- (void)onPageConfigUpdate:(const std::shared_ptr<lynx::tasm::PageConfig> &)pageConfig;

@end

NS_ASSUME_NONNULL_END
