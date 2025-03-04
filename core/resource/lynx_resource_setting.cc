// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lynx_resource_setting.h"

namespace lynx {
namespace piper {

std::shared_ptr<LynxResourceSetting> LynxResourceSetting::getInstance() {
  static std::shared_ptr<LynxResourceSetting> instance =
      std::make_shared<LynxResourceSetting>();
  return instance;
}

}  // namespace piper
}  // namespace lynx
