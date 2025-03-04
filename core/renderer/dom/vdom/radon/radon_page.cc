// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/radon_page.h"

#include <numeric>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/string/string_number_convert.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/base/base_def.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/base/tasm_utils.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/path_parser.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/long_task_timing/long_task_monitor.h"
#include "core/services/timing_handler/timing.h"
#include "core/services/timing_handler/timing_constants.h"

namespace lynx {
namespace tasm {

namespace {

void ReportNoPatch(bool has_patch, uint64_t start_time,
                   std::string component_name = "RootComponent") {
  if (has_patch) {
    return;
  }
  auto duration =
      static_cast<int>(base::CurrentTimeMicroseconds() - start_time);

  tasm::report::EventTracker::OnEvent(
      [duration, component_name = std::move(component_name)](
          tasm::report::MoveOnlyEvent &event) {
        event.SetName("lynxsdk_no_patch");
        event.SetProps("component_name", component_name);
        event.SetProps("duration_microsecond", duration);
      });
}

}  // namespace

RadonPage::RadonPage(PageProxy *proxy, int tid, CSSFragment *style_sheet,
                     std::shared_ptr<CSSStyleSheetManager> style_sheet_manager,
                     PageMould *mould, lepus::Context *context)
    : RadonComponent(proxy, tid, style_sheet, style_sheet_manager, mould,
                     context, kRadonInvalidNodeIndex,
                     BASE_STATIC_STRING(kRadonPageTag)),
      proxy_(proxy) {
  node_type_ = kRadonPage;
  if (!context_) {
    return;
  }
  if (proxy_) {
    bool enable_css_lazy_import =
        page_proxy_->element_manager()->GetEnableCSSLazyImport();
    if (style_sheet_manager) {
      style_sheet_manager->SetEnableCSSLazyImport(enable_css_lazy_import);
    }
  }
  entry_name_ = context_->name();
  UpdatePageData(kSystemInfo, GenerateSystemInfo(nullptr), true);
}

fml::RefPtr<Element> RadonPage::CreateFiberElement() {
  auto fiber_element = page_proxy_->element_manager()->CreateFiberPage(
      ComponentStrId(), GetCSSId());
  fiber_element->SetAttributeHolder(attribute_holder_);
  fiber_element->set_style_sheet_manager(style_sheet_manager());
  return fiber_element;
}

void RadonPage::DeriveFromMould(ComponentMould *data) {
  if (data == nullptr) {
    return;
  }
  auto init_data = data->data();
  if (!init_data.IsObject()) {
    return;
  }
  ForEachLepusValue(init_data,
                    [this](const lepus::Value &key, const lepus::Value &value) {
                      context_->UpdateTopLevelVariable(key.StdString(), value);
                    });
}

RadonPage::~RadonPage() {
  if (page_proxy_ && page_proxy_->element_manager()) {
    page_proxy_->element_manager()->SetRoot(nullptr);
  }
}

void RadonPage::UpdateComponentData(const std::string &id,
                                    const lepus::Value &table,
                                    PipelineOptions &pipeline_options) {
  uint64_t start_time = base::CurrentTimeMicroseconds();
  ResetComponentDispatchOrder();
  int i_id = atoi(id.c_str());
  if (proxy_->GetComponentMap().find(i_id) != proxy_->GetComponentMap().end()) {
    RadonComponent *component = proxy_->GetComponentMap()[i_id];
    timing::LongTaskTiming *timing =
        tasm::timing::LongTaskMonitor::Instance()->GetTopTimingPtr();
    if (timing != nullptr) {
      timing->task_name_ = component->name().str();
      timing->task_info_ = ConcatenateTableKeys(table);
    }

    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "UpdateComponentData",
                [&](lynx::perfetto::EventContext ctx) {
                  std::string info = ConcatUpdateDataInfo(component, table);
                  LOGI(info);
                  auto *debug = ctx.event()->add_debug_annotations();
                  debug->set_name("Info");
                  debug->set_string_value(info);
                });
    DispatchOption dispatch_option(page_proxy_);
    component->UpdateRadonComponent(
        RadonComponent::RenderType::UpdateFromJSBySelf, lepus::Value(), table,
        dispatch_option, pipeline_options);
    TriggerComponentLifecycleUpdate(kComponentAttached);
    if (dispatch_option.has_patched_) {
      page_proxy_->element_manager()->SetNeedsLayout();
    };
    if (proxy_->EnableFeatureReport()) {
      ReportNoPatch(dispatch_option.has_patched_, start_time,
                    component->path().str());
    }
    page_proxy_->element_manager()->OnPatchFinish(pipeline_options);
    TriggerComponentLifecycleUpdate(kComponentReady);
  }
}

bool RadonPage::NeedsExtraData() const {
  if (page_proxy_ == nullptr) {
    return true;
  }

  if (page_proxy_->IsServerSideRendering()) {
    // For SSR, currently we kept old behavior
    return true;
  }

  return !page_proxy_->GetEnableRemoveComponentExtraData();
}

std::unique_ptr<lepus::Value> RadonPage::GetPageData() {
  if (ShouldKeepPageData()) {
    return std::make_unique<lepus::Value>(lepus::Value::Clone(data_));
  } else {
    return context_->GetTopLevelVariable(true);
  }
}

// acquire specified value from page data.
lepus::Value RadonPage::GetPageDataByKey(const std::vector<std::string> &keys) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "GetPageDataByKey",
              [&keys](perfetto::EventContext ctx) {
                ctx.event()->set_name("GetPageDataByKey");
                auto *debug = ctx.event()->add_debug_annotations();
                debug->set_name("keys");
                std::string str =
                    std::accumulate(keys.begin(), keys.end(), std::string{},
                                    [](std::string &s1, const std::string &s2) {
                                      return s1.append(",").append(s2);
                                    });
                debug->set_string_value(str);
              });

  using TopLevelVariableFinder =
      base::MoveOnlyClosure<lepus::Value, const std::string &>;

  // if ShouldKeepPageData, find from data, else find from context
  auto finder =
      ShouldKeepPageData()
          ? TopLevelVariableFinder{[&data = data_](const std::string &key) {
              return data.GetProperty(key);
            }}
          : TopLevelVariableFinder{[ctx = context_](const std::string &key) {
              auto val = lepus::Value();
              ctx->GetTopLevelVariableByName(key, &val);
              return val;
            }};

  lepus::Value result = lepus::Value(lepus::Dictionary::Create());

  std::for_each(keys.cbegin(), keys.cend(),
                [&result, &finder](const std::string &key) {
                  result.Table()->SetValue(key, finder(key));
                });

  return result;
}

// comp_id == "" or "card" indicates the component to get is the card
RadonComponent *RadonPage::GetComponent(const std::string &comp_id) {
  if (comp_id.empty() || comp_id == PAGE_ID) {
    return this;
  }

  int i_id;
  if (!base::StringToInt(comp_id, &i_id, 10)) {
    return nullptr;
  }
  auto it = proxy_->GetComponentMap().find(i_id);
  if (it == proxy_->GetComponentMap().end()) {
    return nullptr;
  };
  return it->second;
}

bool RadonPage::UpdatePage(const lepus::Value &table,
                           const UpdatePageOption &update_page_option,
                           PipelineOptions &pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxUpdateData",
              [&](lynx::perfetto::EventContext ctx) {
                std::string info = ConcatUpdateDataInfo(this, table);
                LOGI(info);
                auto *debug = ctx.event()->add_debug_annotations();
                debug->set_name("Info");
                debug->set_string_value(info);
                std::string defaultInfo = ConcatUpdateDataInfo(this, data_);
                auto *debug_default_data = ctx.event()->add_debug_annotations();
                debug_default_data->set_name("defaultData");
                debug_default_data->set_string_value(defaultInfo);
              });
  auto *timing = tasm::timing::LongTaskMonitor::Instance()->GetTopTimingPtr();
  if (timing != nullptr) {
    timing->task_name_ = "root";
    timing->task_info_ = ConcatenateTableKeys(table);
  }

  uint64_t start_time = base::CurrentTimeMicroseconds();
  // UpdateFromJSBySelf
  if (!update_page_option.from_native &&
      !update_page_option.update_first_time) {
    SetRenderType(RenderType::UpdateFromJSBySelf);
    if (IsReact() && CheckReactShouldAbortUpdating(table)) {
      return false;
    }
  } else if (update_page_option.update_first_time) {
    // FirstRender
    SetRenderType(RenderType::FirstRender);
  } else if (update_page_option.from_native) {
    // UpdateByNative
    SetRenderType(RenderType::UpdateByNative);

    // TODO(wangqingyu): TT should also reset when support data version
    if (IsReact() && update_page_option.reload_template) {
      // For reload template, we should reset data versions
      // since js counter parts are re-created with init version
      // Otherwise, all setState will be aborted
      ResetDataVersions();
    }

    if (should_component_update_function_.IsCallable()) {
      set_pre_data(lepus::Value::ShallowCopy(data_));
      set_pre_properties(lepus::Value::ShallowCopy(properties_));
    }
  }

  bool need_update = false;
  bool should_component_render = true;
  if (update_page_option.reset_page_data ||
      update_page_option.reload_template || update_page_option.reload_from_js) {
    need_update = ResetPageData();
  }
  if (update_page_option.global_props_changed ||
      update_page_option.reload_from_js) {
    // when native update global props or reload from JS, need trigger children
    // render
    need_update = true;
  }
  if (enable_check_data_when_update_page_ &&
      !update_page_option.update_first_time &&
      !update_page_option.global_props_changed &&
      !update_page_option.reload_from_js) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS,
                "RadonPage::UpdatePage::CheckTableShouldUpdated");
    bool update_data_is_equal = false;
    if (ShouldKeepPageData()) {
      if (data_.IsObject()) {
        update_data_is_equal = !CheckTableShadowUpdated(data_, table);
      }
    } else {
      update_data_is_equal =
          !context_->CheckTableShadowUpdatedWithTopLevelVariable(table);
    }
    if (update_data_is_equal) {
      if (page_proxy_->GetPrePaintingStage() ==
          PrePaintingStage::kStartUpdatePage) {
        // when trigger lifecycle after pre_painting, we should trigger
        // OnReactCardRender even if update_data_is_equal.
        if (IsReact()) {
          lepus::Value merged_data = lepus::Value(lepus::Dictionary::Create());
          ForcePreprocessPageData(table, merged_data);
          proxy_->OnReactCardRender(merged_data, true);
        }
        DispatchOption trigger_option(proxy_);
        triggerNewLifecycle(trigger_option);
      }
      pipeline_options.native_update_data_order_ =
          update_page_option.native_update_data_order_;
      page_proxy_->element_manager()->OnPatchFinish(pipeline_options);
      return need_update;
    }
  }
  ForEachLepusValue(
      table, [this, &need_update, &should_component_render](
                 const lepus::Value &key, const lepus::Value &value) {
        if (key.StdString() == REACT_SHOULD_COMPONENT_UPDATE_KEY) {
          should_component_render = value.Bool();
          return;
        }
        if (UpdatePageData(key.StdString(), value)) {
          need_update = true;
        }
      });

  if (!should_component_render) {
    return need_update;
  }
  ResetComponentDispatchOrder();
  bool should_component_update = PrePageRender(table, update_page_option);
  DispatchOption option(page_proxy_);
  this->attribute_holder()->Reset();
  {
    // using radon diff
    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "RadonPage::UpdatePage::RadonDiff");
    if (update_page_option.update_first_time) {
      tasm::TimingCollector::Instance()->Mark(tasm::timing::kMtsRenderStart);
      lepus::Value p1(this);
      lepus::Value p2(true);
      std::string ss = "$renderPage" + std::to_string(this->node_index_);
      if (page_proxy_->element_manager()->GetEnableFiberElementForRadonDiff()) {
        // In Radon-Fiber Arch, element is held by parent element and RadonNode
        // together. So we should manually remove the old element from parent.
        for (auto &node : radon_children_) {
          if (node) {
            node->RemoveElementFromParent();
          }
        }
      }
      radon_children_.clear();
      dispatched_ = false;
      // Before lynx 2.1, $renderPage accept only the previous two params.
      context_->Call(ss, p1, p2, data_);
      tasm::TimingCollector::Instance()->Mark(tasm::timing::kMtsRenderEnd);
      // when the page is first updated
      tasm::TimingCollector::Instance()->Mark(tasm::timing::kResolveStart);
      if (!proxy_->HasSSRRadonPage() && !proxy_->IsServerSideRendering()) {
        page_proxy_->element_manager()
            ->painting_context()
            ->MarkUIOperationQueueFlushTiming(
                tasm::timing::kPaintingUiOperationExecuteStart,
                pipeline_options.pipeline_id);
      }
      option.ignore_component_lifecycle_ = page_proxy_->GetPrePaintingStage() !=
                                           PrePaintingStage::kPrePaintingOFF;
      DispatchForDiff(option);
      tasm::TimingCollector::Instance()->Mark(tasm::timing::kResolveEnd);
    } else if (need_update) {
      option.ignore_component_lifecycle_ = page_proxy_->GetPrePaintingStage() !=
                                           PrePaintingStage::kPrePaintingOFF;
      // no first screen, check shouldComponent update
      if (!should_component_update) {
        LOGI("should_component_update is false in RadonPage::UpdatePage.");
        return need_update;
      }
      if (pipeline_options.need_timestamps) {
        tasm::TimingCollector::Instance()->Mark(tasm::timing::kMtsRenderStart);
      }
      /*
       * original_radon_children will save the original children of a radon
       * page. After finishing rendering new page, do diff on
       * original_radon_children and new radon_children_ of the radon_page
       */
      auto original_radon_children = std::move(radon_children_);
      radon_children_.clear();
      option.force_diff_entire_tree_ = update_page_option.reload_template;
      option.use_new_component_data_ = update_page_option.reload_template;
      option.refresh_lifecycle_ = update_page_option.reload_template;
      option.global_properties_changed_ =
          update_page_option.global_props_changed;
      lepus::Value p1(this);
      // No need to render subTree recursively.
      // SubComponent will render by itself during diff.
      lepus::Value p2(false);
      lepus::Value p3(data_);
      std::string ss = "$renderPage" + std::to_string(this->node_index_);
      // Before lynx 2.1, $renderPage accept only the previous two params.
      context_->Call(ss, p1, p2, p3);
      if (element() != nullptr) {
        EXEC_EXPR_FOR_INSPECTOR(NotifyElementNodeSetted());
      }
      if (pipeline_options.need_timestamps) {
        tasm::TimingCollector::Instance()->Mark(tasm::timing::kMtsRenderEnd);
        page_proxy_->element_manager()
            ->painting_context()
            ->MarkUIOperationQueueFlushTiming(
                tasm::timing::kPaintingUiOperationExecuteStart,
                pipeline_options.pipeline_id);
      }
      PreHandlerCSSVariable();
      if (pipeline_options.need_timestamps) {
        tasm::TimingCollector::Instance()->Mark(tasm::timing::kResolveStart);
      }
      RadonMyersDiff(original_radon_children, option);
      if (pipeline_options.need_timestamps) {
        tasm::TimingCollector::Instance()->Mark(tasm::timing::kResolveEnd);
      }
    }
    if (page_proxy_->GetPrePaintingStage() ==
        PrePaintingStage::kStartUpdatePage) {
      DispatchOption trigger_option(proxy_);
      triggerNewLifecycle(trigger_option);
    }
  }

  OnReactComponentDidUpdate(option);
  if (page_proxy_->GetPrePaintingStage() == PrePaintingStage::kPrePaintingOFF) {
    TriggerComponentLifecycleUpdate(kComponentAttached);
  }

  pipeline_options.is_first_screen = update_page_option.update_first_time;
  pipeline_options.is_reload_template = update_page_option.reload_template;
  if (option.has_patched_) {
    page_proxy_->element_manager()->SetNeedsLayout();
  };
  pipeline_options.native_update_data_order_ =
      update_page_option.native_update_data_order_;
  if (!proxy_->HasSSRRadonPage() && !proxy_->IsServerSideRendering()) {
    if (proxy_->EnableFeatureReport()) {
      ReportNoPatch(option.has_patched_, start_time);
    }
    page_proxy_->element_manager()->OnPatchFinish(pipeline_options);
  }
  if (page_proxy_->GetPrePaintingStage() == PrePaintingStage::kPrePaintingOFF) {
    TriggerComponentLifecycleUpdate(kComponentReady);
  }
  return need_update;
}

#if ENABLE_TRACE_PERFETTO
std::string RadonPage::ConcatUpdateDataInfo(const RadonComponent *comp,
                                            const lepus::Value &table) const {
  /* Concatenate all the keys in updating data */
  std::stringstream ss;
  if (comp->IsRadonPage()) {
    ss << "Update Root Component: ";
  } else {
    ss << "component_name: " << comp->name().str();
  }
  ss << "       Keys: ";
  ss << ConcatenateTableKeys(table);
  return ss.str();
}
#endif

std::string RadonPage::ConcatenateTableKeys(const lepus::Value &table) const {
  std::stringstream ss;
  ForEachLepusValue(table,
                    [&ss](const lepus::Value &key, const lepus::Value &val) {
                      const auto &key_str = key.StdString();
                      if (key_str != REACT_NATIVE_STATE_VERSION_KEY &&
                          key_str != REACT_JS_STATE_VERSION_KEY) {
                        ss << key_str << ",";
                      }
                    });
  return ss.str();
}

void RadonPage::DispatchSelf(const DispatchOption &option) {
  if (!page_proxy_->GetPageElementEnabled() && option.need_update_element_ &&
      !option.ssr_hydrating_ && CreateElementIfNeeded()) {
    page_proxy_->element_manager()->SetRootOnLayout(element()->impl_id());
    page_proxy_->element_manager()->catalyzer()->set_root(element());
    page_proxy_->element_manager()->SetRoot(element());
    option.has_patched_ = true;
    DispatchFirstTime();
  } else if (option.ssr_hydrating_) {
    AttachSSRPageElement(page_proxy_->SSRPage());
    page_proxy_->element_manager()->SetRoot(this->element());
  }
}

void RadonPage::AttachSSRPageElement(RadonPage *ssr_page) {
  element_ = std::move(ssr_page->element_);
  element_->SetAttributeHolder(this->attribute_holder_);
  if (element_->is_fiber_element()) {
    fiber_element()->ResetSheetRecursively(style_sheet_manager());
  }
}

void RadonPage::Dispatch(const DispatchOption &option) {
  RadonNode::Dispatch(option);
}

void RadonPage::DispatchForDiff(const DispatchOption &option) {
  RadonNode::DispatchForDiff(option);
}

bool RadonPage::RefreshWithGlobalProps(const lynx::lepus::Value &table,
                                       bool should_render,
                                       PipelineOptions &pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RefreshWithGlobalProps",
              [&should_render](lynx::perfetto::EventContext ctx) {
                auto *debug = ctx.event()->add_debug_annotations();
                debug->set_name("should_render");
                debug->set_bool_value(should_render);
              });
  if (!context_) {
    return false;
  }
  DCHECK(table.IsObject());

  auto data = lepus::Value(lepus::Dictionary::Create());
  UpdatePageOption update_page_option;
  update_page_option.from_native = true;
  update_page_option.global_props_changed = true;

  auto kGlobalPropsKey_str = BASE_STATIC_STRING(kGlobalPropsKey);

  // update globalProps to topVar unconditionally.
  context_->UpdateTopLevelVariable(kGlobalPropsKey_str.str(), table);

  if (!NeedsExtraData()) {
    if (should_render) {
      // this is called by native update global props
      // should call UpdatePage with empty data
      // with global_props_changed=true, inner UpdatePage will trigger children
      // render
      UpdatePage(data, update_page_option, pipeline_options);
    }
    return true;
  }

  if (should_render) {
    // needs set global props to data here
    data.SetProperty(kGlobalPropsKey_str, table);
    UpdatePage(data, update_page_option, pipeline_options);
  } else {
    UpdatePageData(kGlobalPropsKey_str.str(), table);
  }
  return true;
}

bool RadonPage::PrePageRender(const lepus::Value &data,
                              const UpdatePageOption &update_page_option) {
  return IsReact() ? PrePageRenderReact(data, update_page_option)
                   : PrePageRenderTT(data, update_page_option);
}

bool RadonPage::PrePageRenderReact(const lepus::Value &data,
                                   const UpdatePageOption &update_page_option) {
  switch (render_type_) {
    case RenderType::FirstRender: {
      lepus::Value merged_data = lepus::Value(lepus::Dictionary::Create());
      ForcePreprocessPageData(data, merged_data);
      if (page_proxy_->GetPrePaintingStage() ==
          PrePaintingStage::kPrePaintingOFF) {
        proxy_->OnReactCardRender(merged_data, true);
      }
      return true;
    }
    case RenderType::UpdateByNativeList:
    case RenderType::UpdateByNative: {
      lepus::Value merged_data = lepus::Value(lepus::Dictionary::Create());
      ForcePreprocessPageData(data, merged_data);

      // Add extra version fields when there could be conflicts for native
      // and JS to update data simultaneously. For top level pages this could
      // happen when updating data by native.
      AttachDataVersions(merged_data);

      bool should_component_update =
          ShouldComponentUpdate() || page_proxy_->GetPrePaintingStage() ==
                                         PrePaintingStage::kStartUpdatePage;

      proxy_->OnReactCardRender(merged_data, should_component_update);
      return should_component_update;
    }
    case RenderType::UpdateFromJSBySelf: {
      return true;
    }
    default:
      break;
  }
  return true;
}

bool RadonPage::PrePageRenderTT(const lepus::Value &data,
                                const UpdatePageOption &update_page_option) {
  if (render_type_ == RenderType::UpdateFromJSBySelf) {
    // update from js, no need to call `getDerivedStateFromProps`
    return ShouldComponentUpdate();
  }
  lepus_value new_data;
  if (get_derived_state_from_props_function_.IsCallable()) {
    new_data = PreprocessData();
    if (new_data.IsObject()) {
      UpdateTable(data_, new_data);
      LOGI("getDerivedStateFromProps for TTML Page ");
    }
  }

  // check shouldComponentUpdate
  return render_type_ == RenderType::FirstRender || ShouldComponentUpdate() ||
         page_proxy_->GetPrePaintingStage() ==
             PrePaintingStage::kStartUpdatePage;
}

bool RadonPage::ForcePreprocessPageData(const lepus::Value &updated_data,
                                        lepus::Value &merged_data) {
  bool need_update = false;
  if (updated_data.IsObject()) {
    merged_data = lepus_value::ShallowCopy(updated_data);
  }
  if (get_derived_state_from_props_function_.IsCallable()) {
    lepus_value new_data = PreprocessData();
    if (new_data.IsObject()) {
      ForEachLepusValue(
          new_data, [this, &merged_data, &need_update](
                        const lepus::Value &key, const lepus::Value &value) {
            auto key_str = key.String();
            if (UpdatePageData(key_str.str(), value)) {
              merged_data.SetProperty(key_str, value);
              need_update = true;
            }
          });
    }
  }

  return need_update;
}

bool RadonPage::UpdatePageData(const std::string &name,
                               const lepus::Value &value,
                               const bool update_top_var) {
  // issue:#3257 getDerivedStateFromProps lifecycle use the state of page。
  // can't get All the data from context's TopLevelVariable. so we also save
  // data in "data_"
  const bool should_keep_page_data = ShouldKeepPageData();

  // if already saved pageData. no need to update top_var.
  const bool should_update_top_var = !enable_save_page_data_ || update_top_var;

  if (!should_keep_page_data && !should_update_top_var) {
    return true;
  }

  // The key may be a path. And ParseValuePath is expensive, should only parse
  // once.
  auto path = lepus::ParseValuePath(name);

  if (should_keep_page_data) {
    lepus::Value::UpdateValueByPath(data_, value, path);
  }

  if (should_update_top_var) {
    return context_->UpdateTopLevelVariableByPath(path, value);
  }

  return true;
}

bool RadonPage::ResetPageData() {
  bool need_update = false;
  if (ShouldKeepPageData()) {
    // enableKeepPageData: true
    data_ = lepus::Value::Clone(init_data_);
    need_update = true;
    // lepus top level variables like __globalProps and SystemInfo may be
    // incorrectly changed by data processor
    UpdateLepusTopLevelVariableToData();
  } else {
    if (dsl_ == PackageInstanceDSL::REACT) {
      // EnablePageData in default true in later ReactLynx Versions.
      // In before Versions, global variables won't be cleared in ReactLynx.
      return false;
    }
    context_->ResetTopLevelVariable();
    ForEachLepusValue(init_data_, [this, &need_update](
                                      const lepus::Value &key,
                                      const lepus::Value &value) {
      need_update = context_->UpdateTopLevelVariable(key.StdString(), value);
    });
  }
  return need_update;
}

bool RadonPage::ShouldKeepPageData() {
  return enable_save_page_data_ ||
         get_derived_state_from_props_function_.IsCallable() ||
         should_component_update_function_.IsCallable() ||
         (page_proxy_ && page_proxy_->IsServerSideRendering());
}

void RadonPage::UpdateSystemInfo(const lepus::Value &info) {
  if (NeedsExtraData()) {
    UpdatePageData(kSystemInfo, info, true);
  } else {
    // if no needs for set SystemInfo to page's data
    // only update top level variable
    // but component may needs extra data, so it's required to iterate over all
    // components
    context_->UpdateTopLevelVariable(kSystemInfo, info);
  }

  auto it = proxy_->GetComponentMap().begin();
  for (; it != proxy_->GetComponentMap().end(); ++it) {
    it->second->UpdateSystemInfo(info);
  }
}

void RadonPage::Refresh(const DispatchOption &option,
                        PipelineOptions &pipeline_options) {
  this->attribute_holder()->Reset();
  auto original_radon_children = std::move(radon_children_);
  radon_children_.clear();
  lepus::Value p1(this);
  lepus::Value p2(false);
  std::string ss = "$renderPage" + std::to_string(this->node_index_);
  // Before lynx 2.1, $renderPage accept only the previous two params.
  context_->Call(ss, p1, p2, data_);
  PreHandlerCSSVariable();
  RadonMyersDiff(original_radon_children, option);
  // TODO(kechenglong): SetNeedsLayout if and only if needed.
  page_proxy_->element_manager()->SetNeedsLayout();
  page_proxy_->element_manager()->OnPatchFinish(pipeline_options);
}

void RadonPage::SetCSSVariables(const std::string &component_id,
                                const std::string &id_selector,
                                const lepus::Value &properties,
                                PipelineOptions &pipeline_options) {
  LOGI("start SetProperty from js id: " << component_id);
  if (component_id == PAGE_ID) {
    set_variable_ops_.emplace_back(SetCSSVariableOp(id_selector, properties));
    DispatchOption dispatch_option(proxy_);
    dispatch_option.css_variable_changed_ = true;
    Refresh(dispatch_option, pipeline_options);
  } else {
    int comp_id;
    if (lynx::base::StringToInt(component_id, &comp_id, 10)) {
      if (page_proxy_->CheckComponentExists(comp_id)) {
        auto *component = proxy_->GetComponentMap()[comp_id];
        if (component) {
          component->SetCSSVariables(id_selector, properties, pipeline_options);
        }
      } else {
        LOGE("js SetProperty with UnExisted Component!!");
      }
    }
  }
  LOGI("end SetProperty from js id: " << component_id);
}

CSSFragment *RadonPage::GetStyleSheetBase(AttributeHolder *holder) {
  if (!style_sheet_) {
    if (!intrinsic_style_sheet_ && style_sheet_manager_ && mould_) {
      intrinsic_style_sheet_ =
          style_sheet_manager_->GetCSSStyleSheetForPage(GetCSSId());
    }
    style_sheet_ =
        std::make_shared<CSSFragmentDecorator>(intrinsic_style_sheet_);
    if (intrinsic_style_sheet_ && style_sheet_ &&
        intrinsic_style_sheet_->HasTouchPseudoToken()) {
      style_sheet_->MarkHasTouchPseudoToken();
    }
    PrepareComponentExternalStyles(holder);
    PrepareRootCSSVariables(holder);
  }
  return style_sheet_.get();
}

bool RadonPage::UpdateConfig(const lepus::Value &config, bool to_refresh,
                             PipelineOptions &pipeline_options) {
  if (!context_) {
    return false;
  }

  uint64_t start_time = base::CurrentTimeMicroseconds();
  UpdateSystemInfo(GenerateSystemInfo(&config));

  if (!to_refresh) {
    return false;
  }
  page_proxy_->set_is_updating_config(true);
  DispatchOption dispatch_option{page_proxy_};
  this->attribute_holder()->Reset();
  // using radon diff
  auto original_radon_children = std::move(radon_children_);
  radon_children_.clear();
  lepus::Value p1(this);
  lepus::Value p2(false);
  std::string ss = "$renderPage" + std::to_string(this->node_index_);
  // Before lynx 2.1, $renderPage accept only the previous two params.
  context_->Call(ss, p1, p2, data_);
  PreHandlerCSSVariable();
  page_proxy_->set_is_updating_config(false);
  dispatch_option.force_diff_entire_tree_ = true;
  RadonMyersDiff(original_radon_children, dispatch_option);

  if (proxy_->EnableFeatureReport()) {
    ReportNoPatch(dispatch_option.has_patched_, start_time);
  }
  if (dispatch_option.has_patched_) {
    page_proxy_->element_manager()->SetNeedsLayout();
  };
  page_proxy_->element_manager()->OnPatchFinish(pipeline_options);
  return true;
}

void RadonPage::OnReactComponentDidUpdate(const DispatchOption &option) {
  if (IsReact() && !option.ignore_component_lifecycle_) {
    proxy_->OnReactCardDidUpdate();
  }
}

void RadonPage::TriggerComponentLifecycleUpdate(const std::string name) {
  if (page_proxy_ && page_proxy_->GetComponentLifecycleAlignWithWebview()) {
    for (RadonComponent *component : radon_component_dispatch_order_) {
      if (!proxy_->CheckComponentExists(component->ComponentId())) {
        LOGI(
            "component doesn't exist in "
            "RadonPage::TriggerComponentLifecycleUpdate");
        continue;
      }
      page_proxy_->FireComponentLifecycleEvent(name, component);
    }
  }
}

void RadonPage::ResetComponentDispatchOrder() {
  if (page_proxy_ && page_proxy_->GetComponentLifecycleAlignWithWebview()) {
    radon_component_dispatch_order_.clear();
  }
}

void RadonPage::CollectComponentDispatchOrder(RadonBase *radon_node) {
  if (page_proxy_ && page_proxy_->GetComponentLifecycleAlignWithWebview() &&
      radon_node->IsRadonComponent()) {
    RadonComponent *radon_component = static_cast<RadonComponent *>(radon_node);
    radon_component_dispatch_order_.push_back(radon_component);
  }
}

const std::string &RadonPage::GetEntryName() const { return entry_name_; }

lepus::Value RadonPage::OnScreenMetricsSet(const lepus::Value &input) {
  if (get_override_screen_metrics_function_.IsCallable()) {
    return context_->CallClosure(get_override_screen_metrics_function_, input);
  }
  return lepus::Value();
}

void RadonPage::SetScreenMetricsOverrider(const lepus::Value &overrider) {
  get_override_screen_metrics_function_ = overrider;
}

void RadonPage::Hydrate(PipelineOptions &pipeline_options) {
  if (!page_proxy_->HasSSRRadonPage()) {
    return;
  }

  DispatchOption dispatch_option{page_proxy_};
  dispatch_option.has_patched_ = true;
  dispatch_option.ssr_hydrating_ = true;
  dispatch_option.need_update_element_ = true;
  dispatch_option.need_diff_ = !page_proxy_->HydrateDataIdenticalAsSSR();
  PreHandlerCSSVariable();
  page_proxy_->element_manager()
      ->painting_context()
      ->MarkUIOperationQueueFlushTiming(
          tasm::timing::kPaintingUiOperationExecuteStart,
          pipeline_options.pipeline_id);

  auto old_radon_children = std::move(page_proxy_->SSRPage()->radon_children_);
  DispatchSelf(dispatch_option);
  RadonMyersDiff(old_radon_children, dispatch_option);
  if (dispatch_option.has_patched_) {
    page_proxy_->element_manager()->SetNeedsLayout();
  };

  auto *root_element = page_proxy_->Page()->element();
  // Destory ssr page after hydrate.
  page_proxy_->ResetSSRPage();
  page_proxy_->element_manager()->SetRoot(root_element);

  page_proxy_->element_manager()->OnPatchFinish(pipeline_options);
}

void RadonPage::triggerNewLifecycle(const DispatchOption &option) {
  page_proxy_->SetPrePaintingStage(PrePaintingStage::kPrePaintingOFF);
  RadonBase::triggerNewLifecycle(option);
  OnReactComponentDidUpdate(option);
}

}  // namespace tasm
}  // namespace lynx
