// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_XELEMENT_TAG_MAPPING_H_
#define CLAY_UI_COMPONENT_XELEMENT_TAG_MAPPING_H_

#include <string>
#include <unordered_map>

namespace clay {

inline std::string ResolveXElementTag(const std::string& tag) {
  static const std::unordered_map<std::string, std::string> kTagMap = {
      {"viewpager", "x-viewpager-ng"},
      {"viewpager-item", "x-viewpager-item-ng"},
      {"webview", "x-webview"},
      {"overlay", "x-overlay-ng"},
      {"refresh", "x-refresh-view"},
      {"refresh-header", "x-refresh-header"},
      {"blur-view", "x-blur-view"},
      {"x-foldview-ng", "scroll-coordinator"},
      {"x-foldview-header-ng", "scroll-coordinator-header"},
      {"x-foldview-slot-ng", "scroll-coordinator-slot"},
      {"x-foldview-slot-drag-ng", "scroll-coordinator-slot-drag"},
      {"x-foldview-toolbar-ng", "scroll-coordinator-toolbar"},
      {"input", "x-input-ng"},
      {"textarea", "x-textarea-ng"},
  };
  auto it = kTagMap.find(tag);
  return it == kTagMap.end() ? tag : it->second;
}

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_XELEMENT_TAG_MAPPING_H_
