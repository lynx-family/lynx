// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_LAYOUT_NODE_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_LAYOUT_NODE_H_

#include <memory>
#include <set>

#include "base/include/string/string_utils.h"
#include "base/include/vector.h"
#include "core/public/layout_node_value.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/types/layout_measurefunc.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx {
namespace tasm {

class LayoutContext;

class LayoutNode {
 public:
  LayoutNode(int id, const starlight::LayoutConfigs& layout_configs,
             const tasm::LynxEnvConfig& envs,
             const starlight::ComputedCSSStyle& init_style);

  // interface of  inline view
  FloatSize UpdateMeasureByPlatform(const starlight::Constraints& constraints,
                                    bool final_measure);
  void AlignmentByPlatform(float offset_top, float offset_left);
  void CalculateLayout(const SLNodeSet* fixed_node_set = nullptr);
  void CalculateLayoutWithConstraints(
      starlight::Constraints& constraints,
      const SLNodeSet* fixed_node_set = nullptr);
  void SetMeasureFunc(std::unique_ptr<MeasureFunc> measure_func);
  void InsertNode(LayoutNode* child, int index = -1);
  LayoutNode* RemoveNodeAtIndex(unsigned int index);
  void MoveNode(LayoutNode* child, int from_index, unsigned int to_index);
  void ConsumeFontSize(double cur_node_font_size, double root_node_font_size,
                       double font_scale);
  void ConsumeStyle(CSSPropertyID id, const tasm::CSSValue& value,
                    bool reset = false);
  void ConsumeAttribute(const starlight::LayoutAttribute key,
                        const lepus::Value& value, bool reset = false);

  inline LayoutNode* parent() const { return parent_; }
  inline SLNode* slnode() { return &sl_node_; }
  inline const auto& children() { return children_; }
  inline bool is_virtual() { return type_ & LayoutNodeType::VIRTUAL; }
  inline bool is_common() { return type_ & LayoutNodeType::COMMON; }
  inline bool is_custom() { return type_ & LayoutNodeType::CUSTOM; }
  inline bool is_inline_view() { return type_ & LayoutNodeType::INLINE; };
  inline MeasureFunc* measure_func() { return measure_func_.get(); };
  inline int id() { return id_; }
  inline bool is_list_container() const { return is_list_container_; }
  void set_type(LayoutNodeType type);
  bool IsDirty();
  void MarkDirty();
  void MarkDirtyAndRequestLayout();
  void CleanDirty();
  void MarkUpdated();
  void UpdateLynxEnv(const tasm::LynxEnvConfig& config);
  LayoutNode* FindNonVirtualNode();
  LayoutNode* FindNextNonVirtualChild(size_t before_index) const;

  void SetTag(const base::String& tag);
  starlight::ComputedCSSStyle* GetCSSMutableStyle() { return css_style_.get(); }
  inline const base::String& tag() const { return tag_; }

 protected:
  int id_;
  LayoutNodeType type_;

  bool is_dirty_{false};
  // Whether node is a native list element which needs to invoke
  // OnListElementUpdated() callback after layout.
  bool is_list_container_{false};

  base::InlineVector<LayoutNode*, starlight::kChildrenInlineVectorSize>
      children_;
  LayoutNode* parent_ = nullptr;
  std::unique_ptr<MeasureFunc> measure_func_;

  base::String tag_;
  std::unique_ptr<starlight::ComputedCSSStyle> css_style_;

  SLNode sl_node_;

 private:
  LayoutNode(const LayoutNode&) = delete;
  LayoutNode& operator=(const LayoutNode&) = delete;
  void MarkDirtyInternal(bool request_layout);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_LAYOUT_NODE_H_
