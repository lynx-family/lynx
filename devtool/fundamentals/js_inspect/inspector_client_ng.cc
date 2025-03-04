// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/fundamentals/js_inspect/inspector_client_ng.h"

#include "devtool/fundamentals/js_inspect/inspector_client_delegate.h"

namespace lynx {
namespace devtool {

void InspectorClientNG::SendResponse(const std::string &message,
                                     int instance_id) {
  auto sp = delegate_wp_.lock();
  if (sp != nullptr) {
    sp->SendResponse(message, instance_id);
  }
}

}  // namespace devtool
}  // namespace lynx
