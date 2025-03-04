// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_ELEMENT_H_
#define CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_ELEMENT_H_

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/dom/air/air_element/air_element_container.h"
#include "core/renderer/ui_wrapper/layout/layout_node.h"
#include "core/renderer/ui_wrapper/painting/catalyzer.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/renderer/utils/base/base_def.h"
#include "core/runtime/vm/lepus/ref_type.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

using AirElementVector = std::vector<AirElement*>;
using SharedAirElementVector = std::vector<std::shared_ptr<AirElement>>;
using ClassVector = std::vector<std::string>;

enum AirElementType {
  kAirUnknown = -1,
  kAirNormal,
  kAirPage,
  kAirBlock,
  kAirIf,
  kAirRadonIf,
  kAirFor,
  kAirComponent,
  kAirRawText,
};

class ElementManager;
class CSSKeyframesToken;

class AirElement {
 public:
  constexpr static const char kAirBlockTag[] = "block";
  constexpr static const char kAirIfTag[] = "if";
  constexpr static const char* kAirRadonIfTag = "radon_if";
  constexpr static const char kAirForTag[] = "for";
  constexpr static const char kAirComponentTag[] = "component";
  // lepus element related property
  constexpr static const char kAirLepusId[] = "lepusId";
  constexpr static const char kAirLepusUniqueId[] = "uniqueId";
  constexpr static const char kAirLepusKey[] = "lepusKey";
  constexpr static const char kAirLepusParent[] = "parent";
  constexpr static const char kAirLepusType[] = "type";
  constexpr static const char kAirLepusTag[] = "tag";
  constexpr static const char kAirLepusUseOpt[] = "useOpt";
  constexpr static const char kAirLepusComponentName[] = "name";
  constexpr static const char kAirLepusComponentPath[] = "path";
  constexpr static const char kAirLepusComponentTid[] = "tid";
  constexpr static const char kAirLepusContentBits[] = "contentBits";
  constexpr static const char kAirLepusIfIndex[] = "index";
  constexpr static const char kAirLepusForCount[] = "count";
  constexpr static const char kAirLepusInlineStyle[] = "inlineStyles";
  constexpr static const char kAirLepusAttrs[] = "attrs";
  constexpr static const char kAirLepusClasses[] = "classes";
  constexpr static const char kAirLepusIdSelector[] = "id";
  constexpr static const char kAirLepusEvent[] = "event";
  constexpr static const char kAirLepusEventType[] = "type";
  constexpr static const char kAirLepusEventName[] = "name";
  constexpr static const char kAirLepusEventCallback[] = "callback";
  constexpr static const char kAirLepusDataset[] = "dataSet";

  AirElement(AirElementType type, ElementManager* manager,
             const base::String& tag, uint32_t lepus_id, int32_t id = -1);

  virtual ~AirElement() = default;

  AirElement(const AirElement& node) = delete;
  AirElement& operator=(const AirElement& node) = delete;

  static void MergeHigherPriorityCSSStyle(StyleMap& primary,
                                          const StyleMap& higher);

  // to specific logic ( for example: insertNode)
  virtual bool is_page() const { return false; }

  virtual bool is_for() const { return false; }

  virtual bool is_if() const { return false; }

  virtual bool is_block() const { return false; }

  virtual bool is_component() const { return false; }

  bool is_virtual_node() const {
    return element_type_ == kAirBlock || element_type_ == kAirIf ||
           element_type_ == kAirRadonIf || element_type_ == kAirFor;
  }

  virtual void OnElementRemoved() {}

  /**
   * InsertNode
   * @param child child will be inserted
   * @param from_virtual_child if parent was virtual element
   */

  virtual void InsertNode(AirElement* child, bool from_virtual_child = false);

  /**
   * InsertNode
   * @param child child will be inserted
   * @param reference_child target child
   */
  virtual void InsertNodeBefore(AirElement* child,
                                const AirElement* reference_child);

  /**
   * InsertNode
   * @param child child will be inserted
   * @param index index of target child
   */
  void InsertNodeAfterIndex(AirElement* child, int& index);

  /**
   * InsertNode insert node to the last index
   * @param child child will be inserted
   */
  void InsertNodeAtBottom(AirElement* child);

  /**
   * RemoveAllNodes
   * @param destroy if the platform node should be destroyed
   */
  virtual void RemoveAllNodes(bool destroy = true);

  /**
   * NonVirtualNodeCountInParent
   * return count of actual node
   */
  virtual uint32_t NonVirtualNodeCountInParent() { return 1; }

  /**
   * InsertAirNode air_element tree (both actual element and virtual element are
   * included)
   * @param child child will be inserted
   */
  void InsertAirNode(AirElement* child);

  /**
   * LastNonVirtualNode the last actual element in children
   */
  AirElement* LastNonVirtualNode();

  /**
   * impl_id the unique id of air_element
   */
  int impl_id() const { return id_; }

  inline ElementManager* element_manager() const {
    return air_element_manager_;
  }
  inline AirElement* parent() { return parent_; }

  /**
   * air_parent parent of air_element (parent maybe actual node or virtual node)
   */
  inline AirElement* air_parent() { return air_parent_; }

  /**
   * RemoveNode
   * @param child child will be removed
   * @param destroy if the platform node should be destroyed
   */
  virtual void RemoveNode(AirElement* child, bool destroy = true);

  /**
   * RemoveNode
   * @param child child will be removed
   * @param index index of child
   * @param destroy if the platform node should be destroyed
   */
  void RemoveNode(AirElement* child, unsigned int index, bool destroy = true);

  /**
   * RemoveNode remove child from air_children
   * @param child child will be removed
   */
  void RemoveAirNode(AirElement* child);

  /**
   * RemoveNode
   * @param child remove child from air_children
   * @param index index of child
   * @param destroy if the platform node should be destroyed
   */
  void RemoveAirNode(AirElement* child, unsigned int index,
                     bool destroy = true);

  void Destroy();

  // Called from render function '__AirSetAttribute', push props to
  // props_bundle_. Just attribute, does not contain css style
  void SetAttribute(const base::String& key, const lepus::Value& value,
                    bool resolve = true);

  void SetInlineStyle(CSSPropertyID id, const tasm::CSSValue& value);
  void SetInlineStyle(CSSPropertyID id, tasm::CSSValue&& value);
  void SetInlineStyle(CSSPropertyID id, const lepus::Value& value,
                      bool resolve = true);
  void SetInlineStyle(const std::string& inline_style, bool resolve = true);

  void SetEventHandler(const base::String& name, EventHandler* handler);
  void ResetEventHandlers();

  /**
   * FlushProps flush style and attribute to platform shadow node, platform
   * painting node will be created if has not been created
   */
  void FlushProps();

  void FlushPropsResolveStyles(bool resolve_styles = true);

  /**
   * FlushProps flush element tree recursively
   */
  virtual void FlushRecursively();

  const EventMap& event_map() const { return static_events_; }

  inline const DataMap& data_model() const { return data_set_; }

  inline void SetDataSet(const base::String& key, const lepus::Value& value) {
    data_set_[key] = value;
  }

  /**
   * InComponent if the element inside component
   */
  bool InComponent() const;

  AirElement* GetParentComponent() const;

  inline bool IsLayoutOnly() const { return is_layout_only_; }

  /**
   * UpdateLayout save the layout info
   */
  void UpdateLayout(float left, float top, float width, float height,
                    const std::array<float, 4>& paddings,
                    const std::array<float, 4>& margins,
                    const std::array<float, 4>& borders,
                    const std::array<float, 4>* sticky_positions,
                    float max_height);

  /**
   * PushDynamicNode for radon mode
   */
  void PushDynamicNode(AirElement* node);

  /**
   * GetDynamicNode for radon mode
   */
  AirElement* GetDynamicNode(uint32_t index, uint32_t lepus_id) const;

  size_t GetChildCount() const { return children_.size(); }
  AirElementContainer* element_container() const {
    return element_container_.get();
  }
  bool is_virtual() const { return is_virtual_; }
  AirElement* GetChildAt(size_t index) const;
  virtual lepus::Value GetData();
  virtual lepus::Value GetProperties();
  /**
   * GetLepusId lepus id of element, (readOnly)
   */
  inline uint32_t GetLepusId() const { return lepus_id_; };
  const base::String& GetTag() const { return tag_; }
  AirElementType GetElementType() const { return element_type_; }
  starlight::ComputedCSSStyle* computed_css_style();

  inline EventHandler* SetEvent(const base::String& type,
                                const base::String& name,
                                const base::String& value) {
    auto event = std::make_unique<EventHandler>(type, name, value);
    auto event_raw_ptr = event.get();
    static_events_[name] = std::move(event);
    return event_raw_ptr;
  }

  bool HasElementContainer() const { return element_container_ != nullptr; }

  bool CheckFlattenProp(const base::String& key,
                        const lepus::Value& value = lepus::Value(true));

  const AirElementVector& dynamic_nodes() const { return dynamic_nodes_; }
  const SharedAirElementVector& air_children() const { return air_children_; }

  bool has_been_removed() const { return has_been_removed_; }

  // Contains global and tag, get only once on first screen.
  void GetStableStyleMap(const std::string& tag_name, StyleMap& result);
  void GetClassStyleMap(const ClassVector& class_list, StyleMap& result);
  void GetIdStyleMap(const std::string& id_name, StyleMap& result);
  void GetKeyframesMap(const std::string& keyframes_name, StyleMap& result);

  void CheckHasNonFlattenCSSProps(CSSPropertyID id);

  void CheckHasNonFlattenAttr(const base::String& key,
                              const lepus::Value& value = lepus::Value());

  // Set parsed style for page and component
  virtual void SetParsedStyles(const AirCompStylesMap& parsed_styles) {
    parsed_styles_ = parsed_styles;
  };
  void SetEnableAsyncCalc(bool enable) { enable_async_calc_ = enable; }
  bool EnableAsyncCalc() const { return enable_async_calc_; }

  // Selector
  void SetClasses(const lepus::Value& class_names);
  void SetIdSelector(const lepus::Value& id_selector);
  bool CalcStyle(bool waiting = true);
  std::atomic_uint state_{ElementState::kCreated};
  // For the first screen, async thread and main thread will resolve this
  // element according to the state
  enum ElementState {
    kCreated = 0x01 << 0,
    kStyleCalculating = 0x01 << 1,
    kStyleCalculated = 0x01 << 2,
    kStyleShadowNodeCreated = 0x01 << 3,
    kPropsUpdated = 0x01 << 4,
  };

 protected:
  friend class AirElementContainer;
  friend class AirIfElement;
  friend class AirForElement;
  friend class AirBlockElement;
  friend class AirComponentElement;
  friend class AirRadonIfElement;
  friend class Catalyzer;
  inline bool frame_changed() const { return frame_changed_; }

  void AddChildAt(AirElement* child, size_t index);
  AirElement* RemoveChildAt(size_t index);
  int IndexOf(const AirElement* child);

  void AddAirChildAt(AirElement* child, size_t index);
  AirElement* RemoveAirChildAt(size_t index);
  int IndexOfAirChild(AirElement* child);

  PaintingContext* painting_context();

  void CreateElementContainer(bool platform_is_flatten);

  std::unique_ptr<AirElementContainer> element_container_;

  void set_parent(AirElement* parent) { parent_ = parent; }
  void set_air_parent(AirElement* air_parent) { air_parent_ = air_parent; }

  size_t GetUIIndexForChild(AirElement* child) const;

  void UpdateUIChildrenCountInParent(int delta);

  float width() const { return width_; }
  float height() const { return height_; }
  float top() const { return top_; }
  float left() const { return left_; }

  inline const std::shared_ptr<PropBundle>& prop_bundle() const {
    return prop_bundle_;
  }

  inline const std::array<float, 4>& borders() { return borders_; }
  inline const std::array<float, 4>& paddings() { return paddings_; }
  inline const std::array<float, 4>& margins() { return margins_; }
  inline float max_height() const { return max_height_; }
  inline void MarkUpdated() { frame_changed_ = false; }

  inline void set_is_layout_only(bool is_layout_only) {
    is_layout_only_ = is_layout_only;
  }

  // flag to indicate whether the element has been removed.
  bool has_been_removed_{false};
  bool frame_changed_ = false;
  // relevant to hierarchy
  // parent is the real node(has element_container,parent presents ui tree)
  AirElement* parent_{nullptr};
  // air_parent is the virtual parent(maybe for_element if_element, air_parent
  // presents virtual tree)
  AirElement* air_parent_{nullptr};
  EventMap static_events_;
  DataMap data_set_;

  AirElementType element_type_ = kAirUnknown;

  AirElementVector dynamic_nodes_ = {};
  SharedAirElementVector air_children_ = {};

 private:
  enum Selector {
    // kSTABLE: global(*) & tag ,only changes on the first screen.
    // kCLASS: associated by class.
    // kID: associated by id.
    // kINLINE: associated by inline style.
    // If a selector changes, it will mark style_dirty_,like 'style_dirty_ |=
    // Selector::kCLASS' , Selector is also the key to cache the current css
    // properties in
    // cur_css_styles.
    kSTABLE = 0x01 << 0,
    kCLASS = 0x01 << 1,
    kID = 0x01 << 2,
    kINLINE = 0x01 << 3
  };

  struct StylePatch {
    // StylePatch is the final result of the diff
    // The css property that needs to be reset after the diff is completed
    std::unordered_set<CSSPropertyID> reset_id_set_;
    // The css property that needs to be reserve after the diff is completed
    StyleMap reserve_styles_map_;
    // The css property that needs to be update after the diff is completed
    StyleMap update_styles_map_;
  };
  bool TendToFlatten() const;
  void PreparePropBundleIfNeed();
  virtual void InsertNodeIndex(AirElement* child, size_t index);

  inline void MarkPlatformNodeDestroyedRecursively();
  bool HasPaintingNode() const { return has_painting_node_; }
  /**
   * + config_enable_layout_only_: 1. LynxViewBuilder.setEnableLayoutOnly, 2.
   * PageConfig decode 'enableNewLayoutOnly'
   *
   * + has_layout_only_props_:
   * SetAttribute、SetEventHandler、view+component
   */
  inline bool CanBeLayoutOnly() const {
    return config_enable_layout_only_ && has_layout_only_props_;
  }

  void CheckHasAnimateProps(CSSPropertyID id);

  // For keyframe op
  bool ResolveKeyframesMap(CSSPropertyID id, const lepus::Value&);
  bool ResolveKeyframesMap(CSSPropertyID id, const std::string& keyframes_name);
  void PushKeyframesToPlatform();

  size_t FindInsertIndex(const SharedAirElementVector& target,
                         AirElement* child);

  // Update styles in 'FlushElement' method.
  // 1. According to style_dirty_(class style dirty , id style dirty , inline
  // style dirty) , Diff with old Styles, Get StylesPatch.
  // 2. Loop call 'SetStyle' to update layout_node_ & props_bundle_ with
  // update_css_map in StylePatch.
  // 3. Loop call 'ResetStyle' to reset layout_node_ & props_bundle_ with
  // reset_css_set in StylePatch.
  void RefreshStyles();
  void UpdateStylePatch(Selector selector, StylePatch& style_patch);
  void DiffStyles(StyleMap& old_map, const StyleMap& new_map,
                  StylePatch& style_patch, bool is_final = false,
                  bool is_dirty = false);
  void SetStyle(CSSPropertyID id, tasm::CSSValue& value);
  void ConsumeStyle(CSSPropertyID id, const tasm::CSSValue& value);
  void ResetStyle(CSSPropertyID id);
  void ComputeCSSStyle(CSSPropertyID id, tasm::CSSValue& css_value);
  void GetStyleMap(Selector selector, StyleMap& result);
  void PushToPropsBundle(const base::String& key, const lepus::Value& value);
  void FlushFontSize();
  bool enable_async_calc_{false};
  bool layout_node_inserted_{false};
  // relevant to flatten
  bool config_flatten_;
  bool has_event_listener_{false};
  bool has_non_flatten_attrs_{false};
  bool has_transition_attrs_{false};
  bool has_font_size_{false};

  bool has_animate_props_{false};

  // relevant to layout only
  bool is_virtual_{false};
  base::String tag_;
  bool has_painting_node_{false};

  Catalyzer* catalyzer_;

  // config settings for enableLayoutOnly
  bool config_enable_layout_only_{true};

  bool has_layout_only_props_{true};

  std::shared_ptr<PropBundle> prop_bundle_{nullptr};

  // Save the keyframes of the Animate API.
  CSSKeyframesTokenMap keyframes_map_;

  // relevant to layout and frame
  float width_{0};
  float height_{0};
  float top_{0};
  float left_{0};
  float last_left_{0};
  float last_top_{0};
  std::array<float, 4> borders_{};
  std::array<float, 4> margins_{};
  std::array<float, 4> paddings_{};
  float max_height_{starlight::DefaultLayoutStyle::kDefaultMaxSize};
  // Determine by Catalyzer
  bool is_layout_only_{0};

  // relevant to native hierarchy about ui
  size_t ui_children_count{0};

  std::unique_ptr<starlight::ComputedCSSStyle> platform_css_style_;

  size_t GetUIChildrenCount() const;

  const uint8_t kDirtyCreated = 0x01 << 0;
  const uint8_t kDirtyTree = 0x01 << 1;
  const uint8_t kDirtyStyle = 0x01 << 2;
  const uint8_t kDirtyAttr = 0x01 << 3;
  uint8_t dirty_{0};
  uint8_t style_dirty_{Selector::kSTABLE};

  int id_;
  uint32_t lepus_id_{0};

  double font_size_;
  double root_font_size_;
  std::vector<std::pair<tasm::CSSPropertyID, tasm::CSSValue>>
      async_resolved_styles_{};
  std::unordered_set<tasm::CSSPropertyID> async_reset_styles_{};

  ElementManager* air_element_manager_;

  SharedAirElementVector children_ = {};

  ClassVector classes_ = {};
  std::string id_selector_;
  // Cache all the css properties of the current Element
  std::unordered_map<Selector, StyleMap> cur_css_styles_ = {};

  StyleMap inline_style_map_;
  RawLepusStyleMap static_inline_style_;

  AirCompStylesMap parsed_styles_;
  std::mutex cal_mutex_;
  std::string dynamic_inline_style_;
  base::LinkedHashMap<base::String, lepus::Value> raw_attributes_{};
  // Use this handler to process styles, related to pattern on the one hand
  // and
  // css_property_id on the other.
  class AirComputedCSSStyle {
   public:
    bool Process(CSSPropertyID css_property_id, CSSValuePattern pattern,
                 lepus::Value& value);

   private:
    bool ProcessWithPattern(CSSValuePattern pattern,
                            lepus::Value& result_value);
    bool ProcessWithID(CSSPropertyID css_property_id, CSSValuePattern pattern,
                       lepus::Value& result_value);
  };

  AirComputedCSSStyle air_computed_css_style_;
};

// for lepus::Value,when there is some lepus::Value use this air_element, this
// value will not be released
class AirLepusRef : public lepus::RefCounted {
 public:
  AirLepusRef(std::shared_ptr<AirElement> lepus_ref) : lepus_ref_(lepus_ref) {}
  AirLepusRef(AirLepusRef* ref) : lepus_ref_(ref->lepus_ref_) {}
  ~AirLepusRef() override = default;

  static fml::RefPtr<AirLepusRef> Create(
      std::shared_ptr<AirElement> lepus_ref) {
    return fml::AdoptRef(new AirLepusRef(lepus_ref));
  }

  static fml::RefPtr<AirLepusRef> Create(AirLepusRef* ref) {
    return fml::AdoptRef(new AirLepusRef(ref));
  }

  lepus::RefType GetRefType() const override {
    return lepus::RefType::kElement;
  };

  AirElement* operator->() const { return lepus_ref_.get(); }

  AirElement* Get() const { return lepus_ref_.get(); }
  void ReleaseSelf() const override { delete this; }

 private:
  std::shared_ptr<AirElement> lepus_ref_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_ELEMENT_H_
