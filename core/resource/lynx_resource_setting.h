// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_LYNX_RESOURCE_SETTING_H_
#define CORE_RESOURCE_LYNX_RESOURCE_SETTING_H_

#include <memory>

namespace lynx {
namespace runtime {
namespace js {
class LynxResourceSetting {
 public:
  static std::shared_ptr<LynxResourceSetting> getInstance();

 public:
  bool is_debug_resource_ = false;
};

}  // namespace js

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RESOURCE_LYNX_RESOURCE_SETTING_H_
