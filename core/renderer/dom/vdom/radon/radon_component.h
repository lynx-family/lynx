// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_RADON_COMPONENT_H_
#define CORE_RENDERER_DOM_VDOM_RADON_RADON_COMPONENT_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/css_fragment.h"
#include "core/renderer/css/css_fragment_decorator.h"
#include "core/renderer/dom/vdom/radon/base_component.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/renderer/dom/vdom/radon/radon_slot.h"
#include "core/renderer/dom/vdom/radon/set_css_variable_op.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/vm_context.h"
#include "core/template_bundle/template_codec/ttml_constant.h"

namespace lynx {
namespace tasm {

class RadonPage;

struct RenderOption {
  bool recursively = false;
};

using SetCSSVariableOpVector = std::vector<SetCSSVariableOp>;

class RadonComponent : public RadonNode, public BaseComponent {
 public:
  enum class RenderType {
    FirstRender,
    UpdateByNative,
    UpdateFromJSBySelf,
    UpdateByParentComponent,
    UpdateByRenderError,
    UpdateByNativeList,
  };

  // usually used to create component or lazy bundle
  enum class ComponentType { kUndefined = 0, kStatic, kDynamic };

  RadonComponent(
      PageProxy* client, int tid, CSSFragment* style_sheet,
      std::shared_ptr<CSSStyleSheetManager> style_sheet_manager,
      ComponentMould* mould, lepus::Context* context, uint32_t node_index,
      const base::String& tag_name = BASE_STATIC_STRING(kRadonComponentTag));
  RadonComponent(const RadonComponent& node, PtrLookupMap& map);

  virtual ~RadonComponent();

  // TODO(songshourui.null): In the future, a utility class related to
  // RadonComponent will be added, and this function will be moved to the
  // utility class to avoid the explosion of functions in RadonComponent.
  static void UpdateTable(lepus::Value& target, const lepus::Value& update,
                          bool reset = false);

  int32_t tid() { return tid_; }

  // TODO(songshourui.null): In the future, a utility class related to CSS will
  // be added, and the class is used to unify the CSS logic of RadonComponent
  // and ComponentElement. And this function will be moved to the utility.
  int32_t GetCSSId() const { return mould_ ? mould_->css_id() : 0; }

  void SetDSL(PackageInstanceDSL dsl) { dsl_ = dsl; }
  PackageInstanceDSL GetDSL() { return dsl_; }
  bool IsReact() { return dsl_ == PackageInstanceDSL::REACT; }

  void SetName(base::String name) { name_ = std::move(name); }
  void SetPath(base::String path) { path_ = std::move(path); }

  const lepus::Value& GetData() override { return data_; }
  const lepus::Value& GetProperties() override { return properties_; }
  const lepus::Value& GetInitialData() { return init_data_; }

  std::string ComponentStrId() override {
    return std::to_string(ComponentId());
  };

  const base::String& name() const { return name_; }
  const base::String& path() const { return path_; }

  const std::shared_ptr<CSSStyleSheetManager>& style_sheet_manager() const {
    return style_sheet_manager_;
  }

  bool IsInLepusNGContext() { return context_ && context_->IsLepusNGContext(); }

  void SetGetDerivedStateFromPropsProcessor(const lepus::Value& processor) {
    get_derived_state_from_props_function_ = processor;
  }

  void SetGetDerivedStateFromErrorProcessor(const lepus::Value& processor) {
    get_derived_state_from_error_function_ = processor;
  }

  void SetRenderError(const lepus::Value& error) { render_error_ = error; }

  void SetShouldComponentUpdateProcessor(const lepus::Value& processor) {
    should_component_update_function_ = processor;
  }

  void set_pre_properties(const lepus::Value& properties) {
    pre_properties_ = properties;
  }

  void set_pre_data(const lepus::Value& data) { pre_data_ = data; }

  // Only when a lazy bundle is loaded async, it could be empty.
  bool IsEmpty() const { return context_ == nullptr; }

  std::shared_ptr<ComponentConfig> GetComponentConfig() const {
    if (mould_) {
      return mould_->GetComponentConfig();
    }
    return nullptr;
  }

  NameToSlotMap& slots() { return slots_; }
  NameToPlugMap& plugs() { return plugs_; }
  RadonSlotsHelper* radon_slots_helper() { return radon_slots_helper_.get(); }

  void set_need_reset_data(bool value) { need_reset_data_ = value; }
  bool need_reset_data() { return need_reset_data_; }

  void set_list_need_remove(bool value) { list_need_remove_ = value; }
  // component should be removed from parent in list
  bool list_need_remove() { return list_need_remove_; }

  void set_list_need_remove_after_reused(bool value) {
    list_need_remove_after_reused_ = value;
  }
  // component should be removed from parent after being reused in list
  bool list_need_remove_after_reused() {
    return list_need_remove_after_reused_;
  }

  bool PreRender(const RenderType& render_type);

  // RadonLazyComponent will override this function
  virtual void DeriveFromMould(ComponentMould* data);

  void PrepareComponentExternalStyles(AttributeHolder* holder);

  void PrepareRootCSSVariables(AttributeHolder* holder);

  // RadonPage will override this function
  // TODO(songshourui.null): In the future, a utility class related to CSS will
  // be added, and the class is used to unify the CSS logic of RadonComponent
  // and ComponentElement. And this function will be moved to the utility.
  virtual CSSFragment* GetStyleSheetBase(AttributeHolder* holder);

  void ExtractExternalClass(ComponentMould* data);

  virtual bool UpdateGlobalProps(const lepus::Value& table);

  // RadonLazyComponent will override this function
  virtual void SetProperties(const base::String& key, const lepus::Value& value,
                             bool strict_prop_type);

  // RadonLazyComponent will override this function
  virtual void SetData(const base::String& key, const lepus::Value& value);

  void UpdateSystemInfo(const lynx::lepus::Value& info);

  bool ShouldComponentUpdate();

  lepus_value PreprocessData();

  RadonComponent* GetErrorBoundary();

  lepus_value PreprocessErrorData();

  int ImplId() const override;

  /* Recursively call Component removed lifecycle in post order.
   * But save the original radon tree structure.
   */
  virtual void OnComponentRemovedInPostOrder() final;

  // TODO(songshourui.null): In the future, a utility class related to CSS will
  // be added, and the class is used to unify the CSS logic of RadonComponent
  // and ComponentElement. And this function will be moved to the utility.
  CSSFragment* GetStyleSheet();
  // TODO(songshourui.null): In the future, a utility class related to CSS will
  // be added, and the class is used to unify the CSS logic of RadonComponent
  // and ComponentElement. And this function will be moved to the utility.
  void OnStyleSheetReady(CSSFragment* fragment);

  void AddChild(std::unique_ptr<RadonBase> child) override;
  void AddSubTree(std::unique_ptr<RadonBase> child) override;

  void SetComponent(RadonComponent* component) override;

  void AddRadonPlug(const base::String& name, std::unique_ptr<RadonBase> plug);
  void RemovePlugByName(const base::String& name);

  void AddRadonSlot(const base::String& name, RadonSlot* slot);

  // for remove component element
  bool NeedsElement() const override;

  // RadonPage & RadonLazyComponent will override this function
  virtual bool NeedsExtraData() const;

  Element* TopLevelViewElement() const;

  void Dispatch(const DispatchOption&) override;
  void DispatchSelf(const DispatchOption&) override;

  void DispatchForDiff(const DispatchOption&) override;

  void DispatchChildren(const DispatchOption&) override;
  void DispatchChildrenForDiff(const DispatchOption&) override;

  void ResetElementRecursively() override;

  void OnElementRemoved(int idx) override;
  void OnElementMoved(int from_idx, int to_idx) override;

  void UpdateRadonComponent(RenderType render_type,
                            const lepus::Value& incoming_property,
                            const lepus::Value& incoming_data,
                            const DispatchOption& option,
                            PipelineOptions& pipeline_options);

  // TODO(songshourui.null): In the future, a utility class related to CSS will
  // be added, and the class is used to unify the CSS logic of RadonComponent
  // and ComponentElement. And this function will be moved to the utility.
  void SetCSSVariables(const std::string& id_selector,
                       const lepus::Value& properties,
                       PipelineOptions& pipeline_options);

  bool UpdateRadonComponentWithoutDispatch(
      RenderType render_type, const lepus::Value& incoming_property,
      const lepus::Value& incoming_data);

  void RenderRadonComponentIfNeeded(RenderOption&);
  void SetGlobalPropsFromTasm();

  BASE_EXPORT_FOR_DEVTOOL int32_t ComponentId();
  RadonComponent* GetParentComponent();
  RadonComponent* GetComponentOfThisComponent() { return component(); }

  virtual lepus::Value& GetComponentInfoMap(const std::string& entry_name = "");

  virtual lepus::Value& GetComponentPathMap(const std::string& entry_name = "");

  void OnReactComponentRenderBase(lepus::Value& new_data,
                                  bool should_component_update);

  void SwapElement(const std::unique_ptr<RadonBase>& old_radon_base,
                   const DispatchOption& option) override;

  void RadonDiffChildren(const std::unique_ptr<RadonBase>&,
                         const DispatchOption&) override;
  virtual void OnComponentUpdate(const DispatchOption& option);
  virtual void OnReactComponentDidUpdate(const DispatchOption& option);
  void ResetDispatchedStatus();

// TODO(songshourui.null): avoid using macro in header file
#if ENABLE_TRACE_PERFETTO
  void UpdateTraceDebugInfo(TraceEvent* event) override {
    RadonNode::UpdateTraceDebugInfo(event);
    auto* nameInfo = event->add_debug_annotations();
    nameInfo->set_name("componentName");
    nameInfo->set_string_value(name_.str());
  }
#endif

  void OnDataSetChanged() override;
  void OnSelectorChanged() override;

  // should only be used in render_functions:ProcessComponentData now.
  bool PreRenderForRadonComponent() { return PreRender(render_type_); }

  // render_type_ should be updated every time we re-render the radon tree.
  // render_type_ should only be used in PreRenderForComponent or PrePageRender.
  // This method utilizes the render_type_ last time we set when the tree was
  // updated to prerender the component.
  void SetRenderType(RenderType type) { render_type_ = type; }

  // set virtual component's entry name manually instead of looking up by the
  // vdom tree. only used in virtual component passed to lazy component.
  void SetEntryName(const std::string& entry_name) { entry_name_ = entry_name; }

  // a component may be in a lazy component, which has its own entry
  // same as virtual component
  const std::string& GetEntryName() const override;

  bool CanBeReusedBy(const RadonBase* const radon_base) const override;

  // RadonPage will override this function
  virtual void Refresh(const DispatchOption&,
                       PipelineOptions& pipeline_options);

  void GenerateAndSetComponentId();

  // methods to check properties undefined.
  // it's result will differ according to pageConfig `enableComponentNullProps`
  bool IsPropertiesUndefined(const lepus::Value& value) const;

  void ModifySubTreeComponent(RadonComponent* const target) override;
  bool ShouldBlockEmptyProperty();

  // WillRemoveNode is used to handle some special logic before
  // RemoveElementFromParent or radon's structure dtor
  void WillRemoveNode() override;

  // Used to set some special attribute for a component,
  // like lynx-key and removeComponentElement.
  // If the key is a special attribute key, it should not
  // be a property.
  bool SetSpecialComponentAttribute(const base::String& key,
                                    const lepus::Value& value);

  // Init style sheet through ssr.
  void InitStyleSheetBySSR(
      std::shared_ptr<tasm::CSSFragmentDecorator> style_sheet);

 protected:
  virtual fml::RefPtr<Element> CreateFiberElement() override;

  ComponentElement* component_element() const {
    return static_cast<ComponentElement*>(element());
  }

  enum class InListStatus {
    Unknown,
    InList,
    NotInList,
  };

  void AttachDataVersions(lepus::Value& update_data);
  void ResetDataVersions();

  // TODO(songshourui.null): In the future, a utility class related to CSS will
  // be added, and the class is used to unify the CSS logic of RadonComponent
  // and ComponentElement. And this function will be moved to the utility.
  void PreHandlerCSSVariable();

  // update __golbalProps and SystemInfo to data_
  void UpdateLepusTopLevelVariableToData();
  void AdoptPlugToSlot(RadonSlot* slot, std::unique_ptr<RadonBase> plug);

  bool CheckReactShouldAbortUpdating(const lepus::Value& table);
  bool CheckReactShouldComponentUpdateKey(const lepus::Value& table);
  bool CheckReactShouldAbortRenderError(const lepus::Value& table);
  bool SetRemoveComponentElement(const base::String& key,
                                 const lepus::Value& value);
  virtual void triggerNewLifecycle(const DispatchOption& option) override;

  int32_t tid_;
  bool data_dirty_{true};
  bool properties_dirty_{true};
  bool update_function_called_{false};

  bool need_reset_data_{false};

  // component should be removed from parent in list
  bool list_need_remove_{false};

  // component should be removed from parent after being reused in list
  bool list_need_remove_after_reused_{false};

  // used to set one component's `RemoveComponentElement` config.
  // If the component's `RemoveComponentElement` config has been set,
  // it will override the page_config's global `RemoveComponentElement`
  // This config shouldn't be updated. Otherwise the updating may cause a
  // re-rendering.
  BooleanProp remove_extra_data_{BooleanProp::NotSet};
  BooleanProp remove_component_element_{BooleanProp::NotSet};
  BooleanProp need_element_by_entry_{BooleanProp::NotSet};

  lepus::Value get_derived_state_from_props_function_;
  lepus::Value should_component_update_function_;
  lepus::Value get_derived_state_from_error_function_;
  lepus::Value render_error_{};

  // Key: base::String / value: lepus::Value
  // props and data should be initialized as Value_Nil and then get derived from
  // mould.
  lepus::Value properties_{};
  lepus::Value data_{};
  lepus::Value init_properties_{};
  lepus::Value init_data_{};

  base::String name_;
  base::String path_;

  mutable std::string entry_name_{};

  ComponentMould* mould_{nullptr};
  lepus::Context* context_{nullptr};

  PackageInstanceDSL dsl_;

  // The style sheet containing only the corresponding css file's content.
  CSSFragment* intrinsic_style_sheet_ = nullptr;
  std::shared_ptr<CSSStyleSheetManager> style_sheet_manager_;
  // The lazy-constructed style sheet taking external classes into account.
  std::shared_ptr<CSSFragmentDecorator> style_sheet_;

  lepus::Value component_info_map_ = lepus::Value(lepus::Dictionary::Create());
  lepus::Value component_path_map_ = lepus::Value(lepus::Dictionary::Create());

  InListStatus in_list_status_ = InListStatus::Unknown;

  // TODO(songshourui.null): In the future, a utility class related to CSS will
  // be added, and the class is used to unify the CSS logic of RadonComponent
  // and ComponentElement. And this function will be moved to the utility.
  SetCSSVariableOpVector set_variable_ops_;

  lepus::Value pre_properties_;
  lepus::Value pre_data_;

  RenderType render_type_;

 private:
  // TODO(songshourui.null): In the future, a utility class related to
  // RadonComponent will be added, and this function will be moved to the
  // utility class to avoid the explosion of functions in RadonComponent.
  static lepus::Value GetDefaultValue(const lepus::Value& template_value);

  bool PreRenderReact(const RenderType& render_type);
  bool PreRenderTT(const RenderType& render_type);
  bool IsInList();

  bool GetNeedElementByEntry();

  virtual void RenderRadonComponent(RenderOption&);
  void DisableCallOnElementRemovedInDestructor();

  /*
   * RadonReusableDiffChildren is only used in radon diff list new arch.
   * This function will diff a complete and determined radon component (reuser)
   * without element with an old radon component with element (reused element).
   * If the reuser is a new created component, should call related component's
   * lifecycle and continually diff its children.
   * If the reuser is a component dispatched and updated before, should just
   * continually diff its children, because its lifecycle has been called when
   * it updated using component info's data and properties.
   */
  void RadonReusableDiffChildren(RadonComponent* old_radon_component,
                                 const DispatchOption& option);
  bool NeedSavePreState(const RenderType& render_type) {
    return should_component_update_function_.IsCallable() &&
           !(IsReact() && render_type == RenderType::UpdateFromJSBySelf);
  }

  bool ShouldRemoveComponentElement() const;

  uint32_t component_id_{0};
  bool compile_render_{false};

  NameToSlotMap slots_{kRadonSlotMapAllocationSize};
  NameToPlugMap plugs_;
  std::unique_ptr<RadonSlotsHelper> radon_slots_helper_;
};

class RadonListComponent : public RadonComponent {
 public:
  RadonListComponent(
      PageProxy* page_proxy, int tid, CSSFragment* style_sheet,
      std::shared_ptr<CSSStyleSheetManager> style_sheet_manager,
      ComponentMould* mould, lepus::Context* context, uint32_t node_index,
      int distance_from_root,
      const base::String& tag_name = BASE_STATIC_STRING(kRadonComponentTag));

  int32_t distance_from_root() { return distance_from_root_; }
  void set_distance_from_root(int32_t distance) {
    distance_from_root_ = distance;
  }

  void SetComponent(RadonComponent* component) override;
  void ModifySubTreeComponent(RadonComponent* const target) override;

 private:
  int32_t distance_from_root_{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_RADON_COMPONENT_H_
