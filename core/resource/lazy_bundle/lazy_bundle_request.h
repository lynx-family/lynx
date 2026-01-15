// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_REQUEST_H_
#define CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_REQUEST_H_

#include <memory>
#include <string>

#include "core/resource/lazy_bundle/bundle_resource_info.h"
#include "core/runtime/common/bindings/resource/response_promise.h"

namespace lynx {
namespace tasm {
namespace lazy_bundle {

/**
 * Hold info from request
 * TODO(zhoupeng.z): move all infos into this struct, like callback_id,
 * instance_id
 */
struct LynxLazyBundleRequest {
  std::string url;
  pub::LynxResourceType resource_type = pub::LynxResourceType::kLazyBundle;
  std::shared_ptr<runtime::ResponsePromise<BundleResourceInfo>>
      response_promise = nullptr;
};

}  // namespace lazy_bundle
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_REQUEST_H_
