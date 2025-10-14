// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_RESOURCE_TYPE_H_
#define CLAY_NET_RESOURCE_TYPE_H_

#include <cstring>
#include <memory>
#include <utility>

namespace clay {
struct RawResource {
  size_t length = 0u;
  // TODO(hanhaoshen): use std::shared_ptr<uint8_t[]>;
  std::shared_ptr<const uint8_t> data = nullptr;

  static RawResource MakeWithCopy(const uint8_t* data, const size_t length) {
    if (data == nullptr || length == 0) {
      return {0, nullptr};
    }
    auto response = std::shared_ptr<uint8_t>(new uint8_t[length],
                                             std::default_delete<uint8_t[]>());
    ::memcpy(reinterpret_cast<void*>(response.get()), data, length);
    return {length, std::move(response)};
  }
};

// align to lynx resource type.
enum class ResourceType : int8_t {
  kOthers = -1,
  kImage,
  kFont,
  kLottie,
  kVideo,
  kSvg,
  kTemplate,
  kLynxCoreJs,
  kDynamicComponent,
  kI18nText,
  kExternalJs
};

}  // namespace clay

#endif  // CLAY_NET_RESOURCE_TYPE_H_
