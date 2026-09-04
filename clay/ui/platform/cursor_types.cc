// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/platform/cursor_types.h"

#include <algorithm>
#include <array>
#include <string_view>

#include "clay/net/url/url_helper.h"

namespace clay {

struct CursorTypeEntry {
  std::string_view name;
  CursorTypes type;
};

// Keep this table sorted by name for binary search.
constexpr std::array<CursorTypeEntry, 36> kCursorTypes = {{
    {"alias", CursorTypes::kAlias},
    {"all-scroll", CursorTypes::kAllscroll},
    {"auto", CursorTypes::kAuto},
    {"cell", CursorTypes::kCell},
    {"col-resize", CursorTypes::kResizecolumn},
    {"context-menu", CursorTypes::kContextmenu},
    {"copy", CursorTypes::kSystemmousecursor},
    {"crosshair", CursorTypes::kPrecise},
    {"default", CursorTypes::kBasic},
    {"e-resize", CursorTypes::kResizeright},
    {"ew-resize", CursorTypes::kResizeleftright},
    {"grab", CursorTypes::kGrab},
    {"grabbing", CursorTypes::kGrabbing},
    {"help", CursorTypes::kHelp},
    {"move", CursorTypes::kMove},
    {"n-resize", CursorTypes::kResizeup},
    {"ne-resize", CursorTypes::kResizeupright},
    {"nesw-resize", CursorTypes::kResizeuprightdownleft},
    {"no-drop", CursorTypes::kNodrop},
    {"none", CursorTypes::kNone},
    {"not-allowed", CursorTypes::kForbidden},
    {"ns-resize", CursorTypes::kResizeupdown},
    {"nw-resize", CursorTypes::kResizeupleft},
    {"nwse-resize", CursorTypes::kResizeupleftdownright},
    {"pointer", CursorTypes::kClick},
    {"progress", CursorTypes::kProgress},
    {"row-resize", CursorTypes::kResizerow},
    {"s-resize", CursorTypes::kResizedown},
    {"se-resize", CursorTypes::kResizedownright},
    {"sw-resize", CursorTypes::kResizedownleft},
    {"text", CursorTypes::kText},
    {"vertical-text", CursorTypes::kVerticaltext},
    {"w-resize", CursorTypes::kResizeleft},
    {"wait", CursorTypes::kWait},
    {"zoom-in", CursorTypes::kZoomin},
    {"zoom-out", CursorTypes::kZoomout},
}};

CursorTypes CursorTypeUtil::ParseCursorType(const std::string& str) {
  const auto iter = std::lower_bound(
      kCursorTypes.begin(), kCursorTypes.end(), std::string_view(str),
      [](const CursorTypeEntry& entry, std::string_view name) {
        return entry.name < name;
      });
  if (iter != kCursorTypes.end() && iter->name == str) {
    return iter->type;
  }

  // network url or local file paths
  url::UriSchemeType type = url::ParseUriScheme(str);
  if (type == url::UriSchemeType::kNet) {
    return CursorTypes::kNet;
  }

  return CursorTypes::kFile;
}
}  // namespace clay
