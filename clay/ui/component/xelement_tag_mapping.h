// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_XELEMENT_TAG_MAPPING_H_
#define CLAY_UI_COMPONENT_XELEMENT_TAG_MAPPING_H_

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

namespace clay {

inline std::string ResolveXElementTag(const std::string& tag) {
  struct TagMapping {
    std::string_view tag;
    std::string_view resolved_tag;
  };
  static constexpr std::array<TagMapping, 14> kTagMap = {{
      {"blur-view", "x-blur-view"},
      {"input", "x-input-ng"},
      {"overlay", "x-overlay-ng"},
      {"refresh", "x-refresh-view"},
      {"refresh-header", "x-refresh-header"},
      {"textarea", "x-textarea-ng"},
      {"viewpager", "x-viewpager-ng"},
      {"viewpager-item", "x-viewpager-item-ng"},
      {"webview", "x-webview"},
      {"x-foldview-header-ng", "scroll-coordinator-header"},
      {"x-foldview-ng", "scroll-coordinator"},
      {"x-foldview-slot-drag-ng", "scroll-coordinator-slot-drag"},
      {"x-foldview-slot-ng", "scroll-coordinator-slot"},
      {"x-foldview-toolbar-ng", "scroll-coordinator-toolbar"},
  }};
  const std::string_view tag_view(tag);
  const auto it =
      std::lower_bound(kTagMap.begin(), kTagMap.end(), tag_view,
                       [](const TagMapping& mapping, std::string_view value) {
                         return mapping.tag < value;
                       });
  return it == kTagMap.end() || it->tag != tag_view
             ? tag
             : std::string(it->resolved_tag);
}

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_XELEMENT_TAG_MAPPING_H_
