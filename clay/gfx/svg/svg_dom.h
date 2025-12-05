// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SVG_SVG_DOM_H_
#define CLAY_GFX_SVG_SVG_DOM_H_

#include <memory>
#include <string>

#include "clay/gfx/rendering_backend.h"
#include "skity/include/skity/io/data.hpp"

namespace clay {
class SVGDom : public std::enable_shared_from_this<SVGDom> {
 public:
  using ImageCallback =
      std::function<std::shared_ptr<skity::Image>(std::string url)>;

  virtual ~SVGDom() = default;

  static std::shared_ptr<SVGDom> Create(std::shared_ptr<skity::Data> data);

  virtual std::shared_ptr<skity::Data> Render(int width, int height) = 0;
};
}  // namespace clay

#endif  // CLAY_GFX_SVG_SVG_DOM_H_
