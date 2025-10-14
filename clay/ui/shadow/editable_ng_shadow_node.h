// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_EDITABLE_NG_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_EDITABLE_NG_SHADOW_NODE_H_

#include <string>

#include "clay/ui/shadow/editable_shadow_node.h"

namespace clay {

class EditableNGShadowNode : public EditableShadowNode {
 public:
  EditableNGShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~EditableNGShadowNode() override = default;

  void Measure(const MeasureConstraint& constraint,
               MeasureResult& result) override;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_EDITABLE_NG_SHADOW_NODE_H_
