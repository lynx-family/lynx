// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/platform/cursor_types.h"

#include "clay/net/url/url_helper.h"

namespace clay {

#define ADDTOMAP(name, type) \
  { name, type }

const std::map<std::string, CursorTypes> CursorTypeUtil::cursor_type_map_ = {
    ADDTOMAP("auto", CursorTypes::kAuto),
    ADDTOMAP("default", CursorTypes::kBasic),
    ADDTOMAP("none", CursorTypes::kNone),
    ADDTOMAP("context-menu", CursorTypes::kContextmenu),
    ADDTOMAP("help", CursorTypes::kHelp),
    ADDTOMAP("pointer", CursorTypes::kClick),
    ADDTOMAP("progress", CursorTypes::kProgress),
    ADDTOMAP("wait", CursorTypes::kWait),
    ADDTOMAP("cell", CursorTypes::kCell),
    ADDTOMAP("crosshair", CursorTypes::kPrecise),
    ADDTOMAP("text", CursorTypes::kText),
    ADDTOMAP("vertical-text", CursorTypes::kVerticaltext),
    ADDTOMAP("alias", CursorTypes::kAlias),
    ADDTOMAP("copy", CursorTypes::kSystemmousecursor),
    ADDTOMAP("move", CursorTypes::kMove),
    ADDTOMAP("no-drop", CursorTypes::kNodrop),
    ADDTOMAP("not-allowed", CursorTypes::kForbidden),
    ADDTOMAP("grab", CursorTypes::kGrab),
    ADDTOMAP("grabbing", CursorTypes::kGrabbing),
    ADDTOMAP("all-scroll", CursorTypes::kAllscroll),
    ADDTOMAP("col-resize", CursorTypes::kResizecolumn),
    ADDTOMAP("row-resize", CursorTypes::kResizerow),
    ADDTOMAP("n-resize", CursorTypes::kResizeup),
    ADDTOMAP("e-resize", CursorTypes::kResizeright),
    ADDTOMAP("s-resize", CursorTypes::kResizedown),
    ADDTOMAP("w-resize", CursorTypes::kResizeleft),
    ADDTOMAP("ne-resize", CursorTypes::kResizeupright),
    ADDTOMAP("nw-resize", CursorTypes::kResizeupleft),
    ADDTOMAP("se-resize", CursorTypes::kResizedownright),
    ADDTOMAP("sw-resize", CursorTypes::kResizedownleft),
    ADDTOMAP("ew-resize", CursorTypes::kResizeleftright),
    ADDTOMAP("ns-resize", CursorTypes::kResizeupdown),
    ADDTOMAP("nesw-resize", CursorTypes::kResizeuprightdownleft),
    ADDTOMAP("nwse-resize", CursorTypes::kResizeupleftdownright),
    ADDTOMAP("zoom-in", CursorTypes::kZoomin),
    ADDTOMAP("zoom-out", CursorTypes::kZoomout),
};

CursorTypes CursorTypeUtil::ParseCursorType(const std::string& str) {
  auto iter = cursor_type_map_.find(str);
  if (iter != cursor_type_map_.end()) {
    return iter->second;
  }

  // network url or local file paths
  url::UriSchemeType type = url::ParseUriScheme(str);
  if (type == url::UriSchemeType::kNet) {
    return CursorTypes::kNet;
  }

  return CursorTypes::kFile;
}
}  // namespace clay
