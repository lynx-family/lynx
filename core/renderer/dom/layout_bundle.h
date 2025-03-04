// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_LAYOUT_BUNDLE_H_
#define CORE_RENDERER_DOM_LAYOUT_BUNDLE_H_

#include <list>
#include <memory>
#include <utility>

#include "core/public/prop_bundle.h"
#include "core/renderer/css/computed_css_style.h"

namespace lynx {

namespace tasm {

struct LayoutBundle {
  base::String tag;

  bool is_create_bundle{false};
  bool is_root{false};
  bool allow_inline{false};
  // At present, there is a situation where the Element only needs to mark the
  // layout node dirty, without any property/style/attr updates. In this case,
  // the layout context needs to consume this flag, marking the corresponding
  // layout node dirty.
  bool is_dirty{false};

  std::shared_ptr<PropBundle> shadownode_prop_bundle;
  std::list<std::shared_ptr<PropBundle>> update_prop_bundles{};
  std::list<CSSPropertyID> reset_styles{};
  std::list<std::pair<CSSPropertyID, CSSValue>> styles{};
  std::list<std::pair<starlight::LayoutAttribute, lepus::Value>> attrs{};

  double font_scale{-1};
  double cur_node_font_size{-1};
  double root_node_font_size{-1};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_LAYOUT_BUNDLE_H_
