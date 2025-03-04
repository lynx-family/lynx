// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_RADON_ELEMENT_H_
#define CORE_RENDERER_DOM_VDOM_RADON_RADON_ELEMENT_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/renderer/dom/element.h"

namespace lynx {
namespace tasm {

class RadonElement : public Element {
 public:
  RadonElement(const base::String& tag,
               const std::shared_ptr<AttributeHolder>& node,
               ElementManager* element_manager, uint32_t node_index = 0);

  ~RadonElement();

  bool is_radon_element() const override { return true; }

  bool is_view() const override { return is_view_; }

  bool is_image() const override { return is_image_; }

  bool is_text() const override { return is_text_; }

  bool is_list() const override { return is_list_; }

  virtual std::optional<CSSValue> GetElementStyle(
      tasm::CSSPropertyID css_id) override;

  virtual ListNode* GetListNode() override;

  virtual Element* GetParentComponentElement() const override;

  bool is_component() const;

  bool CanBeLayoutOnly() const override {
    return config_enable_layout_only_ && has_layout_only_props_ &&
           overflow_ == OVERFLOW_XY &&
           (!is_component() || enable_component_layout_only_);
  }

  bool InComponent() const override;

  virtual void MarkAsLayoutRoot() override;
  virtual void AttachLayoutNode(
      const std::shared_ptr<PropBundle>& props) override;
  virtual void UpdateLayoutNodeProps(
      const std::shared_ptr<PropBundle>& props) override;
  virtual void UpdateLayoutNodeStyle(CSSPropertyID css_id,
                                     const tasm::CSSValue& value) override;
  virtual void ResetLayoutNodeStyle(tasm::CSSPropertyID css_id) override;
  virtual void UpdateLayoutNodeFontSize(double cur_node_font_size,
                                        double root_node_font_size) override;
  virtual void UpdateLayoutNodeAttribute(starlight::LayoutAttribute key,
                                         const lepus::Value& value) override;

  virtual void SetNativeProps(const lepus::Value& args,
                              PipelineOptions& pipeline_options) override;

  virtual void SetAttribute(const base::String& key, const lepus::Value& value,
                            bool need_update_data_model = true) override;

  virtual void ResetAttribute(const base::String& key) override;

  virtual void ResetStyle(
      const base::Vector<CSSPropertyID>& style_names) override;

  virtual StyleMap GetStylesForWorklet() override;

  virtual const AttrMap& GetAttributesForWorklet() override;

  virtual void InsertNode(const fml::RefPtr<Element>& child) override;

  virtual void InsertNode(const fml::RefPtr<Element>& child,
                          int32_t index) override;

  void InsertNode(RadonElement* child, int32_t index = -1);

  virtual void RemoveNode(const fml::RefPtr<Element>& child,
                          bool destroy = true) override;

  void RemoveNode(RadonElement* child, bool destroy = true);

  void MarkPlatformNodeDestroyedRecursively();

  void UpdateDynamicElementStyle(uint32_t style, bool force_update) override;
  void FlushDynamicStyles();

  int ParentComponentId() const override;
  std::string ParentComponentIdString() const override;
  const std::string& ParentComponentEntryName() const override;

  Element* Sibling(int offset) const override;
  void AddChildAt(RadonElement* child, size_t index);
  RadonElement* RemoveChildAt(size_t index);

  virtual int32_t IndexOf(const Element* child) const override;

  bool GetPageElementEnabled() override;
  bool GetRemoveCSSScopeEnabled() override;

  Element* GetChildAt(size_t index) override;

  size_t GetChildCount() override { return children_.size(); }

  ElementChildrenArray GetChildren() override { return children_; }

  size_t GetUIIndexForChild(Element* child) override;

  void SetComponentIDPropsIfNeeded();

  bool IsBeforeContent();
  bool IsAfterContent();

  void SetIsPseudoNode() { is_pseudo_ = true; }
  bool IsPseudoNode() { return is_pseudo_; }

  // Flush style and attribute to platform shadow node, platform painting node
  // will be created if has not been created,
  void FlushProps() override;
  void FlushPropsFirstTimeWithParentElement(Element* parent);

  void RequestLayout() override;

  void RequestNextFrame() override;

  void OnPseudoStatusChanged(PseudoState prev_status,
                             PseudoState current_status) override;

  void ConsumeStyle(const StyleMap& styles,
                    StyleMap* inherit_styles = nullptr) override;

  void OnPatchFinish(PipelineOptions& option) override;

  void FlushAnimatedStyleInternal(tasm::CSSPropertyID,
                                  const tasm::CSSValue&) override;

  virtual void ConsumeTransitionStylesInAdvanceInternal(
      CSSPropertyID css_id, const tasm::CSSValue& value) override;
  virtual void ResetTransitionStylesInAdvanceInternal(
      CSSPropertyID css_id) override;

  virtual bool NeedFastFlushPath(
      const std::pair<CSSPropertyID, tasm::CSSValue>& style) override;

  void ResolveStyleValue(CSSPropertyID id, const tasm::CSSValue& value,
                         bool force_update) override;

  virtual CSSFragment* GetRelatedCSSFragment() override;

  virtual int32_t GetCSSID() const override;

  virtual size_t CountInlineStyles() override;
  virtual void MergeInlineStyles(StyleMap& new_styles) override;

  virtual bool WillResolveStyle(StyleMap& merged_styles) override;

  virtual const base::String& GetPlatformNodeTag() const override;

  void UpdatePlatformNodeTag();
  virtual Element* first_child() const override;
  virtual Element* last_child() const override;

 private:
  void RemoveNode(RadonElement* child, int32_t index, bool destroy);

  void ClearDynamicCSSChildrenStatus();

  size_t GetUIChildrenCount();

  constexpr const static char kViewTag[] = "view";
  constexpr const static char kImageTag[] = "image";
  constexpr const static char kTextTag[] = "text";
  constexpr const static char kListTag[] = "list";

  std::unordered_set<CSSPropertyID> animation_props_{};

  bool is_view_{false};
  bool is_image_{false};
  bool is_list_{false};

  // This field is reserved for list
  base::String platform_node_tag_{BASE_STATIC_STRING(kListNodeTag)};

  StyleMap styles_;
  AttrMap attributes_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_RADON_ELEMENT_H_
