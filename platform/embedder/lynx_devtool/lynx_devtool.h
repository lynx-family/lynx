// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_DEVTOOL_LYNX_DEVTOOL_H_
#define PLATFORM_EMBEDDER_LYNX_DEVTOOL_LYNX_DEVTOOL_H_

#include <memory>

#include "core/public/devtool/lynx_inspector_owner.h"

namespace lynx {
namespace devtool {
namespace input {
class InputEventTarget;
}
}  // namespace devtool
namespace embedder {

// Owns the inspector attachment and detaches its proxy on destruction.
class EmbedderDevTool {
 public:
  virtual ~EmbedderDevTool() = default;
  // Returns a non-null borrowed owner valid until this instance is destroyed.
  virtual devtool::LynxInspectorOwner* GetInspectorOwner() = 0;
  virtual void SetInputEventTarget(
      const std::shared_ptr<devtool::input::InputEventTarget>& target) = 0;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_DEVTOOL_LYNX_DEVTOOL_H_
