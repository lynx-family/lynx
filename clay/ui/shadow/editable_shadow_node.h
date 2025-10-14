// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_EDITABLE_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_EDITABLE_SHADOW_NODE_H_

#include <limits>
#include <optional>
#include <string>

#include "clay/ui/component/measurable.h"
#include "clay/ui/shadow/shadow_node.h"

namespace clay {

constexpr float kDefaultFontSizeInDip = 14.f;

class EditableShadowNode : public ShadowNode, public Measurable {
 public:
  EditableShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~EditableShadowNode() override = default;

  Measurable* GetMeasurable() override { return this; }

  void SetAttribute(const char* attr_c, const clay::Value& value) override;
  bool IsEditableShadowNode() override { return true; }
  void Measure(const MeasureConstraint& constraint,
               MeasureResult& result) override;

  void SetTextHeight(float text_height);

 private:
  float font_size_;
  std::optional<float> line_height_;
  uint32_t max_lines_ = std::numeric_limits<uint32_t>::max();
  float min_height_ = -1;
  float max_height_ = -1;
  float text_height_ = 0;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_EDITABLE_SHADOW_NODE_H_
