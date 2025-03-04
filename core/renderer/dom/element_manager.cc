// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/element_manager.h"

#include <array>
#include <memory>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_selector_constants.h"
#include "core/renderer/css/dynamic_css_styles_manager.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/parser/length_handler.h"
#include "core/renderer/dom/element_vsync_proxy.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/none_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/scroll_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fiber/wrapper_element.h"
#include "core/renderer/dom/vdom/radon/radon_list_base.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/ui_component/list/radon_list_element.h"
#include "core/renderer/ui_wrapper/painting/catalyzer.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/recorder/recorder_controller.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/shell/common/vsync_monitor.h"
#include "core/shell/layout_mediator.h"
#include "core/value_wrapper/value_impl_lepus.h"

#if ENABLE_AIR
#include "core/renderer/dom/air/air_element/air_element.h"
#include "core/renderer/dom/air/air_element/air_for_element.h"
#include "core/renderer/dom/air/air_element/air_page_element.h"
#endif

constexpr const static char *kEventDomSizeKey = "dom_size";

namespace lynx {
namespace tasm {
#pragma mark ElementManager

#if ENABLE_AIR
//====== for air element begin ========/
fml::RefPtr<AirLepusRef> ElementManager::GetAirNode(const base::String &tag,
                                                    int32_t lepus_id) {
  uint64_t key = air_root_->GetKeyForCreatedElement(lepus_id);
  auto element = air_node_manager_->GetForLepusId(lepus_id, key);
  if (element) {
    return element;
  }
  return nullptr;
}

fml::RefPtr<AirLepusRef> ElementManager::CreateAirNode(const base::String &tag,
                                                       int32_t lepus_id,
                                                       int32_t impl_id,
                                                       uint64_t key) {
  std::shared_ptr<AirElement> element =
      std::make_shared<AirElement>(kAirNormal, this, tag, lepus_id, impl_id);
  air_node_manager()->Record(element->impl_id(), element);

  auto res = AirLepusRef::Create(element);
  // In most cases, each element has a unique lepus id, but when tt:for node
  // or component node exists, there will be multiple elements with the same
  // lepus id. Use the double-map structure to record the elements. In the outer
  // map, key is the lepus id. In the inner map, for elements with the same
  // lepus id, using the unique id of tt:for or component to assemble a unique
  // key; for other cases, the key is the lepus id. We can find the specific
  // element with this record structrue.
  air_node_manager()->RecordForLepusId(lepus_id, key, res);
  return res;
}

AirPageElement *ElementManager::CreateAirPage(int32_t lepus_id) {
  auto page = std::make_shared<AirPageElement>(this, lepus_id);
  air_node_manager()->Record(page->impl_id(), page);
  return page.get();
}

void AirNodeManager::EraseLepusId(int id, AirElement *node) {
  auto iterator = air_lepus_id_map_.find(id);
  if (iterator != air_lepus_id_map_.end()) {
    auto &lepus_map = iterator->second;
    for (auto it = lepus_map.begin(); it != lepus_map.end();) {
      if (reinterpret_cast<AirLepusRef *>(it->second.get())->Get() == node) {
        lepus_map.erase(it);
        break;
      } else {
        ++it;
      }
    }
  }
}

fml::RefPtr<AirLepusRef> AirNodeManager::GetForLepusId(int tag, uint64_t key) {
  auto it = air_lepus_id_map_.find(tag);
  if (it != air_lepus_id_map_.end()) {
    auto &map = it->second;
    if (map.find(key) != map.end()) {
      return AirLepusRef::Create(
          reinterpret_cast<AirLepusRef *>(map[key].get()));
    }
  }
  return nullptr;
}

std::vector<fml::RefPtr<AirLepusRef>> AirNodeManager::GetAllNodesForLepusId(
    int tag) const {
  auto it = air_lepus_id_map_.find(tag);
  if (it != air_lepus_id_map_.end()) {
    std::vector<fml::RefPtr<AirLepusRef>> result;
    for (auto iter = it->second.begin(); iter != it->second.end(); ++iter) {
      // TODO(renpengcheng) delete the reinterpret_cast when AirLepusRef was
      // included by default
      result.push_back(AirLepusRef::Create(
          reinterpret_cast<AirLepusRef *>(iter->second.get())));
    }
    return result;
  }
  return {};
}

void AirNodeManager::RecordForLepusId(int id, uint64_t key,
                                      fml::RefPtr<AirLepusRef> node) {
  air_lepus_id_map_[id].emplace(key, std::move(node));
}

#endif

ElementManager::ElementManager(
    std::unique_ptr<PaintingCtxPlatformImpl> platform_painting_context,
    Delegate *delegate, const LynxEnvConfig &lynx_env_config,
    int32_t instance_id,
    const std::shared_ptr<shell::VSyncMonitor> &vsync_monitor,
    const bool enable_diff_without_layout)
    : node_manager_(new NodeManager),
      air_node_manager_(new AirNodeManager),
      component_manager_(new ComponentManager),
      catalyzer_(
          std::make_unique<Catalyzer>(std::make_unique<PaintingContext>(
                                          std::move(platform_painting_context)),
                                      instance_id)),
      root_(nullptr),
      instance_id_(instance_id),
      lynx_env_config_(lynx_env_config),
      delegate_(delegate),
      vsync_monitor_(vsync_monitor),
      enable_diff_without_layout_(enable_diff_without_layout),
      platform_computed_css_(std::make_unique<starlight::ComputedCSSStyle>(
          lynx_env_config.LayoutsUnitPerPx(),
          lynx_env_config.PhysicalPixelsPerLayoutUnit())),
      settings_enable_use_mapbuffer_for_ui_op_(
          LynxEnv::GetInstance().EnableUseMapBufferForUIProps()) {
  dom_tree_enabled_ = lynx::tasm::LynxEnv::GetInstance().IsDomTreeEnabled();
  platform_computed_css_->SetCSSParserConfigs(GetCSSParserConfigs());
  task_runner_ = std::make_shared<tasm::TasmWorkerTaskRunner>();
  enable_new_animator_fiber_ = LynxEnv::GetInstance().EnableNewAnimatorFiber();
  enable_new_animator_radon_ = false;
}

static bool EnableLayoutOnlyStatistic() {
  // cache the setting.
  static bool enable = tasm::LynxEnv::GetInstance().GetBoolEnv(
      tasm::LynxEnv::Key::ENABLE_LAYOUT_ONLY_STATISTIC, false);
  return enable;
}

ElementManager::~ElementManager() {
  if (EnableLayoutOnlyStatistic()) {
    report::EventTracker::OnEvent([element_count = element_count_.load(),
                                   layout_only_element_count =
                                       layout_only_element_count_.load(),
                                   layout_only_transition_count =
                                       layout_only_transition_count_.load()](
                                      report::MoveOnlyEvent &event) {
      event.SetName("lynxsdk_layout_only_element_statistic");
      event.SetProps("element_count", static_cast<unsigned int>(element_count));
      event.SetProps("layout_only_element_count",
                     static_cast<unsigned int>(layout_only_element_count));
      event.SetProps("layout_only_transition_count",
                     static_cast<unsigned int>(layout_only_transition_count));
    });
  }
  WillDestroy();
}

void ElementManager::WillDestroy() {
  LOGE("ElementManager::WillDestroy this:" << this);
  if (UseFiberElement()) {
    node_manager_->WillDestroy();
  }
  EXEC_EXPR_FOR_INSPECTOR({ OnElementManagerWillDestroy(); });
}

fml::RefPtr<RadonElement> ElementManager::CreateNode(
    const base::String &tag, const std::shared_ptr<AttributeHolder> &node,
    uint32_t node_index, RadonNodeType radon_node_type) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementManager::CreateNode", "tag",
              tag.str());
  fml::RefPtr<RadonElement> element = nullptr;
  if (radon_node_type == RadonNodeType::kRadonListNode && node &&
      static_cast<RadonListBase *>(node->radon_node_ptr())
          ->DisablePlatformImplementation()) {
    element =
        fml::MakeRefCounted<RadonListElement>(tag, node, this, node_index);
  }
  if (!element) {
    element = fml::MakeRefCounted<RadonElement>(tag, node, this, node_index);
  }
  element->UpdatePlatformNodeTag();
  return element;
}

void ElementManager::OnDocumentUpdated() {
  EXEC_EXPR_FOR_INSPECTOR({
    if (inspector_element_observer_ && IsDomTreeEnabled()) {
      inspector_element_observer_->OnDocumentUpdated();
    }
  });
}

void ElementManager::OnElementManagerWillDestroy() {
  EXEC_EXPR_FOR_INSPECTOR({
    if (inspector_element_observer_ && IsDomTreeEnabled()) {
      inspector_element_observer_->OnElementManagerWillDestroy();
    }
  });
}

void ElementManager::OnElementNodeAddedForInspector(Element *element) {
  EXEC_EXPR_FOR_INSPECTOR({
    if (inspector_element_observer_ && IsDomTreeEnabled()) {
      inspector_element_observer_->OnElementNodeAdded(element);
    }
  });
}

void ElementManager::OnElementNodeRemovedForInspector(Element *element) {
  EXEC_EXPR_FOR_INSPECTOR({
    if (inspector_element_observer_ && IsDomTreeEnabled()) {
      inspector_element_observer_->OnElementNodeRemoved(element);
    }
  });
}

void ElementManager::OnElementNodeSetForInspector(Element *element) {
  EXEC_EXPR_FOR_INSPECTOR({
    if (inspector_element_observer_ && IsDomTreeEnabled()) {
      inspector_element_observer_->OnElementDataModelSet(element);
    }
  });
}

void ElementManager::OnCSSStyleSheetAddedForInspector(Element *element) {
  EXEC_EXPR_FOR_INSPECTOR({
    if (inspector_element_observer_ && IsDomTreeEnabled()) {
      inspector_element_observer_->OnCSSStyleSheetAdded(element);
    }
  });
}

void ElementManager::OnComponentUselessUpdate(const std::string &component_name,
                                              const lepus::Value &properties) {
  EXEC_EXPR_FOR_INSPECTOR({
    auto hierarchy_observer = hierarchy_observer_.lock();
    if (hierarchy_observer) {
      hierarchy_observer->OnComponentUselessUpdate(component_name, properties);
      TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY,
                          "Devtool::OnComponentUselessUpdate",
                          [&component_name](lynx::perfetto::EventContext ctx) {
                            auto *debug = ctx.event()->add_debug_annotations();
                            debug->set_name("ComponentName");
                            debug->set_string_value(component_name);
                          });
    }
  });
}

void ElementManager::OnSetNativeProps(tasm::Element *ptr,
                                      const std::string &name,
                                      const lepus::Value &value,
                                      bool is_style) {
  EXEC_EXPR_FOR_INSPECTOR({
    if (inspector_element_observer_ && IsDomTreeEnabled()) {
      std::string value_str;
      if (value.IsNumber()) {
        std::ostringstream stm;
        stm << value.Number();
        value_str = stm.str();
      } else {
        value_str = value.StdString();
      }
      inspector_element_observer_->OnSetNativeProps(ptr, name, value_str,
                                                    is_style);
    }
  });
}

void ElementManager::RunDevToolFunction(
    lynx::devtool::DevToolFunction func_enum, const base::any &data) {
  EXEC_EXPR_FOR_INSPECTOR({
    auto iter = devtool_func_map_.find(func_enum);
    if (iter != devtool_func_map_.end()) {
      (iter->second)(data);
    } else {
      LOGE("ElementManager::RunDevToolFunction failed since can not find "
           << static_cast<int32_t>(func_enum) << " function.");
    }
  });
}

void ElementManager::FiberAttachToInspectorRecursively(FiberElement *root) {
  EXEC_EXPR_FOR_INSPECTOR({
    if (!devtool_flag_ || !IsDomTreeEnabled()) {
      return;
    }
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "Devtool::FiberAttachToInspectorRecursively");
    std::function<void(FiberElement *)> prepare_and_add_node_f =
        [this, &prepare_and_add_node_f](FiberElement *element) {
          PrepareNodeForInspector(element);
          for (const auto &child : element->children()) {
            prepare_and_add_node_f(child.get());
          }
          CheckAndProcessSlotForInspector(element);
          OnElementNodeAddedForInspector(element);
        };
    prepare_and_add_node_f(root);
  });
}

void ElementManager::PrepareNodeForInspector(Element *element) {
  EXEC_EXPR_FOR_INSPECTOR({
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "Devtool::PrepareNodeForInspector");
    if (devtool_flag_ && IsDomTreeEnabled()) {
      RunDevToolFunction(lynx::devtool::DevToolFunction::InitForInspector,
                         std::make_tuple(element));
      if (element->GetTag() == kElementPageTag ||
          element->GetTag() ==
              kElementComponentTag) {  // page is special component
        PrepareComponentNodeForInspector(element);
      }
    }
  });
}

void ElementManager::CheckAndProcessSlotForInspector(Element *element) {
  EXEC_EXPR_FOR_INSPECTOR({
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "Devtool::CheckAndProcessSlotForInspector");
    // If devtool_flag_ is false or IsDomTreeEnabled() is false, return.
    if (!devtool_flag_ || !IsDomTreeEnabled()) {
      return;
    }
    // Check if element is plug.
    FiberElement *current = static_cast<FiberElement *>(element);
    // If current is nullptr, return.
    if (current == nullptr) {
      return;
    }
    FiberElement *parent = static_cast<FiberElement *>(current->parent());
    // If parent is nullptr, return.
    if (parent == nullptr) {
      return;
    }
    FiberElement *component_element =
        static_cast<FiberElement *>(current->GetParentComponentElement());
    // If current's component_element is nullptr, return.
    if (component_element == nullptr) {
      return;
    }

    // If parent is current's component_element, current must not be plug,
    // then return.
    if (component_element == parent) {
      return;
    }

    // If parent's component_element == current's component_element, current
    // must not be plug, then return
    FiberElement *parent_component_element =
        static_cast<FiberElement *>(parent->GetParentComponentElement());
    if (!parent_component_element ||
        (!parent->is_component() &&
         component_element == parent_component_element) ||
        (parent->is_component() &&
         component_element != parent_component_element)) {
      return;
    }

    RunDevToolFunction(lynx::devtool::DevToolFunction::InitPlugForInspector,
                       std::make_tuple(element));
  });
}

void ElementManager::RequestLayout(const PipelineOptions &options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementManager::RequestLayout");
  if (enable_diff_without_layout_) {
    delegate_->SetEnableLayout();
  } else {
    DispatchLayoutUpdates(options);
  }
}

void ElementManager::DispatchLayoutUpdates(
    const tasm::PipelineOptions &options) {
  // insert PAINTING_UI_OPERATION_FLUSH_END to UI Operation Queue before layout.
  if (options.need_timestamps) {
    painting_context()->MarkUIOperationQueueFlushTiming(
        tasm::timing::kPaintingUiOperationExecuteEnd, options.pipeline_id);
  }
  delegate_->DispatchLayoutUpdates(options);
}

std::unordered_map<int32_t, LayoutInfoArray>
ElementManager::GetSubTreeLayoutInfo(int32_t root_id, Viewport viewport) {
  return delegate_->GetSubTreeLayoutInfo(root_id, viewport);
}

void ElementManager::DidPatchFinishForFiber() {}

void ElementManager::PrepareComponentNodeForInspector(Element *component) {
  EXEC_EXPR_FOR_INSPECTOR({
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "Devtool::PrepareComponentNodeForInspector");

    const auto &create_element = [this, component](const std::string &tag) {
      bool enable_fiber = this->UseFiberElement();
      Element *element = nullptr;
      if (enable_fiber) {
        element = new FiberElement(this, tag);
        // The additional element created by the inspector needs to
        // maintain a null data model to indicate that this element is
        // created by inspector.
        static_cast<FiberElement *>(element)->ResetDataModel();
        static_cast<FiberElement *>(element)
            ->SetParentComponentUniqueIdForFiber(component->impl_id());
      } else {
        element = new RadonElement(tag, nullptr, this, component->NodeIndex());
      }
      return element;
    };

    if (component->GetTag() == kElementPageTag) {
      Element *doc = create_element("doc");
      RunDevToolFunction(lynx::devtool::DevToolFunction::InitForInspector,
                         std::make_tuple(doc));
      RunDevToolFunction(lynx::devtool::DevToolFunction::SetDocElement,
                         std::make_tuple(component, doc));
    }

    Element *style_value = create_element("stylevalue");
    RunDevToolFunction(lynx::devtool::DevToolFunction::InitForInspector,
                       std::make_tuple(style_value));

    RunDevToolFunction(lynx::devtool::DevToolFunction::InitStyleValueElement,
                       std::make_tuple(style_value, component));
    RunDevToolFunction(lynx::devtool::DevToolFunction::SetStyleValueElement,
                       std::make_tuple(component, style_value));
    style_value->set_parent(component);

    RunDevToolFunction(lynx::devtool::DevToolFunction::SetStyleRoot,
                       std::make_tuple(style_value, style_value));

    if (component->GetTag() == kElementPageTag) {
      RunDevToolFunction(lynx::devtool::DevToolFunction::SetStyleRoot,
                         std::make_tuple(component, style_value));
    }

    if (component->is_fiber_element() &&
        static_cast<FiberElement *>(component)->is_wrapper()) {
      component->inspector_attribute()->wrapper_component_ = true;
    }

    std::string style_sheet_id = std::to_string(style_value->impl_id());
    OnCSSStyleSheetAddedForInspector(style_value);
  });
}

void ElementManager::ResolveAttributesAndStyle(AttributeHolder *node,
                                               Element *shadow_node,
                                               const StyleMap &styles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ResolveAttributesAndStyle");
  // FIXME: key frames should not be singleton
  auto style_sheet = node->ParentStyleSheet();
  if (preresolving_style_sheet_ != style_sheet && style_sheet && !style_sheet->HasFontFacesResolved() /* && TODO:(radon)
      node->component()->is_first_patch() */) {
    preresolving_style_sheet_ = style_sheet;
    const auto &all_fontfaces = style_sheet->GetFontFaceRuleMap();
    if (!all_fontfaces.empty()) {
      root_->SetFontFaces(all_fontfaces);
    }
    style_sheet->MarkFontFacesResolved(true);
  }

  // Normally, all attributes should be consumed before consuming the style.
  // This is because attributes are usually a switch, such as
  // enable_new_animator, and the value of the attribute switch may be needed
  // when consuming the style. However, due to historical legacy issues,
  // attributes were consumed later than styles. If we directly exchange the
  // order of the two, it will cause a breaking change. Therefore, here we check
  // the new animator in advance.
  for (const auto &attribute : node->attributes()) {
    shadow_node->CheckNewAnimatorAttr(attribute.first, attribute.second);
  }

  shadow_node->ConsumeStyle(styles);

  for (const auto &attribute : node->attributes()) {
    shadow_node->SetAttribute(attribute.first, attribute.second);
  }

  const DataMap &data_map = node->dataset();
  if (!data_map.empty()) {
    shadow_node->SetDataSet(node->dataset());
  }

  // Resolve other pseudo selectors
  shadow_node->ResolvePlaceHolder();

  ResolveEvents(node, shadow_node);
  // resolve gesture detectors
  ResolveGestures(node, shadow_node);
}

void ElementManager::ResolveEvents(AttributeHolder *node, Element *element) {
  for (const auto &event : node->static_events()) {
    element->SetEventHandler(event.first, event.second.get());
  }

  for (const auto &lepus_event : node->lepus_events()) {
    element->SetEventHandler(lepus_event.first, lepus_event.second.get());
  }
  // handle global-bind event and store element id in order to construct
  // currentTarget object
  for (const auto &global_bind_event : node->global_bind_events()) {
    EventHandler *handler = global_bind_event.second.get();
    element->SetEventHandler(global_bind_event.first, handler);
    element->HandleDelayTask([this, name_ = handler->name(),
                              type_ = handler->type(),
                              id_ = element->impl_id()] {
      SetGlobalBindElementId(name_, type_, id_);
    });
  }
}

// resolve gesture detector from attribute holder
void ElementManager::ResolveGestures(AttributeHolder *node, Element *element) {
  for (const auto &gesture : node->gesture_detectors()) {
    element->SetGestureDetector(gesture.first, gesture.second.get());
  }
}

void ElementManager::UpdateScreenMetrics(float width, float height) {
  LOGI("ElementManager::UpdateScreenMetrics width:" << width
                                                    << ",height:" << height);
  GetLynxEnvConfig().UpdateScreenSize(width, height);
  // 1.update layout tree
  delegate_->UpdateLynxEnvForLayoutThread(GetLynxEnvConfig());
  if (root()) {
    // 2.update element tree
    root()->UpdateDynamicElementStyle(
        DynamicCSSStylesManager::kUpdateScreenMetrics, false);
  }
}

void ElementManager::UpdateFontScale(float font_scale) {
  GetLynxEnvConfig().SetFontScale(font_scale);
  // update element tree
  delegate_->UpdateLynxEnvForLayoutThread(GetLynxEnvConfig());
  if (root()) {
    root()->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateFontScale,
                                      false);
    delegate_->SetRootOnLayout(root_->impl_id());
  }
}

void ElementManager::SetInspectorElementObserver(
    const std::shared_ptr<InspectorElementObserver>
        &inspector_element_observer) {
  inspector_element_observer_ = inspector_element_observer;
  devtool_func_map_ = inspector_element_observer->GetDevToolFunction();
  devtool_flag_ = true;
}

void ElementManager::OnFinishUpdateProps(Element *node,
                                         PipelineOptions &options) {
  if (node->is_radon_element()) {
    SetNeedsLayout();
    node->StylesManager().UpdateWithParentStatusForOnceInheritance(
        node->parent());
    node->FlushProps();
  } else if (node->is_fiber_element()) {
    static_cast<FiberElement *>(node)->MarkPropsDirty();
    OnPatchFinish(options, static_cast<FiberElement *>(node));
  }
}

void ElementManager::OnPatchFinishForRadon(PipelineOptions &options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "ElementManager::OnPatchFinish");
  catalyzer_->painting_context()->FinishTasmOperation(options);

  if (options.is_reload_template) {
    catalyzer_->painting_context()->UpdateNodeReloadPatching();
  }

  if (!need_layout_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementManager::OnPatchFinishNoPatch");
    LOGI("ElementManager::OnPatchFinishNoPatch!");
    catalyzer_->painting_context()->FinishLayoutOperation(options);
    delegate_->OnUpdateDataWithoutChange();
  } else {
    LOGI("ElementManager::OnPatchFinish");
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementManager::OnPatchFinishInner",
                [&options](lynx::perfetto::EventContext ctx) {
                  options.UpdateTraceDebugInfo(ctx.event());
                });
    BindTimingFlagToPipelineOptions(options);
    PatchEventRelatedInfo();
    root()->UpdateDynamicElementStyle(DynamicCSSStylesManager::kAllStyleUpdate,
                                      false);
    {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementManager sort z-index");
      // sort z-index children
      for (const auto &context : dirty_stacking_contexts_) {
        context->UpdateZIndexList();
      }
    }
    dirty_stacking_contexts_.clear();
    RequestLayout(options);
  }
  need_layout_ = false;
}

void ElementManager::PatchEventRelatedInfo() {
  if (push_touch_pseudo_flag_) {
    catalyzer_->painting_context()->UpdateEventInfo(true);
    push_touch_pseudo_flag_ = false;
  }
}

#if ENABLE_AIR
void ElementManager::OnPatchFinishInnerForAir(const PipelineOptions &options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementManager::OnPatchFinishInnerForAir");
  DispatchLayoutUpdates(options);
}
#endif

PaintingContext *ElementManager::painting_context() {
  return catalyzer_->painting_context();
}

void ElementManager::UpdateViewport(float width, SLMeasureMode width_mode,
                                    float height, SLMeasureMode height_mode,
                                    bool need_layout) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementManager::UpdateViewport");
  auto old_env = GetLynxEnvConfig();
  GetLynxEnvConfig().UpdateViewport(width, width_mode, height, height_mode);
  if (old_env.ViewportHeight() != GetLynxEnvConfig().ViewportHeight() ||
      old_env.ViewportWidth() != GetLynxEnvConfig().ViewportWidth()) {
    delegate_->UpdateLynxEnvForLayoutThread(GetLynxEnvConfig());
  }
  if (root()) {
    // 2.update element tree
    root()->UpdateDynamicElementStyle(DynamicCSSStylesManager::kUpdateViewport,
                                      false);
  }
  OnUpdateViewport(width, width_mode, height, height_mode, need_layout);
}

void ElementManager::OnUpdateViewport(float width, int width_mode, float height,
                                      int height_mode, bool need_layout) {
  delegate_->OnUpdateViewport(width, width_mode, height, height_mode,
                              need_layout);
}

void ElementManager::SetRootOnLayout(int32_t id) {
  delegate_->SetRootOnLayout(id);
}

// delegate for class element
void ElementManager::CreateLayoutNode(int32_t id, const base::String &tag) {
  delegate_->CreateLayoutNode(id, tag);
}

void ElementManager::UpdateLayoutNodeFontSize(int32_t id,
                                              double cur_node_font_size,
                                              double root_node_font_size) {
  delegate_->UpdateLayoutNodeFontSize(id, cur_node_font_size,
                                      root_node_font_size,
                                      GetLynxEnvConfig().FontScale());
}

void ElementManager::InsertLayoutNode(int32_t parent_id, int32_t child_id,
                                      int index) {
  delegate_->InsertLayoutNode(parent_id, child_id, index);
}

void ElementManager::RemoveLayoutNodeAtIndex(int32_t parent_id, int index) {
  delegate_->RemoveLayoutNodeAtIndex(parent_id, index);
}

void ElementManager::InsertLayoutNodeBefore(int32_t parent_id, int32_t child_id,
                                            int32_t ref_id) {
  delegate_->InsertLayoutNodeBefore(parent_id, child_id, ref_id);
}

void ElementManager::RemoveLayoutNode(int32_t parent_id, int32_t child_id) {
  delegate_->RemoveLayoutNode(parent_id, child_id);
}
void ElementManager::DestroyLayoutNode(int32_t id) {
  delegate_->DestroyLayoutNode(id);
}

void ElementManager::MoveLayoutNode(int32_t parent_id, int32_t child_id,
                                    int from_index, int to_index) {
  delegate_->MoveLayoutNode(parent_id, child_id, from_index, to_index);
}

void ElementManager::SendAnimationEvent(const char *type, int tag,
                                        const lepus::Value &dict) {
  delegate_->SendAnimationEvent(type, tag, dict);
}

void ElementManager::SendNativeCustomEvent(const std::string &name, int tag,
                                           const lepus::Value &param_value,
                                           const std::string &param_name) {
  delegate_->SendNativeCustomEvent(name, tag, param_value, param_name);
}

void ElementManager::UpdateLayoutNodeStyle(int32_t id,
                                           tasm::CSSPropertyID css_id,
                                           const tasm::CSSValue &value) {
  SetNeedsLayout();
  delegate_->UpdateLayoutNodeStyle(id, css_id, value);
}

void ElementManager::ResetLayoutNodeStyle(int32_t id,
                                          tasm::CSSPropertyID css_id) {
  SetNeedsLayout();
  delegate_->ResetLayoutNodeStyle(id, css_id);
}

void ElementManager::UpdateLayoutNodeAttribute(int32_t id,
                                               starlight::LayoutAttribute key,
                                               const lepus::Value &value) {
  SetNeedsLayout();
  delegate_->UpdateLayoutNodeAttribute(id, key, value);
}

void ElementManager::SetFontFaces(const tasm::CSSFontFaceRuleMap &fontfaces) {
  delegate_->SetFontFaces(fontfaces);
}

void ElementManager::AddFontFace(const lepus::Value &font) {
  if (!font.IsTable()) {
    return;
  }
  BASE_STATIC_STRING_DECL(kFontFamily, "font-family");
  CSSFontFaceRuleMap map;
  auto token = std::shared_ptr<CSSFontFaceRule>(MakeCSSFontFaceToken(font));
  const std::string &key = font.Table()->GetValue(kFontFamily).StdString();
  if (key.empty()) {
    return;
  }
  map[key] = {token};
  delegate_->SetFontFaces(map);
}

void ElementManager::UpdateLayoutNodeProps(
    int32_t id, const std::shared_ptr<tasm::PropBundle> &props) {
  delegate_->UpdateLayoutNodeProps(id, props);
}

void ElementManager::UpdateLayoutNodeByBundle(
    int32_t id, std::unique_ptr<LayoutBundle> bundle) {
  delegate_->UpdateLayoutNodeByBundle(id, std::move(bundle));
}

int32_t ElementManager::GetNodeInfoByTag(const base::String &tag_name) {
  auto it = node_info_recorder_.find(tag_name);
  if (it != node_info_recorder_.end()) {
    return it->second;
  }
  int32_t result = painting_context()->GetTagInfo(tag_name.str());
  node_info_recorder_.emplace(tag_name, result);
  return result;
}

bool ElementManager::IsShadowNodeVirtual(const base::String &tag_name) {
  return GetNodeInfoByTag(tag_name) & LayoutNodeType::VIRTUAL;
}

void ElementManager::MarkLayoutDirty(int32_t id) {
  delegate_->MarkLayoutDirty(id);
}

void ElementManager::AttachLayoutNodeType(
    int32_t id, const base::String &tag, bool allow_inline,
    const std::shared_ptr<PropBundle> &props) {
  delegate_->AttachLayoutNodeType(id, tag, allow_inline, props);
}

void ElementManager::UpdateTouchPseudoStatus(bool value) {
  push_touch_pseudo_flag_ = value;
}

void ElementManager::SetConfig(const std::shared_ptr<PageConfig> &config) {
  config_ = config;
  // Apply pagewise configs
  if (config_) {
    painting_context()->SetEnableVsyncAlignedFlush(
        config_->GetEnableVsyncAlignedFlush());
    lynx_env_config_.SetFontScaleSpOnly(GetLayoutConfigs().font_scale_sp_only_);
    delegate_->SetPageConfigForLayoutThread(config_);
  }
}

void ElementManager::AppendTimingFlag(std::string flag) {
  attribute_timing_flag_list_.Push(std::move(flag));
}

void ElementManager::BindTimingFlagToPipelineOptions(PipelineOptions &options) {
  auto timing_flag = ObtainTimingFlagList();
  if (!timing_flag.empty()) {
    options.need_timestamps = true;
    for (const auto &attribute_timing_flag : timing_flag) {
      delegate_->BindPipelineIDWithTimingFlag(options.pipeline_id,
                                              attribute_timing_flag);
    }
  }
}

void ElementManager::SetNeedsLayout() { need_layout_ = true; }

void ElementManager::RequestNextFrame(Element *element) {
  animation_element_set_.insert(element);
  if (element_vsync_proxy_ == nullptr) {
    element_vsync_proxy_ = std::make_shared<ElementVsyncProxy>(
        ElementVsyncProxy(this, vsync_monitor_));
  }
  element_vsync_proxy_->SetPreferredFps(config_->GetPreferredFps());
  element_vsync_proxy_->RequestNextFrame();
}

void ElementManager::NotifyElementDestroy(Element *element) {
  animation_element_set_.erase(element);
  paused_animation_element_set_.erase(element);
}

void ElementManager::TickAllElement(fml::TimePoint &frame_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementManager::TickAllElement");
  if (element_vsync_proxy_) {
    PipelineOptions options;
    auto temp_element_set = std::unordered_set<Element *>();
    // We should swap all element to a temporary set before when we tick them.
    temp_element_set.swap(animation_element_set_);
    if (!temp_element_set.empty()) {
      bool has_layout_animated_style = false;
      for (auto iter : temp_element_set) {
        // tick element, for List.
        iter->TickElement(frame_time);

        // tick element, for Animation.
        if (iter->TickAllAnimation(frame_time, options)) {
          has_layout_animated_style = true;
        }
      }
      if (!has_layout_animated_style) {
        painting_context()->UpdateNodeReadyPatching();
        painting_context()->Flush();
      } else {
        // Optimization: If there is only an element need to be ticked, take
        // it as root to flush action.
        if (temp_element_set.size() == 1) {
          OnPatchFinish(options, static_cast<tasm::FiberElement *>(
                                     *temp_element_set.begin()));
        } else {
          OnPatchFinish(options);
        }
      }
    }
  }
}

void ElementManager::PauseAllAnimations() {
  LOGI("Call ElementManager::PauseAllAnimations.");
  animations_paused_ = true;
}

void ElementManager::ResumeAllAnimations() {
  LOGI("Call ElementManager::ResumeAllAnimations.");
  animations_paused_ = false;
  // TODO(wangyifei.20010605): Can't pause running animations, fix later.
  for (const auto &ele : paused_animation_element_set_) {
    ele->SetDataToNativeKeyframeAnimator(true);
  }
  paused_animation_element_set_.clear();
}

void ElementManager::SetGlobalBindElementId(const base::String &name,
                                            const base::String &type,
                                            const int node_id) {
  auto &map = global_bind_name_to_ids_;
  auto name_str = name.str();
  if (name_str.empty()) {
    return;
  }
  auto iter = map.find(name_str);
  if (iter == map.end()) {
    std::set<int> set;
    set.insert(node_id);
    map[name_str] = std::move(set);
  } else {
    iter->second.insert(node_id);
  }
}

void ElementManager::EraseGlobalBindElementId(const EventMap &global_event_map,
                                              const int node_id) {
  if (global_event_map.empty()) {
    return;
  }
  auto &map = global_bind_name_to_ids_;
  for ([[maybe_unused]] auto &[key, event] : map) {
    event.erase(node_id);
  }
}

std::set<int> ElementManager::GetGlobalBindElementIds(
    const std::string &name) const {
  auto &map = global_bind_name_to_ids_;

  auto iter = map.find(name);
  if (iter != map.end()) {
    return iter->second;
  }
  return {};
}

bool ElementManager::Hydrate(AttributeHolder *node, Element *shadow_node) {
  if (node->static_events().empty() && node->lepus_events().empty()) {
    return false;
  }

  for (const auto &event : node->static_events()) {
    shadow_node->SetEventHandler(event.first, event.second.get());
  }

  for (const auto &lepus_event : node->lepus_events()) {
    shadow_node->SetEventHandler(lepus_event.first, lepus_event.second.get());
  }

  return true;
}

fml::RefPtr<FiberElement> ElementManager::CreateFiberElement(
    const base::String &raw_tag) {
  return CreateFiberElement(ElementProperty::ConvertStringTagToEnumTag(raw_tag),
                            raw_tag);
}

fml::RefPtr<FiberElement> ElementManager::CreateFiberElement(
    ElementBuiltInTagEnum enum_tag, const base::String &raw_tag) {
  auto result = StaticCreateFiberElement(enum_tag, raw_tag);
  result->AttachToElementManager(this, nullptr, false);
  return result;
}

fml::RefPtr<FiberElement> ElementManager::StaticCreateFiberElement(
    ElementBuiltInTagEnum enum_tag, const base::String &raw_tag) {
  fml::RefPtr<FiberElement> element = nullptr;
  switch (enum_tag) {
    case ELEMENT_VIEW:
      element = fml::AdoptRef<ViewElement>(new ViewElement(nullptr));
      break;
    case ELEMENT_IMAGE:
      element = fml::AdoptRef<ImageElement>(
          new ImageElement(nullptr, BASE_STATIC_STRING(kElementImageTag)));
      break;
    case ELEMENT_TEXT:
      element = fml::AdoptRef<TextElement>(
          new TextElement(nullptr, BASE_STATIC_STRING(kElementTextTag)));
      break;
    case ELEMENT_X_TEXT:
      element = fml::AdoptRef<TextElement>(
          new TextElement(nullptr, BASE_STATIC_STRING(kElementXTextTag)));
      break;
    case ELEMENT_INLINE_TEXT:
      element = fml::AdoptRef<TextElement>(
          new TextElement(nullptr, BASE_STATIC_STRING(kElementTextTag)));
      break;
    case ELEMENT_X_INLINE_TEXT:
      element = fml::AdoptRef<TextElement>(
          new TextElement(nullptr, BASE_STATIC_STRING(kElementXTextTag)));
      break;
    case ELEMENT_RAW_TEXT:
      element = fml::AdoptRef<RawTextElement>(new RawTextElement(nullptr));
      break;
    case ELEMENT_SCROLL_VIEW:
      element = fml::AdoptRef<ScrollElement>(new ScrollElement(
          nullptr, BASE_STATIC_STRING(kElementScrollViewTag)));
      break;
    case ELEMENT_X_SCROLL_VIEW:
      element = fml::AdoptRef<ScrollElement>(new ScrollElement(
          nullptr, BASE_STATIC_STRING(kElementXScrollViewTag)));
      break;
    case ELEMENT_X_NESTED_SCROLL_VIEW:
      element = fml::AdoptRef<ScrollElement>(new ScrollElement(
          nullptr, BASE_STATIC_STRING(kElementXNestedScrollViewTag)));
      break;
    case ELEMENT_LIST:
      element = fml::AdoptRef<ListElement>(
          new ListElement(nullptr, BASE_STATIC_STRING(kElementListTag),
                          lepus::Value(), lepus::Value(), lepus::Value()));
      break;
    case ELEMENT_NONE:
      element = fml::AdoptRef<NoneElement>(new NoneElement(nullptr));
      break;
    case ELEMENT_WRAPPER:
      element = fml::AdoptRef<WrapperElement>(new WrapperElement(nullptr));
      break;
    case ELEMENT_COMPONENT: {
      base::String empty_string;
      // When constructing the component element, the component ID, CSS ID,
      // and path cannot be obtained yet, so default values are assigned
      // initially. Later, during the decoding of the built-in attribute
      // section, these values within the component element will be updated.
      element = fml::AdoptRef<ComponentElement>(
          new ComponentElement(nullptr, empty_string, -1,
                               BASE_STATIC_STRING(tasm::DEFAULT_ENTRY_NAME),
                               empty_string, empty_string));
      break;
    }
    case ELEMENT_PAGE:
      // When constructing the page element, the component ID and CSS ID
      // cannot be obtained yet, so default values are assigned initially.
      // Later, during the decoding of the built-in attribute section, these
      // values within the page element will be updated.
      element = fml::AdoptRef<PageElement>(
          new PageElement(nullptr, base::String(), -1));
      break;
    default:
      element = fml::AdoptRef<FiberElement>(new FiberElement(nullptr, raw_tag));
  }
  return element;
}

fml::RefPtr<FiberElement> ElementManager::CreateFiberNode(
    const base::String &tag) {
  auto res = fml::AdoptRef<FiberElement>(new FiberElement(this, tag));
  return res;
}

fml::RefPtr<PageElement> ElementManager::CreateFiberPage(
    const base::String &component_id, int32_t css_id) {
  return fml::AdoptRef<PageElement>(
      new PageElement(this, component_id, css_id));
}

fml::RefPtr<ComponentElement> ElementManager::CreateFiberComponent(
    const base::String &component_id, int32_t css_id,
    const base::String &entry_name, const base::String &name,
    const base::String &path) {
  auto res = fml::AdoptRef<ComponentElement>(
      new ComponentElement(this, component_id, css_id, entry_name, name, path));
  return res;
}

fml::RefPtr<ViewElement> ElementManager::CreateFiberView() {
  auto res = fml::AdoptRef<ViewElement>(new ViewElement(this));
  return res;
}

fml::RefPtr<ImageElement> ElementManager::CreateFiberImage(
    const base::String &tag) {
  auto res = fml::AdoptRef<ImageElement>(new ImageElement(this, tag));
  return res;
}

fml::RefPtr<TextElement> ElementManager::CreateFiberText(
    const base::String &tag) {
  auto res = fml::AdoptRef<TextElement>(new TextElement(this, tag));
  return res;
}

fml::RefPtr<RawTextElement> ElementManager::CreateFiberRawText() {
  return fml::AdoptRef<RawTextElement>(new RawTextElement(this));
}

fml::RefPtr<ScrollElement> ElementManager::CreateFiberScrollView(
    const base::String &tag) {
  auto res = fml::AdoptRef<ScrollElement>(new ScrollElement(this, tag));
  return res;
}

fml::RefPtr<ListElement> ElementManager::CreateFiberList(
    tasm::TemplateAssembler *tasm, const base::String &tag,
    const lepus::Value &component_at_index,
    const lepus::Value &enqueue_component,
    const lepus::Value &component_at_indexes) {
  auto res = fml::AdoptRef<ListElement>(new ListElement(
      this, tag, component_at_index, enqueue_component, component_at_indexes));
  res->set_tasm(tasm);
  return res;
}

fml::RefPtr<NoneElement> ElementManager::CreateFiberNoneElement() {
  auto res = fml::AdoptRef<NoneElement>(new NoneElement(this));
  return res;
}

fml::RefPtr<WrapperElement> ElementManager::CreateFiberWrapperElement() {
  auto res = fml::AdoptRef<WrapperElement>(new WrapperElement(this));
  return res;
}

void ElementManager::OnPatchFinish(PipelineOptions &option, Element *element) {
  if (element == nullptr) {
    element = static_cast<Element *>(root());
  }
  if (!element) {
    LOGE("ElementManager::OnPatchFinish failed since element is nullptr.");
    return;
  }
  if (element->is_radon_element()) {
    OnPatchFinishForRadon(option);
  } else if (element->is_fiber_element()) {
    OnPatchFinishForFiber(option, static_cast<FiberElement *>(element));
  }
  if (option.need_timestamps) {
    report::EventTracker::UpdateGenericInfo(
        instance_id_, kEventDomSizeKey,
        static_cast<int64_t>(element_count_.load()));
  }
}

void ElementManager::OnPatchFinishForFiber(PipelineOptions &options,
                                           FiberElement *element) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementManager::OnPatchFinishInner");
  if (options.need_timestamps) {
    painting_context()->MarkUIOperationQueueFlushTiming(
        tasm::timing::kPaintingUiOperationExecuteStart, options.pipeline_id);
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kResolveStart);
  }
  if (options.enable_report_list_item_life_statistic_ &&
      options.IsRenderListItem()) {
    options.list_item_life_option_.start_dispatch_time_ =
        base::CurrentTimeMicroseconds();
  }

  if (options.force_update_style_sheet_) {
    // When force_update_style_sheet_ is true, need recursively traverse the
    // entire tree to mark dirty and reset style sheet.
    element->ApplyFunctionRecursive([](FiberElement *element) {
      element->ResetStyleSheet();
      element->MarkStyleDirty();
    });
  } else if (options.force_resolve_style_) {
    // When force_resolve_style_ is true, need recursively traverse the entire
    // tree to mark dirty.
    element->MarkStyleDirty(true);
  }
  if (options.is_reload_template && config_ &&
      config_->GetEnableReloadLifecycle()) {
    element->ApplyFunctionRecursive(
        [](FiberElement *element) { element->onNodeReload(); });
    catalyzer_->painting_context()->UpdateNodeReloadPatching();
  }
  element->FlushActionsAsRoot();

  BindTimingFlagToPipelineOptions(options);

  if (options.need_timestamps) {
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kResolveEnd);
  }
  if (options.enable_report_list_item_life_statistic_ &&
      options.IsRenderListItem()) {
    options.list_item_life_option_.end_dispatch_time_ =
        base::CurrentTimeMicroseconds();
  }

  catalyzer_->painting_context()->FinishTasmOperation(options);

  // if flush_option do not need layout or options do not need layout, skip
  // layout.
  if (!need_layout_ || !options.trigger_layout_) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "ElementManager::OnPatchFinishForFiberNoPatch");
    LOGI("ElementManager::OnPatchFinishForFiber NoPatch!");

    // When list render a child which is obtained from pool, it may has no patch
    // and don't trigger layout, so we need to invoke OnComponentFinish to
    // notify list that child has been rendered.
    OnListComponentUpdated(options);
    catalyzer_->painting_context()->FinishLayoutOperation(options);
    delegate_->OnUpdateDataWithoutChange();
  } else {
    LOGI("ElementManager::OnPatchFinishForFiber WithPatch!");
    {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementManager::UpdateZIndexList");
      // sort z-index children
      for (const auto &context : dirty_stacking_contexts_) {
        context->UpdateZIndexList();
      }
    }
    dirty_stacking_contexts_.clear();
    RequestLayout(options);
    need_layout_ = false;
  }

  // Only when the root node of FlushActionsAsRoot is a direct child of the
  // list, calling FlushImmediately ensures that the generated operation can be
  // executed immediately.
  if (element != nullptr && element->is_list_item()) {
    painting_context()->FlushImmediately();
  }

  DidPatchFinishForFiber();
}

int32_t ElementManager::GenerateElementID() { return element_id_++; }

void ElementManager::ReuseElementID(int32_t reuse_id) {
  element_id_ = element_id_ > reuse_id ? element_id_ : reuse_id + 1;
}

void ElementManager::RecordComponent(const std::string &id, Element *node) {
  if (component_manager_) {
    component_manager_->Record(id, node);
  }
}

void ElementManager::EraseComponentRecord(const std::string &id,
                                          Element *node) {
  if (component_manager_) {
    component_manager_->Erase(id, node);
  }
}

Element *ElementManager::GetComponent(const std::string &id) {
  if (id.empty() || id == PAGE_ID) {
    if (fiber_page_) {
      return fiber_page_.get();
    }
  }
  if (component_manager_) {
    return component_manager_->Get(id);
  }
  return nullptr;
}

void ElementManager::OnListComponentUpdated(const PipelineOptions &options) {
  if (options.operation_id != 0 && options.list_id_ != 0 &&
      options.list_comp_id_ != 0 && node_manager_) {
    Element *list = node_manager_->Get(options.list_id_);
    Element *component = node_manager_->Get(options.list_comp_id_);
    if (list && list->DisableListPlatformImplementation() && component) {
      list->OnComponentFinished(component, options);
    }
  }
}

void ElementManager::OnErrorOccurred(base::LynxError error) {
  delegate_->OnErrorOccurred(std::move(error));
}

void ElementManager::SetEnableUIOperationOptimize(TernaryBool enable) {
  if (enable == TernaryBool::TRUE_VALUE ||
      LynxEnv::GetInstance().EnableUIOpBatch()) {
    painting_context()->EnableUIOperationBatching();
  }
}

void ElementManager::SetEnableFiberElementForRadonDiff(TernaryBool value) {
  if (value == TernaryBool::TRUE_VALUE) {
    enable_fiber_element_for_radon_diff_ = true;
  } else if (value == TernaryBool::FALSE_VALUE) {
    enable_fiber_element_for_radon_diff_ = false;
  } else {
    enable_fiber_element_for_radon_diff_ = LynxEnv::GetInstance().GetBoolEnv(
        lynx::tasm::LynxEnv::Key::ENABLE_FIBER_ELEMENT_FOR_RADON_DIFF, false);
  }
}

namespace {
void ClearExtremeParsedStylesRecursively(FiberElement *cur) {
  cur->ClearExtremeParsedStyles();
  for (auto &child : cur->children()) {
    ClearExtremeParsedStylesRecursively(child.get());
  }
}
}  // namespace

void ElementManager::ClearExtremeParsedStyles() {
  if (likely(root_ && root_->is_fiber_element())) {
    ClearExtremeParsedStylesRecursively(static_cast<FiberElement *>(root()));
  }
}

}  // namespace tasm
}  // namespace lynx
