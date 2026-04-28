// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_CONTENT_DATA_H_
#define CORE_STYLE_CONTENT_DATA_H_

#include "base/include/value/base_value.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace starlight {
struct ContentData {
  ContentData() : type(ContentType::kInvalid) {}

  ContentType type;
  base::String content_data;

  bool operator==(const ContentData& rhs) const {
    return type == rhs.type && content_data == rhs.content_data;
  }
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_CONTENT_DATA_H_
