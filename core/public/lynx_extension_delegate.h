// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/*
 * This is a experimental API, it is unstable and may break at any time.
 */

#ifndef CORE_PUBLIC_LYNX_EXTENSION_DELEGATE_H_
#define CORE_PUBLIC_LYNX_EXTENSION_DELEGATE_H_

#include <memory>

#include "core/public/jsb/native_module_factory.h"
#include "core/public/runtime_lifecycle_observer.h"

namespace lynx {
namespace shell {
template <typename T>
class LynxActor;
class BTSRuntime;
}  // namespace shell

namespace pub {

class LynxExtensionDelegate {
 public:
  virtual ~LynxExtensionDelegate() = default;
  virtual void OnDevicePixelRatioChanged(float device_pixel_ratio){};
  virtual void SetRuntimeActor(
      std::shared_ptr<shell::LynxActor<shell::BTSRuntime>> actor) = 0;
};

}  // namespace pub
}  // namespace lynx

#endif  // CORE_PUBLIC_LYNX_EXTENSION_DELEGATE_H_
