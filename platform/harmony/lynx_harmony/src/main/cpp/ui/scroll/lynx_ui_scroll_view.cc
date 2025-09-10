// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/lynx_ui_scroll_view.h"

#include <utility>

#include "core/renderer/dom/lynx_get_ui_result.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {

LynxUIScrollView::LynxUIScrollView(LynxContext* context, int sign,
                                   const std::string& tag)
    : LynxUIScrollViewInternal(context, sign, tag) {}

void LynxUIScrollView::OnPropUpdate(const std::string& name,
                                    const lepus::Value& value) {
  if (auto it = prop_setters_.find(name); it != prop_setters_.end()) {
    PropSetter setter = it->second;
    (this->*setter)(value);
  } else {
    LynxUIScrollViewInternal::OnPropUpdate(name, value);
  }
}

void LynxUIScrollView::InvokeMethod(
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (auto it = ui_method_map_.find(method); it != ui_method_map_.end()) {
    (this->*it->second)(args, std::move(callback));
  } else {
    LynxUIScrollViewInternal::InvokeMethod(method, args, std::move(callback));
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
