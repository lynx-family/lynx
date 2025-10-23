// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_SHADOW_NODE_H_

#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include "clay/public/style_types.h"
#include "clay/third_party/txt/src/txt/paragraph.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/layout_controller.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/shadow/measure_utils.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "clay/ui/shadow/vertical_align_style.h"

namespace clay {

class Measurable;
class CustomMeasurable;
class TextUpdateBundle;

class ShadowNode {
 public:
  ShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  virtual ~ShadowNode() = default;

  virtual bool IsRawTextShadowNode() { return false; }
  virtual bool IsInlineTextShadowNode() { return false; }
  virtual bool IsInlineImageShadowNode() { return false; }
  virtual bool IsTextShadowNode() { return false; }
  virtual bool IsInlineViewShadowNode() { return false; }
  virtual bool IsBaseTextShadowNode() { return false; }
  virtual bool IsSliderShadowNode() { return false; }
  virtual bool IsEditableShadowNode() { return false; }
  virtual bool IsInlineTruncationShadowNode() { return false; }
  virtual bool IsImageShadowNode() { return false; }

  virtual Measurable* GetMeasurable() { return nullptr; }
  virtual CustomMeasurable* GetCustomMeasurable() { return nullptr; }

  // The return value represents whether the text is displayed completely after
  // layout
  virtual void TextLayout(LayoutContext* context) {}

  virtual void OnLayout(float width, TextMeasureMode width_mode, float height,
                        TextMeasureMode height_mode,
                        const std::array<float, 4>& paddings,
                        const std::array<float, 4>& borders) {
    is_dirty_ = false;
  }

  virtual void PreLayout(PreLayoutContext* context) {}

  virtual bool IsVirtual() { return false; }

  virtual void SetAttribute(const char* attr_c, const clay::Value& value);

  std::string GetName() { return tag_; }

  void SetWidth(float width) { styles_.width = width; }
  void SetHeight(float height) { styles_.height = height; }

  float Width() const { return styles_.width; }
  float Height() const { return styles_.height; }
  float PaddingLeft() const { return styles_.padding_left; }
  float PaddingTop() const { return styles_.padding_top; }
  float PaddingRight() const { return styles_.padding_right; }
  float PaddingBottom() const { return styles_.padding_bottom; }
  float MarginLeft() const { return styles_.margin_left; }
  float MarginTop() const { return styles_.margin_top; }
  float MarginRight() const { return styles_.margin_right; }
  float MarginBottom() const { return styles_.margin_bottom; }

  virtual void AddChild(ShadowNode* node);

  virtual void AddChild(ShadowNode* child, int index);

  virtual void RemoveChild(ShadowNode* child);

  void RemoveAllChildren();

  ShadowNode* Parent() { return parent_; }

  std::vector<ShadowNode*>& GetChildren() { return children_; }

  int id() const { return id_; }

  std::string id_selector() const { return id_selector_; }

  size_t ChildCount() const { return children_.size(); }

  void Destroy() { RemoveAllChildren(); }

  bool IsDirty() { return is_dirty_; }

  virtual void MarkDirty();

  ShadowNode* FindNoneVirtualNode();

  void MarkNeedsUpdate(TextUpdateFlag flag);

  bool HasInlineObject();

  void AddEventCallback(const char* event);
  bool HasEvent(const std::string& event) const {
    return events_ &&
           std::find(events_->begin(), events_->end(), event) != events_->end();
  }

  float Logical2ClayPixelRatio() {
    if (owner_) {
      return owner_->Logical2ClayPixelRatio();
    }
    return 1.0;
  }

  void UpdateLayoutStylesFromLynx();

  std::optional<VerticalAlign> GetVerticalAlign() const {
    return vertical_align_;
  }
  void SetVerticalAlign(const clay::Value& value);
  void SetVerticalAlign(std::optional<VerticalAlign> vertical_align) {
    vertical_align_ = vertical_align;
  }

  void SetBaselineOffset(const FontMetrics& parent_metrics,
                         double child_descent, double child_ascent);
  // when offset is positive, text will move up
  virtual void SetBaselineOffset(double offset) {}
  double CalculateBaselineOffset(const FontMetrics& parent_metrics,
                                 double child_descent, double child_ascent);

  void SetEndIndex(size_t end_index) { truncated_index_ = end_index; }
  size_t GetEndIndex() { return truncated_index_; }
  void ResetEndIndex();

  void CreateTextInfo(txt::Paragraph* paragraph);
  TextUpdateBundle* FindTextBundle();
  virtual void SetAttribute(KeywordID attr, const char* attr_c,
                            const clay::Value&){};

 protected:
  ClayLayoutStyles styles_{};
  std::string tag_;
  std::string id_selector_;
  ShadowNode* parent_ = nullptr;
  std::vector<ShadowNode*> children_;
  bool is_dirty_ = true;
  int id_;
  ShadowNodeOwner* owner_;
  std::optional<std::vector<std::string>> events_;

  std::optional<VerticalAlign> vertical_align_ = std::nullopt;
  // This value will be used to limit the display area of ​raw-​text and
  // inline-images and inline-view
  size_t truncated_index_ = std::numeric_limits<size_t>::max();
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_SHADOW_NODE_H_
