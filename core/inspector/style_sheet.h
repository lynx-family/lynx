// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INSPECTOR_STYLE_SHEET_H_
#define CORE_INSPECTOR_STYLE_SHEET_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lynx {
namespace devtool {

using LynxDoubleMapString =
    std::unordered_map<std::string,
                       std::unordered_map<std::string, std::string>>;

using LynxTripleMapString = std::unordered_map<
    std::string,
    std::unordered_map<std::string,
                       std::unordered_map<std::string, std::string>>>;

using LynxAttributePair =
    std::pair<std::vector<std::string>,
              std::unordered_map<std::string, std::string>>;

constexpr intptr_t kElementPtr = -1;

struct Range {
  int start_line_;
  int end_line_;
  int start_column_;
  int end_column_;
};

struct CSSPropertyDetail {
  std::string name_;
  std::string value_;
  std::string text_;
  bool disabled_;
  bool implicit_;
  bool important_;
  bool looped_;
  bool parsed_ok_;
  Range property_range_;
};

struct InspectorStyleSheet {
  std::string style_sheet_id_;
  std::string style_name_;
  std::string origin_;
  std::string css_text_;
  std::unordered_multimap<std::string, CSSPropertyDetail> css_properties_;
  std::unordered_map<std::string, CSSPropertyDetail> shorthand_entries_;
  std::vector<std::string> property_order_;
  Range style_value_range_;
  Range style_name_range_;
  bool empty = true;
  uint64_t position_;
};

struct InspectorKeyframe {
  std::string key_text_;
  InspectorStyleSheet style_;
};

struct InspectorSelectorList {
  std::string text_;
  std::vector<std::string> selectors_order_;
  std::unordered_map<std::string, Range> selectors_;
};

struct InspectorCSSRule {
  std::string style_sheet_id_;
  std::string origin_;
  InspectorStyleSheet style_;
  InspectorSelectorList selector_list_;
};

enum class InspectorElementType {
  DOCUMENT = 0,
  STYLEVALUE,
  ELEMENT,
  COMPONENT
};

enum class InspectorNodeType : int {
  ElementNode = 1,
  kTextNode = 3,
  kDocumentNode = 9
};

enum Function {
  Index = 0,
  Parent,             // 1
  Impl,               // 2
  Type,               // 3
  ComponentName,      // 4
  SlotName,           // 5
  Tag,                // 6
  ID,                 // 7
  Children,           // 8
  ClassOrder,         // 9
  SlotFillers,        // 10
  InlineStyle,        // 11
  Attr,               // 12
  DataSet,            // 13
  EventMap,           // 14
  RootCss,            // 15
  RootAnimation,      // 16
  ThisManager,        // 17
  MessageToJSEngine,  // 18
  OnClose,            // 19
  OnTASMCreated,      // 20
  DefaultCSS,         // 21
  Density,            // 22
  BoxModel,           // 23
  ImplID,             // 24
  RemoveNode,         // 25
  SetAttribute,       // 26
  Reset,              // 27
  SetStyle,           // 28
  SetFontSize,        // 29
  FlushProps,         // 30
  Component,          // 31
  ProcessCSS,         // 32
  ProcessRootCss      // 33
};

enum class DevToolFunction : int {
  InitForInspector,
  InitPlugForInspector,
  InitStyleValueElement,
  InitStyleRoot,
  SetDocElement,
  SetStyleValueElement,
  SetStyleRoot,
};

}  // namespace devtool
}  // namespace lynx

#endif  // CORE_INSPECTOR_STYLE_SHEET_H_
