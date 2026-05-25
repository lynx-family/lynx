// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/bounce_view/bounce_view.h"

#include <memory>
#include <string>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/rendering/render_bounce.h"
#include "clay/ui/rendering/render_container.h"

namespace clay {

BounceView::BounceView(int id, PageView* page_view)
    : WithTypeInfo(id, "BounceView", std::make_unique<RenderBounce>(),
                   page_view) {}

void BounceView::SetAttribute(const char* attr, const clay::Value& value) {
  auto kw = GetKeywordID(attr);
  if (kw == KeywordID::kDirection) {
    std::string direction;
    attribute_utils::TryGetString(value, direction, "direction");
    if (direction.compare("right") == 0) {
      SetBounceDirection(Direction::kRight);
    } else if (direction.compare("left") == 0) {
      SetBounceDirection(Direction::kLeft);
    } else if (direction.compare("bottom") == 0) {
      SetBounceDirection(Direction::kBottom);
    } else if (direction.compare("top") == 0) {
      SetBounceDirection(Direction::kTop);
    } else {
      FML_DCHECK(false);
    }
    return;
  }
  BaseView::SetAttribute(attr, value);
}

}  // namespace clay
