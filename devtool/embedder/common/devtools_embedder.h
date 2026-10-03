// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_COMMON_DEVTOOLS_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_COMMON_DEVTOOLS_EMBEDDER_H_

#include <functional>
#include <memory>

#include "core/public/devtool/lynx_devtool_proxy.h"
#include "core/public/devtool/lynx_inspector_owner.h"

namespace lynx {

namespace devtool {

namespace input {
class InputEventTarget;
}  // namespace input

class DevtoolsEmbedder {
 public:
  explicit DevtoolsEmbedder(devtool::LynxDevToolProxy* proxy);
  virtual ~DevtoolsEmbedder() = default;

  devtool::LynxInspectorOwner* GetInspectorOwner() { return owner_.get(); }
  void SetInputEventTarget(
      const std::shared_ptr<input::InputEventTarget>& target);

 private:
  std::shared_ptr<devtool::LynxInspectorOwner> owner_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_COMMON_DEVTOOLS_EMBEDDER_H_
