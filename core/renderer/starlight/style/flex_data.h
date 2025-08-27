// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_STYLE_FLEX_DATA_H_
#define CORE_RENDERER_STARLIGHT_STYLE_FLEX_DATA_H_

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/style/default_layout_style.h"
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
  FlexData() = default;
  LYNX_EXPORT FlexData(const FlexData& data);
  ~FlexData() = default;
  void Reset();

  NLength flex_basis_{DefaultLayoutStyle::SL_DEFAULT_FLEX_BASIS()};
  float flex_grow_{DefaultLayoutStyle::SL_DEFAULT_FLEX_GROW};
  float flex_shrink_{DefaultLayoutStyle::SL_DEFAULT_FLEX_SHRINK};
  float order_{DefaultLayoutStyle::SL_DEFAULT_ORDER};
  FlexDirectionType flex_direction_{
      DefaultLayoutStyle::SL_DEFAULT_FLEX_DIRECTION};
  FlexWrapType flex_wrap_{DefaultLayoutStyle::SL_DEFAULT_FLEX_WRAP};
  JustifyContentType justify_content_{
      DefaultLayoutStyle::SL_DEFAULT_JUSTIFY_CONTENT};
  FlexAlignType align_items_{DefaultLayoutStyle::SL_DEFAULT_ALIGN_ITEMS};
  FlexAlignType align_self_{DefaultLayoutStyle::SL_DEFAULT_ALIGN_SELF};
  AlignContentType align_content_{DefaultLayoutStyle::SL_DEFAULT_ALIGN_CONTENT};
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_STYLE_FLEX_DATA_H_
