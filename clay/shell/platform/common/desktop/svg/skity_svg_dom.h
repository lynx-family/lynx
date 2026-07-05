// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_COMMON_DESKTOP_SVG_SKITY_SVG_DOM_H_
#define CLAY_SHELL_PLATFORM_COMMON_DESKTOP_SVG_SKITY_SVG_DOM_H_

#include <memory>
#include <string>

#include "clay/gfx/svg/svg_dom.h"

namespace serval {
namespace svg {
namespace parser {
class SrSVGDOM;
}
}  // namespace svg
}  // namespace serval

namespace clay {

class SkitySVGDom : public SVGDom {
 public:
  SkitySVGDom(std::shared_ptr<skity::Data> data,
              SVGDom::ImageCallback callback);
  ~SkitySVGDom() override;

  std::shared_ptr<skity::Image> Render(
      int width, int height, fml::RefPtr<GPUUnrefQueue> unref_queue) override;

 private:
  std::shared_ptr<skity::Data> data_;
  std::unique_ptr<serval::svg::parser::SrSVGDOM> svg_dom_;
  SVGDom::ImageCallback callback_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_COMMON_DESKTOP_SVG_SKITY_SVG_DOM_H_
