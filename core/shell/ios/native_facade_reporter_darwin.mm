// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/shell/ios/native_facade_reporter_darwin.h"

#include "core/renderer/dom/ios/lepus_value_converter.h"

namespace lynx {
namespace shell {

void NativeFacadeReporterDarwin::OnPerformanceEvent(const lepus::Value& entry) {
  __strong id<TemplateRenderCallbackProtocol> render = _render;
  [render onPerformanceEvent:lynx::tasm::convertLepusValueToNSObject(entry)];
}

}  // namespace shell
}  // namespace lynx
