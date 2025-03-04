// (c) Dean McNamee <dean@gmail.com>, 2012.
// C++ port by Mapbox, Konstantin Käfer <mail@kkaefer.com>, 2014-2017.
//
// https://github.com/deanm/css-color-parser-js
// https://github.com/kkaefer/css-color-parser-cpp
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/element/inspector_css_helper.h"

#include <unordered_map>
#include <unordered_set>

#include "base/include/compiler_specific.h"
#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace devtool {

#define FOREACH_ALL_STRING_PROPERTY(V)                                        \
  V(Display, "display", "flex", "inline", "none")                             \
  V(Position, "position", "relative", "absolute", "sticky")                   \
  V(FontWeight, "font-weight", "normal", "bold", "100", "200", "300", "400",  \
    "500", "600", "700", "800", "900")                                        \
  V(WhiteSpace, "white-space", "normal", "nowrap")                            \
  V(BorderStyle, "border-style", "solid", "dashed", "dotted", "double",       \
    "groove", "ridge", "inset", "outset", "hidden", "none")                   \
  V(TextAlign, "text-align", "left", "right", "center")                       \
  V(TextOverflow, "text-overflow", "clip", "ellipsis")                        \
  V(FlexDirection, "flex-direction", "row", "row-reverse", "column",          \
    "column-reverse")                                                         \
  V(BoxSizing, "box-sizing", "border-box")                                    \
  V(Overflow, "overflow", "hidden", "visible")                                \
  V(OverflowX, "overflow-x", "hidden", "visible")                             \
  V(OverflowY, "overflow-y", "hidden", "visible")                             \
  V(FlexWrap, "flex-wrap", "nowrap", "wrap")                                  \
  V(AlignSelf, "align-self", "stretch", "center", "flex-start", "flex-end")   \
  V(AlignItems, "align-items", "stretch", "center", "flex-start", "flex-end") \
  V(AlignContent, "align-content", "stretch", "center", "flex-start",         \
    "flex-end", "space-between", "space-around")                              \
  V(JustifyContent, "justify-content", "center", "flex-start", "flex-end",    \
    "space-between", "space-around")                                          \
  V(WordBreak, "word-break", "normal", "break-all", "keep-all")

namespace {
bool IsUint(const std::string& str) {
  bool res = !str.empty();
  for (const auto& c : str) {
    if (c < '0' || c > '9') {
      res = false;
      break;
    }
  }
  return res;
}

bool IsFloat(const std::string& str) {
  bool res = !str.empty();
  bool has_point = false;
  for (const auto& c : str) {
    if (c == '.' && !has_point) {
      has_point = true;
      continue;
    }

    if (c < '0' || c > '9') {
      res = false;
      break;
    }
  }
  return res;
}

struct NamedColor {
  const char* const name;
  const std::vector<int> color;
};

const NamedColor namedColors[] = {{"transparent", {0, 0, 0, 0}},
                                  {"aliceblue", {240, 248, 255, 1}},
                                  {"antiquewhite", {250, 235, 215, 1}},
                                  {"aqua", {0, 255, 255, 1}},
                                  {"aquamarine", {127, 255, 212, 1}},
                                  {"azure", {240, 255, 255, 1}},
                                  {"beige", {245, 245, 220, 1}},
                                  {"bisque", {255, 228, 196, 1}},
                                  {"black", {0, 0, 0, 1}},
                                  {"blanchedalmond", {255, 235, 205, 1}},
                                  {"blue", {0, 0, 255, 1}},
                                  {"blueviolet", {138, 43, 226, 1}},
                                  {"brown", {165, 42, 42, 1}},
                                  {"burlywood", {222, 184, 135, 1}},
                                  {"cadetblue", {95, 158, 160, 1}},
                                  {"chartreuse", {127, 255, 0, 1}},
                                  {"chocolate", {210, 105, 30, 1}},
                                  {"coral", {255, 127, 80, 1}},
                                  {"cornflowerblue", {100, 149, 237, 1}},
                                  {"cornsilk", {255, 248, 220, 1}},
                                  {"crimson", {220, 20, 60, 1}},
                                  {"cyan", {0, 255, 255, 1}},
                                  {"darkblue", {0, 0, 139, 1}},
                                  {"darkcyan", {0, 139, 139, 1}},
                                  {"darkgoldenrod", {184, 134, 11, 1}},
                                  {"darkgray", {169, 169, 169, 1}},
                                  {"darkgreen", {0, 100, 0, 1}},
                                  {"darkgrey", {169, 169, 169, 1}},
                                  {"darkkhaki", {189, 183, 107, 1}},
                                  {"darkmagenta", {139, 0, 139, 1}},
                                  {"darkolivegreen", {85, 107, 47, 1}},
                                  {"darkorange", {255, 140, 0, 1}},
                                  {"darkorchid", {153, 50, 204, 1}},
                                  {"darkred", {139, 0, 0, 1}},
                                  {"darksalmon", {233, 150, 122, 1}},
                                  {"darkseagreen", {143, 188, 143, 1}},
                                  {"darkslateblue", {72, 61, 139, 1}},
                                  {"darkslategray", {47, 79, 79, 1}},
                                  {"darkslategrey", {47, 79, 79, 1}},
                                  {"darkturquoise", {0, 206, 209, 1}},
                                  {"darkviolet", {148, 0, 211, 1}},
                                  {"deeppink", {255, 20, 147, 1}},
                                  {"deepskyblue", {0, 191, 255, 1}},
                                  {"dimgray", {105, 105, 105, 1}},
                                  {"dimgrey", {105, 105, 105, 1}},
                                  {"dodgerblue", {30, 144, 255, 1}},
                                  {"firebrick", {178, 34, 34, 1}},
                                  {"floralwhite", {255, 250, 240, 1}},
                                  {"forestgreen", {34, 139, 34, 1}},
                                  {"fuchsia", {255, 0, 255, 1}},
                                  {"gainsboro", {220, 220, 220, 1}},
                                  {"ghostwhite", {248, 248, 255, 1}},
                                  {"gold", {255, 215, 0, 1}},
                                  {"goldenrod", {218, 165, 32, 1}},
                                  {"gray", {128, 128, 128, 1}},
                                  {"green", {0, 128, 0, 1}},
                                  {"greenyellow", {173, 255, 47, 1}},
                                  {"grey", {128, 128, 128, 1}},
                                  {"honeydew", {240, 255, 240, 1}},
                                  {"hotpink", {255, 105, 180, 1}},
                                  {"indianred", {205, 92, 92, 1}},
                                  {"indigo", {75, 0, 130, 1}},
                                  {"ivory", {255, 255, 240, 1}},
                                  {"khaki", {240, 230, 140, 1}},
                                  {"lavender", {230, 230, 250, 1}},
                                  {"lavenderblush", {255, 240, 245, 1}},
                                  {"lawngreen", {124, 252, 0, 1}},
                                  {"lemonchiffon", {255, 250, 205, 1}},
                                  {"lightblue", {173, 216, 230, 1}},
                                  {"lightcoral", {240, 128, 128, 1}},
                                  {"lightcyan", {224, 255, 255, 1}},
                                  {"lightgoldenrodyellow", {250, 250, 210, 1}},
                                  {"lightgray", {211, 211, 211, 1}},
                                  {"lightgreen", {144, 238, 144, 1}},
                                  {"lightgrey", {211, 211, 211, 1}},
                                  {"lightpink", {255, 182, 193, 1}},
                                  {"lightsalmon", {255, 160, 122, 1}},
                                  {"lightseagreen", {32, 178, 170, 1}},
                                  {"lightskyblue", {135, 206, 250, 1}},
                                  {"lightslategray", {119, 136, 153, 1}},
                                  {"lightslategrey", {119, 136, 153, 1}},
                                  {"lightsteelblue", {176, 196, 222, 1}},
                                  {"lightyellow", {255, 255, 224, 1}},
                                  {"lime", {0, 255, 0, 1}},
                                  {"limegreen", {50, 205, 50, 1}},
                                  {"linen", {250, 240, 230, 1}},
                                  {"magenta", {255, 0, 255, 1}},
                                  {"maroon", {128, 0, 0, 1}},
                                  {"mediumaquamarine", {102, 205, 170, 1}},
                                  {"mediumblue", {0, 0, 205, 1}},
                                  {"mediumorchid", {186, 85, 211, 1}},
                                  {"mediumpurple", {147, 112, 219, 1}},
                                  {"mediumseagreen", {60, 179, 113, 1}},
                                  {"mediumslateblue", {123, 104, 238, 1}},
                                  {"mediumspringgreen", {0, 250, 154, 1}},
                                  {"mediumturquoise", {72, 209, 204, 1}},
                                  {"mediumvioletred", {199, 21, 133, 1}},
                                  {"midnightblue", {25, 25, 112, 1}},
                                  {"mintcream", {245, 255, 250, 1}},
                                  {"mistyrose", {255, 228, 225, 1}},
                                  {"moccasin", {255, 228, 181, 1}},
                                  {"navajowhite", {255, 222, 173, 1}},
                                  {"navy", {0, 0, 128, 1}},
                                  {"oldlace", {253, 245, 230, 1}},
                                  {"olive", {128, 128, 0, 1}},
                                  {"olivedrab", {107, 142, 35, 1}},
                                  {"orange", {255, 165, 0, 1}},
                                  {"orangered", {255, 69, 0, 1}},
                                  {"orchid", {218, 112, 214, 1}},
                                  {"palegoldenrod", {238, 232, 170, 1}},
                                  {"palegreen", {152, 251, 152, 1}},
                                  {"paleturquoise", {175, 238, 238, 1}},
                                  {"palevioletred", {219, 112, 147, 1}},
                                  {"papayawhip", {255, 239, 213, 1}},
                                  {"peachpuff", {255, 218, 185, 1}},
                                  {"peru", {205, 133, 63, 1}},
                                  {"pink", {255, 192, 203, 1}},
                                  {"plum", {221, 160, 221, 1}},
                                  {"powderblue", {176, 224, 230, 1}},
                                  {"purple", {128, 0, 128, 1}},
                                  {"red", {255, 0, 0, 1}},
                                  {"rosybrown", {188, 143, 143, 1}},
                                  {"royalblue", {65, 105, 225, 1}},
                                  {"saddlebrown", {139, 69, 19, 1}},
                                  {"salmon", {250, 128, 114, 1}},
                                  {"sandybrown", {244, 164, 96, 1}},
                                  {"seagreen", {46, 139, 87, 1}},
                                  {"seashell", {255, 245, 238, 1}},
                                  {"sienna", {160, 82, 45, 1}},
                                  {"silver", {192, 192, 192, 1}},
                                  {"skyblue", {135, 206, 235, 1}},
                                  {"slateblue", {106, 90, 205, 1}},
                                  {"slategray", {112, 128, 144, 1}},
                                  {"slategrey", {112, 128, 144, 1}},
                                  {"snow", {255, 250, 250, 1}},
                                  {"springgreen", {0, 255, 127, 1}},
                                  {"steelblue", {70, 130, 180, 1}},
                                  {"tan", {210, 180, 140, 1}},
                                  {"teal", {0, 128, 128, 1}},
                                  {"thistle", {216, 191, 216, 1}},
                                  {"tomato", {255, 99, 71, 1}},
                                  {"turquoise", {64, 224, 208, 1}},
                                  {"violet", {238, 130, 238, 1}},
                                  {"wheat", {245, 222, 179, 1}},
                                  {"white", {255, 255, 255, 1}},
                                  {"whitesmoke", {245, 245, 245, 1}},
                                  {"yellow", {255, 255, 0, 1}},
                                  {"yellowgreen", {154, 205, 50, 1}}};

template <typename T>
uint8_t clamp_css_byte(T i) {  // Clamp to integer 0 .. 255.
  i = round(i);
  return i < 0 ? 0 : i > 255 ? 255 : i;
}

template <typename T>
float clamp_css_float(T f) {  // Clamp to float 0.0 .. 1.0.
  return f < 0 ? 0 : f > 1 ? 1 : f;
}

#define DECLARE_PROPERTY_ID(name, name_str, ...) \
  std::unordered_set<std::string> kStringPropValueSet##name{__VA_ARGS__};
FOREACH_ALL_STRING_PROPERTY(DECLARE_PROPERTY_ID)
#undef DECLARE_PROPERTY_ID

static std::unordered_map<lynx::tasm::CSSPropertyID,
                          std::unordered_set<std::string>>
    string_prop_value_map = {
#define DECLARE_PROPERTY_ID(name, name_str, args...) \
  {lynx::tasm::kPropertyID##name, kStringPropValueSet##name},
        FOREACH_ALL_STRING_PROPERTY(DECLARE_PROPERTY_ID)
#undef DECLARE_PROPERTY_ID
            {lynx::tasm::kPropertyEnd, std::unordered_set<std::string>()}};

}  // namespace

bool InspectorCSSHelper::IsColor(lynx::tasm::CSSPropertyID id) {
  switch (id) {
    case lynx::tasm::kPropertyIDColor:
    case lynx::tasm::kPropertyIDBackgroundColor:
    case lynx::tasm::kPropertyIDBorderBottomColor:
    case lynx::tasm::kPropertyIDBorderTopColor:
    case lynx::tasm::kPropertyIDBorderLeftColor:
    case lynx::tasm::kPropertyIDBorderRightColor:
      return true;
    default:
      return false;
  }
}

bool InspectorCSSHelper::IsDimension(lynx::tasm::CSSPropertyID id) {
  switch (id) {
    case lynx::tasm::kPropertyIDLineSpacing:
    case lynx::tasm::kPropertyIDLetterSpacing:
    case lynx::tasm::kPropertyIDBorderWidth:
    case lynx::tasm::kPropertyIDBorderLeftWidth:
    case lynx::tasm::kPropertyIDBorderRightWidth:
    case lynx::tasm::kPropertyIDBorderTopWidth:
    case lynx::tasm::kPropertyIDBorderBottomWidth:
    case lynx::tasm::kPropertyIDBorderRadius:
    case lynx::tasm::kPropertyIDBorderTopLeftRadius:
    case lynx::tasm::kPropertyIDBorderBottomLeftRadius:
    case lynx::tasm::kPropertyIDBorderTopRightRadius:
    case lynx::tasm::kPropertyIDBorderBottomRightRadius:
      return true;
    default:
      return false;
  }
}

bool InspectorCSSHelper::IsAutoDimension(lynx::tasm::CSSPropertyID id) {
  switch (id) {
    case lynx::tasm::kPropertyIDTop:
    case lynx::tasm::kPropertyIDBottom:
    case lynx::tasm::kPropertyIDLeft:
    case lynx::tasm::kPropertyIDRight:
    case lynx::tasm::kPropertyIDHeight:
    case lynx::tasm::kPropertyIDWidth:
    case lynx::tasm::kPropertyIDMaxHeight:
    case lynx::tasm::kPropertyIDMaxWidth:
    case lynx::tasm::kPropertyIDMinHeight:
    case lynx::tasm::kPropertyIDMinWidth:
    case lynx::tasm::kPropertyIDPadding:
    case lynx::tasm::kPropertyIDPaddingTop:
    case lynx::tasm::kPropertyIDPaddingBottom:
    case lynx::tasm::kPropertyIDPaddingLeft:
    case lynx::tasm::kPropertyIDPaddingRight:
    case lynx::tasm::kPropertyIDMargin:
    case lynx::tasm::kPropertyIDMarginTop:
    case lynx::tasm::kPropertyIDMarginBottom:
    case lynx::tasm::kPropertyIDMarginLeft:
    case lynx::tasm::kPropertyIDMarginRight:
    case lynx::tasm::kPropertyIDFlexBasis:
      return true;
    default:
      return false;
  }
}

bool InspectorCSSHelper::IsStringProp(lynx::tasm::CSSPropertyID id) {
  switch (id) {
#define DECLARE_PROPERTY_ID(name, name_str, args...) \
  case lynx::tasm::kPropertyID##name:
    FOREACH_ALL_STRING_PROPERTY(DECLARE_PROPERTY_ID)
#undef DECLARE_PROPERTY_ID
    return true;
    default:
      return false;
  }
}

bool InspectorCSSHelper::IsIntProp(lynx::tasm::CSSPropertyID id) {
  switch (id) {
    case lynx::tasm::kPropertyIDOrder:
      return true;
    default:
      return false;
  }
}

bool InspectorCSSHelper::IsFloatProp(lynx::tasm::CSSPropertyID id) {
  switch (id) {
    case lynx::tasm::kPropertyIDOpacity:
    case lynx::tasm::kPropertyIDFlex:
    case lynx::tasm::kPropertyIDFlexGrow:
    case lynx::tasm::kPropertyIDFlexShrink:
      return true;
    default:
      return false;
  }
}

bool InspectorCSSHelper::IsBorderProp(lynx::tasm::CSSPropertyID id) {
  switch (id) {
    case lynx::tasm::kPropertyIDBorder:
    case lynx::tasm::kPropertyIDBorderRight:
    case lynx::tasm::kPropertyIDBorderLeft:
    case lynx::tasm::kPropertyIDBorderTop:
    case lynx::tasm::kPropertyIDBorderBottom:
      return true;
    default:
      return false;
  }
}

bool InspectorCSSHelper::IsSupportedProp(lynx::tasm::CSSPropertyID id) {
  switch (id) {
#define DECLARE_PROPERTY_ID(name, c, value) case lynx::tasm::kPropertyID##name:
    FOREACH_ALL_PROPERTY(DECLARE_PROPERTY_ID)
#undef DECLARE_PROPERTY_ID
    return true;
    default:
      return false;
  }
}

bool InspectorCSSHelper::IsLegal(const std::string& name,
                                 const std::string& value) {
  lynx::tasm::StyleMap output;
  lynx::tasm::CSSParserConfigs configs;
  return lynx::tasm::UnitHandler::Process(
      lynx::tasm::CSSProperty::GetPropertyID(name), lynx::lepus::Value(value),
      output, configs);
}

bool InspectorCSSHelper::IsAnimationLegal(const std::string& name,
                                          const std::string& value) {
  bool res = false;
  if (name == "animation-duration" || name == "animation-delay") {
    if (value == "0")
      res = true;
    else if (value.length() > 2 && lynx::base::EndsWith(value, "ms")) {
      res = IsUint(value.substr(0, value.size() - 2));
    } else if (value.length() > 1 && lynx::base::EndsWith(value, "s")) {
      auto num = value.substr(0, value.size() - 1);
      res = IsUint(num) || IsFloat(num);
    }
  } else if (name == "animation-timing-function") {
    if (value == "linear" || value == "ease" || value == "ease-in" ||
        value == "ease-out" || value == "ease-in-out" ||
        value.find("cubic-bezier") != std::string::npos)
      res = true;
  } else if (name == "animation-iteration-count") {
    if (value == "infinite" || IsUint(value)) res = true;
  } else if (name == "animation-direction") {
    if (value == "normal" || value == "reverse" || value == "alternate" ||
        value == "alternate-reverse")
      res = true;
  } else if (name == "animation-fill-mode") {
    if (value == "none" || value == "forwards" || value == "backwards" ||
        value == "both")
      res = true;
  } else if (name == "animation-play-state") {
    if (value == "running" || value == "paused") res = true;
  }
  return res;
}

}  // namespace devtool
}  // namespace lynx
