// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_LEPUS_IOS_LYNX_LEPUS_MODULE_DARWIN_H_
#define CORE_RUNTIME_BINDINGS_LEPUS_IOS_LYNX_LEPUS_MODULE_DARWIN_H_

#include <string>

#include "core/runtime/vm/lepus/lepus_value.h"

@protocol TemplateRenderCallbackProtocol;

namespace lynx {
namespace piper {

extern lepus::Value TriggerLepusMethod(const std::string& js_method_name, const lepus::Value& args,
                                       id<TemplateRenderCallbackProtocol> render);

void TriggerLepusMethodAsync(const std::string& js_method_name, const lepus::Value& args,
                             id<TemplateRenderCallbackProtocol> render);
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_LEPUS_IOS_LYNX_LEPUS_MODULE_DARWIN_H_
