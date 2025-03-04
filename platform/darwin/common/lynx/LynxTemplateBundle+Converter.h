// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_CONVERTER_H_
#define DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_CONVERTER_H_

#import "LynxTemplateBundle.h"
#include "core/template_bundle/lynx_template_bundle.h"

std::shared_ptr<lynx::tasm::LynxTemplateBundle> LynxGetRawTemplateBundle(
    LynxTemplateBundle* bundle);

LynxTemplateBundle* ConstructTemplateBundleFromNative(
    lynx::tasm::LynxTemplateBundle bundle);

void LynxSetRawTemplateBundle(LynxTemplateBundle* bundle,
                              lynx::tasm::LynxTemplateBundle* raw_bundle);

#endif  // DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_CONVERTER_H_
