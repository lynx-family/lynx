// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/svg/svg_dom.h"

namespace clay {
namespace {
class FallbackSVGDom : public SVGDom {
 public:
  std::shared_ptr<skity::Image> Render(
      int width, int height, fml::RefPtr<GPUUnrefQueue> unref_queue) override {
    return nullptr;
  }
};
}  // namespace

std::shared_ptr<SVGDom> SVGDom::Create(std::shared_ptr<skity::Data> data,
                                       ImageCallback callback) {
  return std::make_shared<FallbackSVGDom>();
}
}  // namespace clay
