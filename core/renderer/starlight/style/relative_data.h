// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_STYLE_RELATIVE_DATA_H_
#define CORE_RENDERER_STARLIGHT_STYLE_RELATIVE_DATA_H_

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace starlight {

class RelativeData : public fml::RefCountedThreadSafeStorage {
 public:
  void ReleaseSelf() const override { delete this; }
  static fml::RefPtr<RelativeData> Create() {
    return fml::AdoptRef(new RelativeData());
  }
  fml::RefPtr<RelativeData> Copy() const {
    return fml::AdoptRef(new RelativeData(*this));
  }
  RelativeData();
  RelativeData(const RelativeData& data);
  ~RelativeData() = default;
  void Reset();
  int relative_id_;
  int relative_align_top_, relative_align_right_, relative_align_bottom_,
      relative_align_left_;
  int relative_top_of_, relative_right_of_, relative_bottom_of_,
      relative_left_of_;
  bool relative_layout_once_;
  RelativeCenterType relative_center_;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_STYLE_RELATIVE_DATA_H_
