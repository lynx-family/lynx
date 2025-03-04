// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_STYLE_BOX_DATA_H_
#define CORE_RENDERER_STARLIGHT_STYLE_BOX_DATA_H_

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace starlight {

class BoxData : public fml::RefCountedThreadSafeStorage {
 public:
  void ReleaseSelf() const override { delete this; }
  static fml::RefPtr<BoxData> Create() { return fml::AdoptRef(new BoxData()); }
  fml::RefPtr<BoxData> Copy() const {
    return fml::AdoptRef(new BoxData(*this));
  }

  BoxData();
  BoxData(const BoxData& other);
  ~BoxData() = default;
  void Reset();
  NLength width_;
  NLength height_;
  NLength min_width_;
  NLength max_width_;
  NLength min_height_;
  NLength max_height_;
  float aspect_ratio_;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_STYLE_BOX_DATA_H_
