// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_LYNX_ELEMENT_QUERY_H_
#define CORE_RENDERER_DOM_LYNX_ELEMENT_QUERY_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/include/value/base_value.h"

namespace lynx {
namespace tasm {

class ElementManager;

struct LynxElementIdentity {
  int32_t sign{0};
  std::string tag;
  std::string id;
};

struct LynxElementPositionInfo {
  float left{0.f};
  float top{0.f};
  float width{0.f};
  float height{0.f};
  float padding_left{0.f};
  float padding_top{0.f};
  float padding_right{0.f};
  float padding_bottom{0.f};
  float margin_left{0.f};
  float margin_top{0.f};
  float margin_right{0.f};
  float margin_bottom{0.f};
  float border_left_width{0.f};
  float border_top_width{0.f};
  float border_right_width{0.f};
  float border_bottom_width{0.f};
};

class LynxElementQuery {
 public:
  static constexpr int32_t kInvalidSign = -1;
  static constexpr char kSignKey[] = "sign";
  static constexpr char kTagNameKey[] = "tagName";
  static constexpr char kIdKey[] = "id";
  static constexpr char kPositionInfoKey[] = "positionInfo";
  static constexpr char kDatasetKey[] = "dataset";
  static constexpr char kAttributesKey[] = "attributes";
  static constexpr char kChildrenKey[] = "children";
  static constexpr char kLeftKey[] = "left";
  static constexpr char kTopKey[] = "top";
  static constexpr char kWidthKey[] = "width";
  static constexpr char kHeightKey[] = "height";
  static constexpr char kPaddingLeftKey[] = "paddingLeft";
  static constexpr char kPaddingTopKey[] = "paddingTop";
  static constexpr char kPaddingRightKey[] = "paddingRight";
  static constexpr char kPaddingBottomKey[] = "paddingBottom";
  static constexpr char kMarginLeftKey[] = "marginLeft";
  static constexpr char kMarginTopKey[] = "marginTop";
  static constexpr char kMarginRightKey[] = "marginRight";
  static constexpr char kMarginBottomKey[] = "marginBottom";
  static constexpr char kBorderLeftWidthKey[] = "borderLeftWidth";
  static constexpr char kBorderTopWidthKey[] = "borderTopWidth";
  static constexpr char kBorderRightWidthKey[] = "borderRightWidth";
  static constexpr char kBorderBottomWidthKey[] = "borderBottomWidth";
  static constexpr char kUniqueIdFieldKey[] = "unique_id";
  static constexpr char kTagFieldKey[] = "tag";
  static constexpr char kAttributeFieldKey[] = "attribute";

  static int32_t GetRootSign(ElementManager* manager);
  static bool IsAlive(ElementManager* manager, int32_t sign);
  static bool GetIdentity(ElementManager* manager, int32_t sign,
                          LynxElementIdentity& identity);
  static int32_t GetParentSign(ElementManager* manager, int32_t sign);
  static bool GetChildrenSigns(ElementManager* manager, int32_t sign,
                               std::vector<int32_t>& children);
  static int32_t FindById(ElementManager* manager, int32_t root_sign,
                          std::string id);
  static bool GetPositionInfo(ElementManager* manager, int32_t sign,
                              LynxElementPositionInfo& info);
  static lepus::Value GetPositionInfoValue(ElementManager* manager,
                                           int32_t sign);
  static lepus::Value GetDataset(ElementManager* manager, int32_t sign);
  static lepus::Value GetAttributes(ElementManager* manager, int32_t sign);
  static lepus::Value GetAttribute(ElementManager* manager, int32_t sign,
                                   std::string name);
  static std::string ToJSONString(ElementManager* manager, int32_t root_sign);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_LYNX_ELEMENT_QUERY_H_
