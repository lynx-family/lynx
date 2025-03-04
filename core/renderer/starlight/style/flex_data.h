// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_STYLE_FLEX_DATA_H_
#define CORE_RENDERER_STARLIGHT_STYLE_FLEX_DATA_H_

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace starlight {

class FlexData : public fml::RefCountedThreadSafeStorage {
 public:
  void ReleaseSelf() const override { delete this; }
  static fml::RefPtr<FlexData> Create() {
    return fml::AdoptRef(new FlexData());
  }
  fml::RefPtr<FlexData> Copy() const {
    return fml::AdoptRef(new FlexData(*this));
  }
  FlexData();
  FlexData(const FlexData& data);
  ~FlexData() = default;
  void Reset();
  float flex_grow_;
  float flex_shrink_;
  NLength flex_basis_;
  FlexDirectionType flex_direction_;
  FlexWrapType flex_wrap_;
  JustifyContentType justify_content_;
  FlexAlignType align_items_;
  FlexAlignType align_self_;
  AlignContentType align_content_;
  float order_;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_STYLE_FLEX_DATA_H_
