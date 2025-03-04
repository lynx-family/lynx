// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ELEMENT_MANAGER_H_
#define CORE_RENDERER_DOM_ELEMENT_MANAGER_H_

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/base/utils/any.h"
#include "core/inspector/observer/inspector_element_observer.h"
#include "core/inspector/style_sheet.h"
#include "core/public/pipeline_option.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_variable_handler.h"
#include "core/renderer/dom/css_patching.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_container.h"
#include "core/renderer/dom/element_vsync_proxy.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/vdom/radon/radon_element.h"
#include "core/renderer/dom/vdom/radon/radon_types.h"
#include "core/renderer/page_config.h"
#include "core/renderer/ui_wrapper/common/prop_bundle_creator_default.h"
#include "core/renderer/ui_wrapper/layout/layout_context.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/renderer/utils/base/tasm_worker_task_runner.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/timing_handler/timing_handler.h"

namespace lynx {
namespace shell {
class VSyncMonitor;
}  // namespace shell
namespace tasm {

struct PseudoPlaceHolderStyles;
class PaintingContext;
class PropBundle;
class Element;
class FiberElement;
class ComponentElement;
class ImageElement;
class ListElement;
class NoneElement;
class ScrollElement;
class TextElement;
class RawTextElement;
class ViewElement;
class WrapperElement;
class Catalyzer;
class ElementCache;
class LynxEnvConfig;
class AirElement;
class AirLepusRef;
class AirPageElement;
class TemplateAssembler;

class HierarchyObserver {
 public:
  virtual ~HierarchyObserver() {}

  virtual void OnLayoutNodeCreated(int32_t id, LayoutNode *ptr) {}
  virtual void OnLayoutNodeDestroy(int32_t id) {}
  virtual void OnComponentUselessUpdate(const std::string &component_name,
                                        const lepus::Value &properties) {}
};

class NodeManager {
 public:
  NodeManager() = default;
  ~NodeManager() = default;
  inline void Record(int id, Element *node) { node_map_[id] = node; }

  inline void Erase(int id) { node_map_.erase(id); }

  inline Element *Get(int tag) {
    auto it = node_map_.find(tag);
    if (it != node_map_.end()) {
      return it->second;
    }
    return nullptr;
  }

  void WillDestroy() {
    for (const auto &pair : node_map_) {
      if (pair.second) {
        pair.second->set_will_destroy(true);
      }
    }
    node_map_.clear();
  }

 private:
  std::unordered_map<int, Element *> node_map_;
};

/*
 * ComponentManager is used to map component id into element.
 * ComponentManager is a field of ElementManager.
 */
class ComponentManager {
 public:
  ComponentManager() = default;
  ~ComponentManager() = default;

  inline void Record(const std::string &id, Element *node) {
    component_map_[id] = node;
  }

  inline void Erase(const std::string &id, Element *node) {
    // see issue:#8417, if the component element corresponding to the deleted id
    // is not same as the current element, the deletion operation will not be
    // performed.
    auto iter = component_map_.find(id);
    if (iter == component_map_.end()) {
      return;
    }
    if (node == iter->second) {
      component_map_.erase(iter);
    }
  }

  inline Element *Get(const std::string &id) {
    auto it = component_map_.find(id);
    if (it != component_map_.end()) {
      return it->second;
    }
    return nullptr;
  }

 private:
  std::unordered_map<std::string, Element *> component_map_;
};

class AirNodeManager {
 public:
  AirNodeManager() = default;
  ~AirNodeManager() = default;

#if ENABLE_AIR
  inline void Record(int id, const std::shared_ptr<AirElement> &node) {
    air_node_map_[id] = node;
  }

  inline bool IsActive() const { return !(air_node_map_.empty()); }

  void RecordForLepusId(int id, uint64_t key, fml::RefPtr<AirLepusRef> node);

  inline void RecordCustomId(const std::string &id, int tag) {
    air_customize_id_map_[id] = tag;
  }

  inline void Erase(int id) { air_node_map_.erase(id); }

  inline void EraseCustomId(const std::string &id) {
    air_customize_id_map_.erase(id);
  }

  void EraseLepusId(int id, AirElement *node);

  inline std::shared_ptr<AirElement> Get(int tag) const {
    auto it = air_node_map_.find(tag);
    if (it != air_node_map_.end()) {
      return it->second;
    }
    return nullptr;
  }

  fml::RefPtr<AirLepusRef> GetForLepusId(int tag, uint64_t key);

  std::vector<fml::RefPtr<AirLepusRef>> GetAllNodesForLepusId(int tag) const;

  inline const std::shared_ptr<AirElement> GetCustomId(
      const std::string &tag) const {
    auto it = air_customize_id_map_.find(tag);
    if (it != air_customize_id_map_.end()) {
      return Get(it->second);
    }
    return nullptr;
  }

 private:
  std::unordered_map<int, std::shared_ptr<AirElement>> air_node_map_;
  std::unordered_map<int, std::map<uint64_t, fml::RefPtr<AirLepusRef>>>
      air_lepus_id_map_;
  std::unordered_map<std::string, int> air_customize_id_map_;
#endif
};

class ElementManager {
 public:
  class Delegate {
   public:
    Delegate() = default;
    virtual ~Delegate() = default;

    virtual void DispatchLayoutUpdates(const PipelineOptions &options) = 0;
    virtual std::unordered_map<int32_t, LayoutInfoArray> GetSubTreeLayoutInfo(
        int32_t root_id, Viewport viewport = Viewport{}) = 0;

    virtual void SetEnableLayout() = 0;

    virtual void CreateLayoutNode(int32_t id, const base::String &tag) = 0;

    virtual void UpdateLayoutNodeFontSize(int32_t id, double cur_node_font_size,
                                          double root_node_font_size,
                                          double font_scale) = 0;
    virtual void InsertLayoutNode(int32_t parent_id, int32_t child_id,
                                  int index) = 0;
    virtual void SendAnimationEvent(const char *type, int tag,
                                    const lepus::Value &dict) = 0;
    virtual void RemoveLayoutNodeAtIndex(int32_t parent_id, int index) = 0;
    virtual void MoveLayoutNode(int32_t parent_id, int32_t child_id,
                                int from_index, int to_index) = 0;
    virtual void SendNativeCustomEvent(const std::string &name, int tag,
                                       const lepus::Value &param_value,
                                       const std::string &param_name) = 0;
    virtual void InsertLayoutNodeBefore(int32_t parent_id, int32_t child_id,
                                        int32_t ref_id) = 0;
    virtual void RemoveLayoutNode(int32_t parent_id, int32_t child_id) = 0;
    virtual void DestroyLayoutNode(int32_t id) = 0;
    virtual void UpdateLayoutNodeStyle(int32_t id, CSSPropertyID css_id,
                                       const tasm::CSSValue &value) = 0;
    virtual void ResetLayoutNodeStyle(int32_t id, CSSPropertyID css_id) = 0;
    virtual void UpdateLayoutNodeAttribute(int32_t id,
                                           starlight::LayoutAttribute key,
                                           const lepus::Value &value) = 0;
    virtual void SetFontFaces(const CSSFontFaceRuleMap &fontfaces) = 0;

    virtual void UpdateLayoutNodeByBundle(
        int32_t id, std::unique_ptr<LayoutBundle> bundle) = 0;

    virtual void UpdateLayoutNodeProps(
        int32_t id, const std::shared_ptr<PropBundle> &props) = 0;
    virtual void MarkLayoutDirty(int32_t id) = 0;
    virtual void AttachLayoutNodeType(
        int32_t id, const base::String &tag, bool allow_inline,
        const std::shared_ptr<PropBundle> &props) = 0;
    virtual void UpdateLynxEnvForLayoutThread(LynxEnvConfig env) = 0;
    virtual void OnUpdateViewport(float width, int width_mode, float height,
                                  int height_mode, bool need_layout) = 0;
    virtual void SetRootOnLayout(int32_t id) = 0;

    virtual void OnUpdateDataWithoutChange() = 0;
    virtual void SetPageConfigForLayoutThread(
        const std::shared_ptr<PageConfig> &config) = 0;

    virtual void OnErrorOccurred(base::LynxError error) = 0;

    virtual void BindPipelineIDWithTimingFlag(
        const tasm::PipelineID &pipeline_id,
        const tasm::timing::TimingFlag &timing_flag) = 0;
  };

  ElementManager(
      std::unique_ptr<PaintingCtxPlatformImpl> platform_painting_context,
      Delegate *delegate, const LynxEnvConfig &lynx_env_config,
      int32_t instance_id = tasm::report::kUnknownInstanceId,
      const std::shared_ptr<shell::VSyncMonitor> &vsync_monitor = nullptr,
      const bool enable_diff_without_layout = false);

  // avoid pImpl idiom type of compilation error when self inlclude
  // std::unique_ptr object
  virtual ~ElementManager();

  virtual fml::RefPtr<RadonElement> CreateNode(
      const base::String &tag, const std::shared_ptr<AttributeHolder> &node,
      uint32_t node_index = 0,
      RadonNodeType radon_node_type = RadonNodeType::kRadonUnknown);

  BASE_EXPORT_FOR_DEVTOOL void OnFinishUpdateProps(Element *node,
                                                   PipelineOptions &options);

  void PatchEventRelatedInfo();

  bool GetDevToolFlag() { return devtool_flag_; }

  // for air only, these functions won't be used when ENABLE_AIR is off, so link
  // process works normally even if ENABLE_AIR is off.
  fml::RefPtr<AirLepusRef> GetAirNode(const base::String &tag,
                                      int32_t lepus_id);
  fml::RefPtr<AirLepusRef> CreateAirNode(const base::String &tag,
                                         int32_t lepus_id, int32_t impl_id,
                                         uint64_t key);
  AirPageElement *CreateAirPage(int32_t lepus_id);
  inline void SetAirRoot(AirPageElement *node) { air_root_ = node; }
  AirPageElement *AirRoot() { return air_root_; }
  void OnPatchFinishInnerForAir(const PipelineOptions &option);
  // for air end

  PaintingContext *painting_context();
  inline Catalyzer *catalyzer() { return catalyzer_.get(); }
  inline NodeManager *node_manager() { return node_manager_.get(); }
  inline AirNodeManager *air_node_manager() { return air_node_manager_.get(); }
  inline void SetRoot(Element *node) { root_ = node; }
  Element *root() { return root_; }

  void RecordComponent(const std::string &id, Element *node);
  void EraseComponentRecord(const std::string &id, Element *node);
  Element *GetComponent(const std::string &id);

  void ResolveAttributesAndStyle(AttributeHolder *node, Element *shadow_node,
                                 const StyleMap &styles);

  void ResolveEvents(AttributeHolder *node, Element *element);

  // resolve gesture detector from attribute holder
  void ResolveGestures(AttributeHolder *node, Element *element);

  void UpdateScreenMetrics(float width, float height);
  void UpdateFontScale(float font_scale);
  void UpdateViewport(float width, SLMeasureMode width_mode_, float height,
                      SLMeasureMode height_mode, bool need_layout);

  BASE_EXPORT_FOR_DEVTOOL void SetInspectorElementObserver(
      const std::shared_ptr<InspectorElementObserver>
          &inspector_element_observer);

  void OnUpdateViewport(float width, int width_mode, float height,
                        int height_mode, bool need_layout);
  BASE_EXPORT_FOR_DEVTOOL void SetRootOnLayout(int32_t id);
#if ENABLE_TESTBENCH_RECORDER
  void SetRecordId(int64_t record_id) { record_id_ = record_id; }
#endif

  // delegate for class element
  void CreateLayoutNode(int id, const base::String &tag);

  void UpdateLayoutNodeFontSize(int32_t id, double cur_node_font_size,
                                double root_node_font_size);
  void InsertLayoutNode(int32_t parent_id, int32_t child_id, int index);
  void RemoveLayoutNodeAtIndex(int32_t parent_id, int index);
  void InsertLayoutNodeBefore(int32_t parent_id, int32_t child_id,
                              int32_t ref_id);
  void RemoveLayoutNode(int32_t parent_id, int32_t child_id);
  void DestroyLayoutNode(int32_t id);
  void SendAnimationEvent(const char *type, int tag, const lepus::Value &dict);
  void MoveLayoutNode(int32_t parent_id, int32_t child_id, int from_index,
                      int to_index);
  void UpdateLayoutNodeStyle(int32_t id, tasm::CSSPropertyID css_id,
                             const tasm::CSSValue &value);
  void SendNativeCustomEvent(const std::string &name, int tag,
                             const lepus::Value &param_value,
                             const std::string &param_name);
  void ResetLayoutNodeStyle(int32_t id, tasm::CSSPropertyID css_id);
  void UpdateLayoutNodeAttribute(int32_t id, starlight::LayoutAttribute key,
                                 const lepus::Value &value);
  void SetFontFaces(const tasm::CSSFontFaceRuleMap &fontfaces);
  void AddFontFace(const lepus::Value &font);
  void UpdateLayoutNodeByBundle(int32_t id,
                                std::unique_ptr<LayoutBundle> bundle);
  void UpdateLayoutNodeProps(int32_t id,
                             const std::shared_ptr<tasm::PropBundle> &props);
  int32_t GetNodeInfoByTag(const base::String &tag_name);
  bool IsShadowNodeVirtual(const base::String &tag_name);

  void MarkLayoutDirty(int32_t id);
  void AttachLayoutNodeType(int32_t id, const base::String &tag,
                            bool allow_inline,
                            const std::shared_ptr<PropBundle> &props);

  void UpdateTouchPseudoStatus(bool value);
  std::unordered_map<int32_t, LayoutInfoArray> GetSubTreeLayoutInfo(
      int32_t root_id, Viewport viewport = Viewport{});

  /**
   * DevTool Related.
   * When page is created, call this API to notify devtool that page is updated
   * and node ids are no longer valid.
   */
  void OnDocumentUpdated();

  /**
   * DevTool Related.
   */
  void OnElementManagerWillDestroy();

  /**
   * Devtool Related.
   */
  void OnElementNodeAddedForInspector(Element *element);

  /**
   * DevTool Related.
   */
  void OnElementNodeRemovedForInspector(Element *element);

  /**
   * DevTool Related.
   */
  void OnElementNodeSetForInspector(Element *element);

  /**
   * DevTool Related.
   */
  void OnCSSStyleSheetAddedForInspector(Element *element);

  /**
   * DevTool Related.
   */
  void OnComponentUselessUpdate(const std::string &component_name,
                                const lepus::Value &properties);

  /**
   * DevTool Related.
   */
  void OnSetNativeProps(tasm::Element *ptr, const std::string &name,
                        const lepus::Value &value, bool is_style);

  /**
   * DevTool Related.
   */
  void RunDevToolFunction(lynx::devtool::DevToolFunction func_enum,
                          const base::any &data);

  virtual int GetInstanceId() { return instance_id_; }

  const tasm::DynamicCSSConfigs &GetDynamicCSSConfigs() {
    if (config_) {
      return config_->GetDynamicCSSConfigs();
    }
    return DynamicCSSConfigs::GetDefaultDynamicCSSConfigs();
  }

  const tasm::CSSParserConfigs &GetCSSParserConfigs() {
    if (config_) {
      return config_->GetCSSParserConfigs();
    }
    static base::NoDestructor<tasm::CSSParserConfigs> kDefaultCSSConfigs;
    return *kDefaultCSSConfigs;
  }

  bool GetEnableLayoutOnly() {
    if (config_) {
      return config_->GetEnableNewLayoutOnly() && enable_layout_only_;
    }
    return enable_layout_only_;
  }

  bool GetEnableComponentLayoutOnly() {
    if (config_) {
      return config_->GetEnableComponentLayoutOnly();
    }
    return false;
  }

  bool GetEnableExtendedLayoutOnlyOpt() {
    if (config_) {
      return config_->GetEnableExtendedLayoutOpt();
    }
    return false;
  }

  bool GetEnableStandardCSSSelector() {
    if (config_) {
      return config_->GetEnableStandardCSSSelector();
    }
    return false;
  }

  void SetEnableLayoutOnly(bool enable) { enable_layout_only_ = enable; }

  void SetThreadStrategy(int thread_strategy) {
    thread_strategy_ = thread_strategy;
  }

  void SetEnableNewAnimatorRadon(bool enable) {
    enable_new_animator_radon_ = enable;
  }

  bool GetEnableNewAnimatorForRadon() { return enable_new_animator_radon_; }

  void SetEnableNewAnimatorFiber(bool enable) {
    enable_new_animator_fiber_ = enable;
  }

  bool GetEnableNewAnimatorForFiber() {
    if (config_) {
      return config_->GetEnableNewAnimator();
    }
    return LynxEnv::GetInstance().EnableNewAnimatorFiber();
  }

  bool GetParallelWithSyncLayout() { return parallel_with_sync_layout_; }

  void SetConfig(const std::shared_ptr<PageConfig> &config);

  const std::shared_ptr<PageConfig> &GetConfig() { return config_; }

  bool GetEnableCSSLazyImport() {
    if (config_) {
      return config_->GetEnableCSSLazyImport();
    }
    // enableCSSLazyImport default value is false.
    return false;
  }

  bool GetPageFlatten() {
    if (config_) {
      return config_->GetGlobalFlattern();
    }
    return true;
  }

  starlight::LayoutConfigs GetLayoutConfigs() {
    if (config_) {
      return config_->GetLayoutConfigs();
    }
    return starlight::LayoutConfigs();
  }

  bool GetRemoveComponentElement() {
    if (config_) {
      return config_->GetRemoveComponentElement();
    }
    return false;
  }

  bool GetEnableSavePageData() {
    if (config_) {
      return config_->GetEnableSavePageData();
    }
    return false;
  }

  bool GetEnableCheckDataWhenUpdatePage() {
    if (config_) {
      return config_->GetEnableCheckDataWhenUpdatePage();
    }
    return true;
  }

  bool GetListNewArchitecture() {
    if (config_) {
      return config_->GetListNewArchitecture();
    }
    return false;
  }

  void SetEnableNativeListFromShell(bool enable) {
    enable_native_list_ = enable;
  }

  bool GetEnableNativeListFromShell() const { return enable_native_list_; }

  bool GetEnableNativeListFromPageConfig() const {
    return config_ && config_->GetEnableNativeList();
  }

  bool GetEnableNewGesture() {
    if (config_) {
      return config_->GetEnableNewGesture();
    }
    return false;
  }

  bool UseFiberElement() {
    return GetEnableFiberArch() || GetEnableFiberElementForRadonDiff();
  }

  bool GetEnableFiberArch() {
    if (config_) {
      return config_->GetEnableFiberArch();
    }
    return false;
  }

  bool GetEnableUseMapBuffer() {
    if (config_) {
      TernaryBool enabled = config_->GetEnableUseMapBuffer();
      if (enabled != TernaryBool::UNDEFINE_VALUE) {
        // if pageConfig existed, use user-specified config
        return enabled == TernaryBool::TRUE_VALUE;
      }
    }
    return settings_enable_use_mapbuffer_for_ui_op_;
  }

  bool GetCSSInheritance() {
    if (config_) {
      return config_->GetEnableCSSInheritance();
    }
    return false;
  }

  bool GetListRemoveComponent() {
    if (config_) {
      return config_->GetListRemoveComponent();
    }
    return false;
  }

  bool GetListEnableMoveOperation() {
    if (config_) {
      return config_->GetEnableListMoveOperation();
    }
    return false;
  }

  bool GetListEnablePlug() {
    if (config_) {
      return config_->list_enable_plug();
    }
    return false;
  }

  bool GetDefaultOverflowVisible() {
    if (painting_context()->DefaultOverflowAlwaysVisible()) {
      return true;
    }
    return config_ ? config_->GetDefaultOverflowVisible() : false;
  }

  bool GetDefaultTextOverflow() {
    return config_ ? config_->GetEnableTextOverflow() : false;
  }

  bool GetDefaultDisplayLinear() {
    return config_ ? config_->GetDefaultDisplayLinear() : false;
  }

  bool GetDefaultCssAlignWithLegacyW3C() {
    return config_ ? config_->GetCSSAlignWithLegacyW3C() : false;
  }

  bool GetEnableComponentLifecycleAlignWebview() {
    return config_ ? config_->GetEnableComponentLifecycleAlignWebview() : false;
  }

  bool GetStrictPropType() {
    return config_ ? config_->GetStrictPropType() : false;
  }

  bool GetKeyboardCallbackUseRelativeHeight() {
    return config_ ? config_->GetKeyboardCallbackUseRelativeHeight() : false;
  }

  bool GetForceCalcNewStyle() {
    return config_ ? config_->GetForceCalcNewStyle() : true;
  }

  bool GetCompileRender() {
    return config_ ? config_->GetCompileRender() : false;
  }

  void InsertPlug(Element *plug) {
    current_insert_plug_vector_.push_back(plug);
  }

  std::vector<Element *> &GetCurrentInsertPlugVector() {
    return current_insert_plug_vector_;
  }

  virtual bool IsDomTreeEnabled() { return dom_tree_enabled_; }
  bool GetEnableZIndex() { return config_ && config_->GetEnableZIndex(); }

  void InsertDirtyContext(ElementContainer *stacking_context) {
    dirty_stacking_contexts_.insert(stacking_context);
  }

  void RemoveDirtyContext(ElementContainer *stacking_context) {
    auto it = dirty_stacking_contexts_.find(stacking_context);
    if (it != dirty_stacking_contexts_.end())
      dirty_stacking_contexts_.erase(it);
  }

  std::string GetTargetSdkVersion() {
    return config_ ? config_->GetTargetSDKVersion() : "";
  }

  bool GetEnableReactOnlyPropsId() {
    return config_ ? config_->GetEnableReactOnlyPropsId() : false;
  }

  bool GetEnableGlobalComponentMap() {
    return config_ ? config_->GetEnableGlobalComponentMap() : false;
  }

  bool GetEnableRemoveComponentExtraData() {
    return config_ ? config_->GetEnableRemoveComponentExtraData() : false;
  }

  bool GetIsTargetSdkVerionHigherThan21() const {
    return config_ ? config_->GetIsTargetSdkVerionHigherThan21() : false;
  }

  bool GetEnableReduceInitDataCopy() {
    return config_ ? config_->GetEnableReduceInitDataCopy() : false;
  }

  bool GetEnableCascadePseudo() {
    return config_ ? config_->GetEnableCascadePseudo() : false;
  }

  bool GetEnableRasterAnimation() {
    return config_ ? config_->GetEnableRasterAnimation() : false;
  }

  bool GetEnableComponentNullProp() {
    return config_ ? config_->GetEnableComponentNullProp() : false;
  }

  LynxEnvConfig &GetLynxEnvConfig() { return lynx_env_config_; }

  const LynxEnvConfig &GetLynxEnvConfig() const { return lynx_env_config_; }

  bool GetRemoveDescendantSelectorScope() const {
    return config_ ? config_->GetRemoveDescendantSelectorScope() : false;
  }

  bool GetEnableFixedNew() const {
    return config_ && config_->GetEnableFixedNew();
  }

  bool GetEnableReloadLifecycle() const {
    return config_ ? config_->GetEnableReloadLifecycle() : false;
  }

  std::shared_ptr<shell::VSyncMonitor> &vsync_monitor() {
    return vsync_monitor_;
  }

  bool Hydrate(AttributeHolder *node, Element *shadow_node);

  void SetGlobalBindElementId(const base::String &name,
                              const base::String &type, const int node_id);

  std::set<int> GetGlobalBindElementIds(const std::string &name) const;

  void EraseGlobalBindElementId(const EventMap &global_event_map,
                                const int node_id);

  void AppendTimingFlag(std::string flag);

  auto ObtainTimingFlagList() { return attribute_timing_flag_list_.PopAll(); }

  void BindTimingFlagToPipelineOptions(PipelineOptions &options);

  // Element notify element_manager to trigger layout.
  void SetNeedsLayout();

  // Element notify element_manager to regist itself to set.
  void RequestNextFrame(Element *element);

  // Element notify element_manager to logout itself from set.
  void NotifyElementDestroy(Element *element);

  // Tick all element need to animated.
  void TickAllElement(fml::TimePoint &time);

  // Pause all element.
  void PauseAllAnimations();

  // Resume paused element.
  void ResumeAllAnimations();

  bool IsPause() const { return animations_paused_; }

  bool IsAirModeFiberEnabled() const {
    return config_ ? (config_->GetLynxAirMode() ==
                      CompileOptionAirMode::AIR_MODE_FIBER)
                   : false;
  }

  // for Fiber Element related
  /**
   * Create a Fiber Element based on the raw tag name.
   * This method converts the raw tag name string to an enum tag and then calls
   * the corresponding method to create the specific Fiber Element.
   *
   * @param raw_tag The raw tag name of the Dom Element.
   * @return The refCounted FiberElement.
   */
  fml::RefPtr<FiberElement> CreateFiberElement(const base::String &raw_tag);
  /**
   * Create a Fiber Element based on the enum tag and default tag name.
   * This method uses a switch statement to decide which specific method to call
   * to create the Fiber Element based on the enum tag.
   *
   * @param enum_tag The enum representation of the tag name.
   * @param raw_tag The raw tag name of the Dom Element.
   * @return The refCounted FiberElement.
   * Note: If the enum tag is 'ELEMENT_EMPTY', the Fiber Element will be created
   * using the raw tag name.
   */
  fml::RefPtr<FiberElement> CreateFiberElement(
      ElementBuiltInTagEnum enum_tag,
      const base::String &raw_tag = base::String());

  /**
   * A static method to create a Fiber Element based on the enum tag and default
   * tag name. This method uses a switch statement to decide which specific
   * method to call to create the Fiber Element based on the enum tag.
   *
   * @param enum_tag The enum representation of the tag name.
   * @param raw_tag The raw tag name of the Dom Element.
   * @return The refCounted FiberElement.
   * Note: If the enum tag is 'ELEMENT_EMPTY', the Fiber Element will be created
   * using the raw tag name.
   */
  static fml::RefPtr<FiberElement> StaticCreateFiberElement(
      ElementBuiltInTagEnum enum_tag,
      const base::String &raw_tag = base::String());

  /**
   * create common Element via tag name
   * @param tag the tag name of Dom Element
   * @return  the refCounted type
   */
  fml::RefPtr<FiberElement> CreateFiberNode(const base::String &tag);
  /**
   * create Page Element for fiber
   * @param component_id the component id for Page
   * @param css_id the css_id for getting StyleSheet for Page
   * @return the refCounted type
   */
  fml::RefPtr<PageElement> CreateFiberPage(const base::String &component_id,
                                           int32_t css_id);

  /**
   * create Component Element for fiber
   * @param component_id the component id for specific Component
   * @param css_id  the css_id for getting StyleSheet for current component
   * @param entry_name the entry_name for current component
   * @param name the component name
   * @param path the component path
   * @return the refCounted type
   */
  fml::RefPtr<ComponentElement> CreateFiberComponent(
      const base::String &component_id, int32_t css_id,
      const base::String &entry_name, const base::String &name,
      const base::String &path);

  /**
   * create View Element
   * @return the refCounted type
   */
  fml::RefPtr<ViewElement> CreateFiberView();

  /**
   * create Text Element
   * @param tag the tag name of Image Element, such as "image", "x-image"
   * @return the refCounted type
   */
  fml::RefPtr<ImageElement> CreateFiberImage(const base::String &tag);

  /**
   * create Text Element
   * @param tag the tag name of Image Element, such as "text", "x-text"
   * @return the refCounted type
   */
  fml::RefPtr<TextElement> CreateFiberText(const base::String &tag);

  /**
   * create Raw Text Element
   * @return the refCounted type
   */
  fml::RefPtr<RawTextElement> CreateFiberRawText();

  /**
   * create Scroll Element
   * @param tag the tag name of Image Element, such as "scroll-view",
   * "x-scroll-view"
   * @return the refCounted type
   */
  fml::RefPtr<ScrollElement> CreateFiberScrollView(const base::String &tag);

  /**
   * create List element for fiber
   * @param tasm the template_assembler instance
   * @param component_at_index (Ref list, Number listID, Number cellIndex,
   * Number opID)=>{}
   * @param enqueue_component  (Ref list, Number listID, Number
   * elementUniqueID)=>{}
   * @param component_at_indexes (Ref list, Number listID, Number[] cellIndexes,
   * Number[] opIDs)=>{}
   * @return the refCounted type
   */
  fml::RefPtr<ListElement> CreateFiberList(
      tasm::TemplateAssembler *tasm, const base::String &tag,
      const lepus::Value &component_at_index,
      const lepus::Value &enqueue_component,
      const lepus::Value &component_at_indexes);

  /**
   * create None Element, it's just meaningless Node
   * @return the refCounted type
   */
  fml::RefPtr<NoneElement> CreateFiberNoneElement();

  /**
   * create Wrapper Element, it's just meaningless Node
   * @return the refCounted type
   */
  fml::RefPtr<WrapperElement> CreateFiberWrapperElement();

  BASE_EXPORT_FOR_DEVTOOL void OnPatchFinish(PipelineOptions &option,
                                             Element *root = nullptr);

  /**
   * Generate ID for element
   */
  int32_t GenerateElementID();
  /**
   * Reuse ID for element when hydrate element bundle
   */
  void ReuseElementID(int32_t reuse_id);

  starlight::ComputedCSSStyle *platform_computed_css() {
    return platform_computed_css_.get();
  }

  /**
   * Prepare node for inspector
   * @param element the element which will be processed
   */
  void PrepareNodeForInspector(Element *element);

  /**
   * Recursively attach an fiber element tree to the inspector
   * @param root The root node of the fiber element tree
   */
  void FiberAttachToInspectorRecursively(FiberElement *root);

  /**
   * Prepare slot and plug node for inspector
   * @param element the element which will be processed
   */
  void CheckAndProcessSlotForInspector(Element *element);

  void SetEnableParallelElement(bool value) {
    enable_parallel_element_ = value;
    if (enable_parallel_element_ &&
        (thread_strategy_ == base::ThreadStrategyForRendering::ALL_ON_UI ||
         thread_strategy_ == base::ThreadStrategyForRendering::MOST_ON_TASM)) {
      parallel_with_sync_layout_ = true;
    }
  }

  bool GetEnableParallelElement() { return enable_parallel_element_; }

  bool GetEnableReportThreadedElementFlushStatistic() {
    return enable_report_threaded_element_flush_statistic_;
  }

  void SetEnableReportThreadedElementFlushStatistic(bool value) {
    enable_report_threaded_element_flush_statistic_ = value;
  }

  bool GetEnableFiberElementForRadonDiff() {
    return enable_fiber_element_for_radon_diff_;
  }

  void SetEnableFiberElementForRadonDiff(TernaryBool value);

  void SetEnableDumpElementTree(bool enable) {
    enable_dump_element_tree_ = enable;
  }
  bool GetEnableDumpElementTree() { return enable_dump_element_tree_; }

  void SetFiberPageElement(const fml::RefPtr<PageElement> &page) {
    if (fiber_page_) {
      fiber_page_->set_will_destroy(true);
      node_manager()->Erase(fiber_page_->impl_id());
    }

    fiber_page_ = page;
  }

  PageElement *GetPageElement() const { return fiber_page_.get(); }

  bool CheckResolvedKeyframes(const std::string &unique_id) {
    return resolved_keyframes_set_.find(unique_id) !=
           resolved_keyframes_set_.end();
  }

  void SetResolvedKeyframes(const std::string &unique_id) {
    resolved_keyframes_set_.insert(unique_id);
  }

  void SetPropBundleCreator(
      const std::shared_ptr<tasm::PropBundleCreator> &prop_bundle_creator) {
    prop_bundle_creator_ = prop_bundle_creator;
  }
  std::shared_ptr<tasm::PropBundleCreator> GetPropBundleCreator() const {
    return prop_bundle_creator_;
  }

  std::list<base::OnceTaskRefptr<ParallelFlushReturn>> &ParallelTasks() {
    return parallel_task_queue_;
  }

  std::list<base::OnceTaskRefptr<ParallelFlushReturn>> &
  ParallelResolveTreeTasks() {
    return parallel_resolve_tree_tasks_queue_;
  }

  std::shared_ptr<tasm::TasmWorkerTaskRunner> GetTasmWorkerTaskRunner() const {
    return task_runner_;
  }

  void SetEnableUIOperationOptimize(TernaryBool enable);

  inline void IncreaseElementCount() { element_count_++; }

  inline void IncreaseLayoutOnlyElementCount() { layout_only_element_count_++; }

  inline void IncreaseLayoutOnlyTransitionCount() {
    layout_only_transition_count_++;
  }

  inline void DecreaseLayoutOnlyElementCount() { layout_only_element_count_--; }

  void AddPausedAnimationElement(Element *element) {
    paused_animation_element_set_.insert(element);
  }

  void OnErrorOccurred(base::LynxError error);

  /**
   * Clear extreme parsed style for the entire tree to make CSS to be fully
   * resolved.
   */
  void ClearExtremeParsedStyles();

 protected:
  /**
   * call this function to request layout
   * @param options the pipeline options passed to layout context
   */
  void RequestLayout(const PipelineOptions &options);

  /**
   * call this function after exec OnPatchFinishForFiber
   */
  void DidPatchFinishForFiber();

  void PrepareComponentNodeForInspector(Element *component);

  std::unique_ptr<NodeManager> node_manager_;
  std::unique_ptr<AirNodeManager> air_node_manager_;
  std::unique_ptr<ComponentManager> component_manager_;
  std::unique_ptr<Catalyzer> catalyzer_;
  Element *root_;
  AirPageElement *air_root_{nullptr};
  std::weak_ptr<HierarchyObserver> hierarchy_observer_;
  std::shared_ptr<InspectorElementObserver> inspector_element_observer_;

 private:
  // Do not call this function directly; it needs to be called from
  // OnPatchFinish.
  void OnPatchFinishForRadon(PipelineOptions &option);
  /**
   * a special onPatchFinish function for fiber
   * @param option options for onPatchFinish
   */
  // Do not call this function directly; it needs to be called from
  // OnPatchFinish.
  void OnPatchFinishForFiber(PipelineOptions &option,
                             FiberElement *root = nullptr);
  void WillDestroy();
  ElementManager(const ElementManager &) = delete;
  ElementManager &operator=(const ElementManager &) = delete;
  void OnListComponentUpdated(const PipelineOptions &options);
  void DispatchLayoutUpdates(const tasm::PipelineOptions &options);
  CSSFragment *preresolving_style_sheet_ = nullptr;
  bool devtool_flag_ = false;
  bool dom_tree_enabled_ = true;
  const int instance_id_;
  std::shared_ptr<PageConfig> config_;

  std::vector<Element *> current_insert_plug_vector_;
  bool enable_layout_only_{true};
  std::unordered_set<ElementContainer *> dirty_stacking_contexts_;
  LynxEnvConfig lynx_env_config_;
  Delegate *delegate_{nullptr};
  std::shared_ptr<shell::VSyncMonitor> vsync_monitor_{nullptr};
  std::unordered_map<base::String, int> node_type_recorder_;
  // <page>,<wrapper>,<none> is a special tag and must be COMMON.
  std::unordered_map<base::String, int32_t> node_info_recorder_{
      {BASE_STATIC_STRING(kElementPageTag), LayoutNodeType::COMMON},
      {BASE_STATIC_STRING(kElementWrapperElementTag), LayoutNodeType::COMMON},
      {BASE_STATIC_STRING(kElementNoneElementTag), LayoutNodeType::COMMON}};
  std::unordered_map<std::string, std::set<int32_t>> global_bind_name_to_ids_;
  std::shared_ptr<tasm::PropBundleCreator> prop_bundle_creator_ =
      std::make_shared<lynx::tasm::PropBundleCreatorDefault>();

  // This set holds the unique_id of the already flushed keyframes to ensure
  // that they are not flushed repeatedly.
  std::unordered_set<std::string> resolved_keyframes_set_;

  // Animation proxy class
  std::shared_ptr<ElementVsyncProxy> element_vsync_proxy_;
  // Animation pause flag
  bool animations_paused_ = false;
  // Save paused Animation Elements.
  std::unordered_set<tasm::Element *> paused_animation_element_set_;

  // If it has been set to 'true', OnPatchFinish will not trigger layout
  // anymore, platform must trigger layout manually.
  bool enable_diff_without_layout_ = false;
  // If this flag is true, it indicates that when exec the next patchfinish
  // operation, additional information related to pseudo-class will be pushed to
  // the platform.
  bool push_touch_pseudo_flag_{false};

  // Indicate if need to do layout for current OnPatchFinish process
  bool need_layout_{false};
  // Current thread strategy
  int thread_strategy_;

  // Enable new animator for current lynx view by default for radon/fiber, the
  // initial values here are defined to show the default values and serve as a
  // backend. Use ElementManager::GetEnableNewAnimatorForRadon/Fiber() to read
  // enable_new_animator_radon/fiber_ when need its value.
  bool enable_new_animator_radon_{false};
  bool enable_new_animator_fiber_{true};

  bool enable_native_list_{false};
  // Indicate whether in parallel-element mode with sync layout(ALL_ON_UI,
  // MOST_ON_TASM) strategy
  bool parallel_with_sync_layout_{false};

  ALLOW_UNUSED_TYPE int64_t record_id_ = 0;

  fml::RefPtr<PageElement> fiber_page_{};

  int32_t element_id_{kInitialImplId};
  std::unique_ptr<starlight::ComputedCSSStyle> platform_computed_css_;
  std::unordered_set<tasm::Element *> animation_element_set_;

  bool enable_dump_element_tree_{false};

  bool enable_parallel_element_{false};

  bool enable_report_threaded_element_flush_statistic_{false};

  bool enable_fiber_element_for_radon_diff_{false};

  std::list<base::OnceTaskRefptr<ParallelFlushReturn>> parallel_task_queue_{};

  std::list<base::OnceTaskRefptr<ParallelFlushReturn>>
      parallel_resolve_tree_tasks_queue_{};

  base::ConcurrentQueue<std::string> attribute_timing_flag_list_{};

  std::shared_ptr<tasm::TasmWorkerTaskRunner> task_runner_;

  bool settings_enable_use_mapbuffer_for_ui_op_{false};

  std::atomic_int element_count_{0};
  std::atomic_int layout_only_element_count_{0};
  std::atomic_int layout_only_transition_count_{0};

  ALLOW_UNUSED_TYPE std::map<lynx::devtool::DevToolFunction,
                             std::function<void(const base::any &)>>
      devtool_func_map_;

 public:
  // fixed node attached to the page node.
  std::list<tasm::ElementContainer *> fixed_node_list_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ELEMENT_MANAGER_H_
