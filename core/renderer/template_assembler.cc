// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/template_assembler.h"

#include <chrono>
#include <exception>
#include <limits>
#include <numeric>
#include <stack>
#include <unordered_set>
#include <vector>

#include "base/include/fml/make_copyable.h"
#include "base/include/log/logging.h"
#include "base/include/string/string_number_convert.h"
#include "base/trace/native/trace_event.h"
#include "core/base/json/json_util.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/css/css_fragment.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/data/lynx_view_data_manager.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/dom/vdom/radon/base_component.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/radon_dispatch_option.h"
#include "core/renderer/dom/vdom/radon/radon_lazy_component.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/renderer/utils/base/base_def.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/base/tasm_utils.h"
#include "core/renderer/utils/value_utils.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/bindings/jsi/api_call_back.h"
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
#include "core/runtime/bindings/lepus/renderer.h"
#include "core/runtime/common/js_error_reporter.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/binary_input_stream.h"
#include "core/runtime/vm/lepus/builtin.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/tasks/lepus_callback_manager.h"
#include "core/runtime/vm/lepus/tasks/lepus_raf_manager.h"
#include "core/runtime/vm/lepus/vm_context.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/recorder/recorder_controller.h"
#include "core/services/ssr/client/ssr_client_utils.h"
#include "core/services/ssr/client/ssr_data_update_manager.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"
#include "core/shared_data/white_board_tasm_delegate.h"
#include "core/template_bundle/template_codec/binary_decoder/template_binary_reader.h"
#include "core/value_wrapper/value_impl_lepus.h"

#if ENABLE_AIR
#include "core/renderer/dom/air/air_element/air_element.h"
#include "core/renderer/dom/air/air_element/air_page_element.h"
#include "core/renderer/dom/air/air_touch_event_handler.h"
#endif

#if ENABLE_LEPUSNG_WORKLET
#include "core/renderer/worklet/lepus_element.h"
#endif  // ENABLE_LEPUSNG_WORKLET

namespace lynx {
namespace tasm {

namespace {

constexpr char kNetworkSuggestion[] =
    "Please refer to the error message, or seek help from resource loader.";
constexpr const static char* kTemplateJSSizeOfGenericInfo = "template_js_size";

std::string ConstructDecodeErrorMessage(bool is_card, const std::string& url,
                                        const std::string& error_msg) {
  constexpr char kDecodeError[] = "Decode error: ";
  std::string msg = kDecodeError + error_msg;
  if (!is_card) {
    constexpr char kDecodeErrorJoiner[] = ", type: lazy bundle, url: ";
    msg.append(kDecodeErrorJoiner + url);
  }
  return msg;
}

base::LynxError ConstructLynxErrorForLazyBundle(const std::string& url,
                                                int error_code,
                                                const std::string& error_msg,
                                                const std::string& suggestion,
                                                bool is_preload = false) {
  base::LynxError error{error_code, error_msg, suggestion,
                        base::LynxErrorLevel::Error};
  common::FormatErrorUrl(error, url);
  error.AddContextInfo("preload", std::to_string(is_preload));
  return error;
}

class ComponentUpdateReporter {
 public:
  ComponentUpdateReporter(const runtime::UpdateDataTask& task,
                          const PageProxy& proxy)
      : type_(task.type_) {
    if (!proxy.EnableFeatureReport()) {
      return;
    }
    if (type_.IsUpdateExplictByUser()) {
      return;
    }
    start_time_ = base::CurrentTimeMicroseconds();
    if (task.is_card_) {
      component_name_ = "RootComponent";
    } else {
      int i_id = atoi(task.component_id_.c_str());
      const auto& map = proxy.GetComponentMap();
      auto iter = map.find(i_id);
      if (iter != map.end()) {
        // original string maybe released in this destructor, so we copy here.
        component_name_ = iter->second->path().str();
      }
    }
  }

  ~ComponentUpdateReporter() {
    if (start_time_ == 0) {
      return;
    }
    if (type_.IsUpdateExplictByUser()) {
      return;
    }
    auto duration =
        static_cast<int>(base::CurrentTimeMicroseconds() - start_time_);
    tasm::report::EventTracker::OnEvent(
        [duration, component_name = std::move(component_name_),
         type = type_](tasm::report::MoveOnlyEvent& event) {
          event.SetName("lynxsdk_component_update");
          event.SetProps("component_name", component_name);
          event.SetProps("update_data_type", static_cast<std::uint32_t>(type));
          event.SetProps("duration_microsecond", duration);
        });
  }

 private:
  uint64_t start_time_ = 0;
  const runtime::UpdateDataType type_;
  std::string component_name_;
};

}  // namespace
const std::unordered_map<int, std::shared_ptr<PageMould>>&
TemplateAssembler::page_moulds() {
  return FindEntry(DEFAULT_ENTRY_NAME)->page_moulds();
}

thread_local TemplateAssembler* TemplateAssembler::curr_ = nullptr;

static constexpr const char k_actual_first_screen[] = "__isActualFirstScreen";

TemplateAssembler::Scope::Scope(TemplateAssembler* tasm) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateAssembler::Scope::Scope");
  if (tasm != nullptr && curr_ == nullptr) {
    curr_ = tasm;
    scoped_ = true;
    base::ErrorStorage::GetInstance().Reset();
  }
}

TemplateAssembler::Scope::~Scope() {
  if (scoped_) {
    auto& error = base::ErrorStorage::GetInstance().GetError();
    if (error != nullptr) {
      // TODO(yuanzhiwen): handle LynxErrorLevel::Fatal in platform.
      if (error->should_abort_) {
        if (LynxEnv::GetInstance().IsDevToolComponentAttach() &&
            !LynxEnv::GetInstance().IsLogBoxEnabled()) {
          LOGF("error_message: " << error->error_message_ << " fix_suggestion: "
                                 << error->error_message_);
          return;
        } else {
          error->error_level_ = base::LynxErrorLevel::Error;
        }
      }
      curr_->ReportError(std::move(*error));
    }
    curr_ = nullptr;
    base::ErrorStorage::GetInstance().Reset();
  }
}

TemplateAssembler::TemplateAssembler(Delegate& delegate,
                                     std::unique_ptr<ElementManager> client,
                                     int32_t instance_id)
    : page_proxy_(this, std::move(client), &delegate),
      support_component_js_(false),
      target_sdk_version_("null"),
      template_loaded_(false),
      actual_fmp_start_(0),
      actual_fmp_end_(0),
      delegate_(delegate),
      touch_event_handler_(nullptr),
#if ENABLE_AIR
      air_touch_event_handler_(nullptr),
#endif
      has_load_page_(false),
      page_config_(nullptr),
      instance_id_(instance_id),
      destroyed_(false),
      is_loading_template_(false),
      font_scale_(1.0),
      component_loader_(nullptr) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateAssembler::TemplateAssembler");
  auto card = std::make_shared<TemplateEntry>();
  InsertEntry(DEFAULT_ENTRY_NAME, std::move(card));
}

TemplateAssembler::~TemplateAssembler() {
  LOGI("TemplateAssembler::Release url:" << url_ << " this:" << this);
};

void TemplateAssembler::Init(fml::RefPtr<fml::TaskRunner> tasm_runner) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateAssembler::Init");
  std::weak_ptr<TemplateAssembler> wp(shared_from_this());
}

void TemplateAssembler::UpdateGlobalProps(const lepus::Value& data,
                                          bool need_render,
                                          PipelineOptions& pipeline_options) {
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::TemplateAssemblerRecorder::RecordSetGlobalProps(data,
                                                                  record_id_);
#endif
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxUpdateGlobalProps", "need_render",
              need_render);
  global_props_ = data;
  if (template_loaded_) {
    NotifyGlobalPropsChanged(data);
    UpdateGlobalPropsToContext(data);
  }

  if (EnableFiberArch()) {
    if (!template_loaded_) {
      return;
    }
    auto& context = FindEntry(DEFAULT_ENTRY_NAME)->GetVm();
    if (context == nullptr) {
      LOGE("TemplateAssembler::UpdateGlobalProps since the context is null!!");
      return;
    }
    context->Call(BASE_STATIC_STRING(kUpdateGlobalProps), data);
  } else {
    // Update `__globalProps` for LazyBundle, used only in Radon.
    ForEachEntry([card_entry = this->FindEntry(DEFAULT_ENTRY_NAME).get(),
                  &global_props = this->global_props_](const auto& entry) {
      if (entry.get() != card_entry) {
        entry->GetVm()->UpdateTopLevelVariable(kGlobalPropsKey, global_props);
      }
    });

    need_render =
        need_render && template_loaded_ && !page_proxy_.IsServerSideRendering();
    page_proxy_.UpdateGlobalProps(global_props_, need_render, pipeline_options);
  }
}

void TemplateAssembler::SetLepusObserver(
    const std::shared_ptr<lepus::InspectorLepusObserver>& observer) {
  lepus_observer_ = observer;
  ForEachEntry(
      [&observer](const auto& entry) { entry->SetLepusObserver(observer); });
}

void TemplateAssembler::UpdateGlobalPropsWithDefaultProps(
    PipelineOptions& pipeline_options) {
  if (page_proxy_.HasSSRRadonPage() &&
      !page_proxy_.GetDefaultGlobalProps().IsEmpty()) {
    if (global_props_.IsNil()) {
      UpdateGlobalProps(page_proxy_.GetDefaultGlobalProps(), true,
                        pipeline_options);
    } else {
      for (const auto& [key, value] :
           *(page_proxy_.GetDefaultGlobalProps().Table())) {
        if (global_props_.GetProperty(key).IsEmpty()) {
          global_props_.SetProperty(key, value);
        }
      }
      UpdateGlobalProps(global_props_, true, pipeline_options);
    }
  }
}

void TemplateAssembler::UpdateGlobalPropsToContext(const lepus::Value& props) {
  auto kGlobalPropsKey_str = BASE_STATIC_STRING(kGlobalPropsKey);
  auto kSystemInfo_str = BASE_STATIC_STRING(kSystemInfo);
  static constexpr const char kPostDataBeforeUpdate[] = "postDataBeforeUpdate";
  auto kPostDataBeforeUpdateLepusStr =
      BASE_STATIC_STRING(kPostDataBeforeUpdate);
  static constexpr const char kTriggerReadyWhenReload[] =
      "triggerReadyWhenReload";
  auto kTriggerReadyWhenReloadStr = BASE_STATIC_STRING(kTriggerReadyWhenReload);

  ForEachEntry([&kGlobalPropsKey_str, &kSystemInfo_str,
                &kTriggerReadyWhenReloadStr, &kPostDataBeforeUpdateLepusStr,
                &props](const auto& entry) {
    auto context = entry->GetVm();
    if (context == nullptr) {
      return;
    }
    context->SetPropertyToLynx(kGlobalPropsKey_str, props);
    context->SetPropertyToLynx(kSystemInfo_str,
                               tasm::GenerateSystemInfo(nullptr));
    context->SetPropertyToLynx(kTriggerReadyWhenReloadStr, lepus::Value(true));
    if (LynxEnv::GetInstance().EnablePostDataBeforeUpdateTemplate()) {
      context->SetPropertyToLynx(kPostDataBeforeUpdateLepusStr,
                                 lepus::Value(true));
    }
  });
}

bool TemplateAssembler::OnLoadTemplate(PipelineOptions& pipeline_options) {
  // timing actions
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadBundleStart);

  // print log
  LOGI("start TemplateAssembler::LoadTemplate, url:"
       << url_ << " len: " << source_size_ << " this:" << this);

  // record source size
  report::EventTracker::UpdateGenericInfo(
      instance_id_, kTemplateJSSizeOfGenericInfo, std::to_string(source_size_));

  // check if is_loading_template_ == true, report error
  if (is_loading_template_) {
    ReportError(base::LynxError(error::E_APP_BUNDLE_LOAD_RENDER_FAILED,
                                "LoadTemplate in another loading process!!!"));
    return false;
  }
  is_loading_template_ = true;

  // Before template load start, update global props
  UpdateGlobalPropsWithDefaultProps(pipeline_options);

  actual_fmp_start_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
  return true;
}

void TemplateAssembler::OnDecodeTemplate() {
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kParseStart);
}

void TemplateAssembler::DidDecodeTemplate(
    const std::shared_ptr<TemplateData>& template_data,
    const std::shared_ptr<TemplateEntry>& entry, bool post_js,
    const PipelineOptions& pipeline_options) {
  if (component_loader_) {
    component_loader_->SetEnableLynxResourceService(
        page_config_->GetEnableLynxResourceServiceProvider());
    component_loader_->SetEnableComponentAsyncDecode(
        page_config_->GetEnableComponentAsyncDecode());
  }
  SetEnableQueryComponentSync(page_config_->GetEnableQueryComponentSync());

  page_proxy()->element_manager()->SetEnableUIOperationOptimize(
      page_config_->GetEnableUIOperationOptimize());

  page_proxy()->element_manager()->SetEnableFiberElementForRadonDiff(
      page_config_->GetEnableFiberElementForRadonDiff());

  if (page_config_) {
    bool enableParallelElementConfig =
        (page_config_->GetPipelineSchedulerConfig() &
         kEnableParallelElementMask) > 0;
    if ((enableParallelElementConfig ||
         page_config_->GetEnableParallelElement()) &&
        page_proxy()
            ->element_manager()
            ->painting_context()
            ->impl()
            ->EnableParallelElement()) {
      page_proxy()->element_manager()->SetEnableParallelElement(true);
      bool enable_report_statistic =
          lynx::tasm::LynxEnv::GetInstance().GetBoolEnv(
              lynx::tasm::LynxEnv::Key::
                  ENABLE_REPORT_THREADED_ELEMENT_FLUSH_STATISTIC,
              false);
      page_proxy()
          ->element_manager()
          ->SetEnableReportThreadedElementFlushStatistic(
              enable_report_statistic);
    }
  }

  // Ensure that only one page config is set.
  if (!page_proxy_.HasSSRRadonPage()) {
    SetPageConfigClient();
  }

  if (post_js) {
    if (entry && EnableDataProcessorOnJs()) {
      // If EnableDataProcessorOnJs is enabled, JS can be loaded after decoding
      // the template. And, before doing this, the int template data and cache
      // template data should be set in advance for the default entry, making it
      // convenient for JS to consume the corresponding data.
      entry->SetInitData(TemplateData::CopyPlatformData(template_data));
      std::vector<TemplateData> cache_data;
      for (const auto& single_cache_data : cache_data_) {
        cache_data.emplace_back(
            TemplateData::CopyPlatformData(single_cache_data));
      }
      entry->SetCacheData(std::move(cache_data));
    }

    // Pending post js when pre_painting_ is enabled
    if (!pre_painting_) {
      OnJSPrepared(url_, pipeline_options);
    }
  }

  // if using leousNG, set gc threshold.
  if (page_config_->GetEnableLepusNG()) {
    FindEntry(tasm::DEFAULT_ENTRY_NAME)
        ->GetVm()
        ->SetGCThreshold(page_config_->GetLepusGCThreshold());
  }
  auto card = FindEntry(tasm::DEFAULT_ENTRY_NAME);
  if (card && card->GetVm()) {
    card->GetVm()->UpdateGCTiming(true);
  }

  // timing actions
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kParseEnd);
}

void TemplateAssembler::OnVMExecute() {
  // register global props and SystemInfo to context.
  UpdateGlobalPropsToContext(global_props_);

  // timing actions
  tasm::TimingCollector::Instance()->MarkFrameworkTiming(
      tasm::timing::kVmExecuteStart);
}

void TemplateAssembler::DidVMExecute() {
  // timing actions
  tasm::TimingCollector::Instance()->MarkFrameworkTiming(
      tasm::timing::kVmExecuteEnd);

  // Radon info can be know only after Vm->Execute()
  SetPageConfigRadonMode();

  // Ensure that only one page config is set
  if (!page_proxy_.HasSSRRadonPage()) {
    OnPageConfigDecoded(page_config_);
  }
}

/**
 * The purpose of this function is to merge cache_data into init data when
 * loadTemplate. There are two requirements:
 * 1. The merge must be done in the order of the input data.
 * 2. Different template_data may have different processors and must be handled
 * separately, but submitting to Lepus is a time-consuming process.
 * Therefore, the final decision is: Merge adjacent template_data that hold the
 * same processor, and then submit it to Lepus in order.
 */
TemplateData TemplateAssembler::ProcessInitData(
    const std::shared_ptr<TemplateData>& init_template_data) {
  // early return if there is no cache data
  if (EnableDataProcessorOnJs() || cache_data_.empty()) {
    return ProcessTemplateData(init_template_data, true);
  }

  bool handle_template_data = false;
  TemplateData result{lepus::Value::CreateObject(), true};

  /**
   * data with the same processor will be merged firstly, and then processed by
   * lepus. Return whether input template_data could be early merged.
   */
  auto early_merge = [&result](auto& dict, const auto& template_data,
                               const std::string& processor_name) -> bool {
    if (!template_data) {
      return true;
    }
    if (template_data->PreprocessorName() == processor_name) {
      result.SetReadOnly(result.IsReadOnly() && template_data->IsReadOnly());
      lepus::Value::MergeValue(dict, template_data->GetValue());
      return true;
    }
    return false;
  };

  /**
   * data with the different processors will be processed firstly, and then
   * merged with init_data
   */
  auto process_and_merge = [&result, this](const auto& templated_data,
                                           bool is_first_screen) {
    auto data = this->ProcessTemplateData(templated_data, is_first_screen);
    lepus::Value::MergeValue(result.value(), data.GetValue());
  };

  /**
   * adjacent data with the same processor can be put together and handed over
   * to lepus for processing, which can save a lot of time
   */
  for (auto cache = cache_data_.begin(); cache != cache_data_.end();) {
    const std::string& processor_name = (*cache)->PreprocessorName();
    auto dict = lepus::Value(lepus::Dictionary::Create());

    // merge adjacent data with the same processor
    while (cache != cache_data_.end() &&
           early_merge(dict, *cache, processor_name)) {
      ++cache;
    }

    // if the iteration is complete, try to merge with init template_data
    if (cache == cache_data_.end()) {
      handle_template_data =
          early_merge(dict, init_template_data, processor_name);
    }

    // submit to lepus engine
    process_and_merge(
        std::make_shared<TemplateData>(dict, false, processor_name),
        handle_template_data);
  }

  // if init template_data has not been processed, that is, template_data has a
  // different processor from the previous data, it is processed separately
  if (!handle_template_data) {
    result.SetReadOnly(result.IsReadOnly() && init_template_data->IsReadOnly());
    process_and_merge(init_template_data, true);
  }

  return result;
}

TemplateData TemplateAssembler::OnRenderTemplate(
    const std::shared_ptr<TemplateData>& template_data,
    const std::shared_ptr<TemplateEntry>& card, bool post_js,
    PipelineOptions& pipeline_options) {
  // If global_props_ not nil, update global_props_ to page_proxy_.
  if (!global_props_.IsNil()) {
    page_proxy_.UpdateGlobalProps(global_props_, false, pipeline_options);
  }

  // Get init data. If init data not empty, set init data to template entry.
  TemplateData data = ProcessInitData(template_data);

  // If data is not empty, set data to card's init data.
  // If data is empty, let data be dict to call
  // page_proxy_.UpdateInLoadTemplate.
  tasm::TimingCollector::Instance()->MarkFrameworkTiming(
      tasm::timing::kSetInitDataStart);
  if (!data.GetValue().IsEmpty()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "TemplateEntry::SetInitData");
    if (ShouldPostDataToJs() && !EnableDataProcessorOnJs()) {
      card->SetInitData(GenerateTemplateDataPostedToJs(data));
    }
  } else {
    data.SetValue(lepus::Value::CreateObject());
  }
  tasm::TimingCollector::Instance()->MarkFrameworkTiming(
      tasm::timing::kSetInitDataEnd);

  // Before render element, execute screen metrics override.
  auto& client = page_proxy_.element_manager();
  if (client != nullptr) {
    OnScreenMetricsSet(client->GetLynxEnvConfig().ScreenWidth(),
                       client->GetLynxEnvConfig().ScreenHeight());
  }

  // If need post js, call OnJSPrepared.
  // Pending post js when pre_painting_ is enabled
  if (post_js && !pre_painting_) {
    OnJSPrepared(url_, pipeline_options);
  }

  return data;
}

void TemplateAssembler::RenderTemplate(
    const std::shared_ptr<TemplateEntry>& card, const TemplateData& data,
    PipelineOptions& pipeline_options) {
  if (EnableFiberArch()) {
    RenderTemplateForFiber(card, data, pipeline_options);
  } else if (EnableLynxAir()) {
    RenderTemplateForAir(card, data.GetValue(), pipeline_options);
  } else {
    UpdatePageOption update_page_option;
    update_page_option.update_first_time = true;
    page_proxy_.UpdateInLoadTemplate(data.GetValue(), update_page_option,
                                     pipeline_options);
  }
}

void TemplateAssembler::UpdateTemplate(
    const TemplateData& data, const UpdatePageOption& update_page_option,
    PipelineOptions& pipeline_options) {
  if (EnableFiberArch()) {
    if (update_page_option.reload_template ||
        pipeline_options.need_timestamps) {
      tasm::TimingCollector::Instance()->Mark(tasm::timing::kMtsRenderStart);
    }

    auto options = update_page_option.ToLepusValue();
    options.SetProperty(BASE_STATIC_STRING(kPipelineOptions),
                        PipelineOptionsToLepusValue(pipeline_options));
    if (pre_painting_) {
      options.SetProperty(BASE_STATIC_STRING(kTriggerLifeCycle),
                          lepus::Value(true));
    }
    if (EnableDataProcessorOnJs()) {
      options.SetProperty(BASE_STATIC_STRING(kProcessorName),
                          lepus::Value(data.PreprocessorName()));
    }

    FindEntry(tasm::DEFAULT_ENTRY_NAME)
        ->GetVm()
        ->Call(BASE_STATIC_STRING(kUpdatePage), data.GetValue(),
               std::move(options));

    if (!update_page_option.reload_template &&
        pipeline_options.need_timestamps) {
      tasm::TimingCollector::Instance()->Mark(tasm::timing::kMtsRenderEnd);
    }

  } else {
    if (UpdateGlobalDataInternal(data.GetValue(), update_page_option,
                                 pipeline_options)) {
      // Currently, only client updateData, client resetData, and JS root
      // component setData updates trigger the OnDataUpdated callback.
      if ((update_page_option.from_native &&
           !update_page_option.reload_template &&
           !update_page_option.reload_from_js) ||
          (update_page_option.from_native &&
           update_page_option.reset_page_data)) {
        delegate_.OnDataUpdated();
      }
    }
  }
}

void TemplateAssembler::RenderTemplateForFiber(
    const std::shared_ptr<TemplateEntry>& card, const TemplateData& data,
    PipelineOptions& pipeline_options) {
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kMtsRenderStart);

  lepus::Value render_options = lepus::Value::CreateObject();
  if (EnableDataProcessorOnJs()) {
    auto kProcessorName_str = BASE_STATIC_STRING(kProcessorName);
    render_options.SetProperty(kProcessorName_str,
                               lepus::Value(data.PreprocessorName()));
    if (!cache_data_.empty()) {
      auto kData_str = BASE_STATIC_STRING(kData);
      auto kCacheData_str = BASE_STATIC_STRING(kCacheData);
      auto cache_data = lepus::CArray::Create();
      for (const auto& data : cache_data_) {
        lepus::Value data_obj = lepus::Value::CreateObject();
        data_obj.SetProperty(kData_str, data->GetValue());
        data_obj.SetProperty(kProcessorName_str,
                             lepus::Value(data->PreprocessorName()));
        cache_data->emplace_back(std::move(data_obj));
      }
      render_options.SetProperty(kCacheData_str,
                                 lepus::Value(std::move(cache_data)));
    }
  }
  render_options.SetProperty(BASE_STATIC_STRING(kPreLoadTemplate),
                             lepus::Value(pre_painting_));
  render_options.SetProperty(BASE_STATIC_STRING(kPipelineOptions),
                             PipelineOptionsToLepusValue(pipeline_options));

  fml::RefPtr<FiberElement> element_cache = card->TryToGetElementCache();
  if (element_cache.get()) {
    TreeResolver::AttachRootToElementManager(
        element_cache, page_proxy()->element_manager().get(),
        style_sheet_manager(DEFAULT_ENTRY_NAME), true);
    render_options.SetProperty(BASE_STATIC_STRING(kInitPage),
                               lepus::Value(element_cache));
  }

  // No need to re-render nodes during SSR
  if (!page_proxy_.IsWaitingSSRHydrate()) {
    card->GetVm()->Call(BASE_STATIC_STRING(kRenderPage), data.GetValue(),
                        std::move(render_options));
  } else {
    // When Hydrating SSR page, the extreme_parsed_style flag has to be cleared
    // to make element do full CSS resolving when the classes is updated after
    // hydrated.
    page_proxy()->element_manager()->ClearExtremeParsedStyles();
  }

  tasm::TimingCollector::Instance()->Mark(tasm::timing::kMtsRenderEnd);

  pipeline_options.is_first_screen = true;
  page_proxy()->element_manager()->OnPatchFinish(pipeline_options);

  if (page_proxy()->element_manager()->GetEnableDumpElementTree()) {
    DumpElementTree(card);
  }
}

void TemplateAssembler::RenderTemplateForAir(
    const std::shared_ptr<TemplateEntry>& card, const lepus::Value& data,
    PipelineOptions& pipeline_options) {
#if ENABLE_AIR
  auto* page = page_proxy()->element_manager()->AirRoot();
  if (!page) {
    // AirRoot is nullptr means an error occurs during VM execution, no further
    // steps are needed.
    return;
  }
  const auto page_ptr = AirLepusRef::Create(
      page_proxy()->element_manager()->air_node_manager()->Get(
          page->impl_id()));
  UpdatePageOption update_options;
  update_options.update_first_time = true;
  page_proxy()->element_manager()->AirRoot()->UpdatePageData(
      data, update_options, pipeline_options);
  page_proxy()
      ->element_manager()
      ->painting_context()
      ->MarkUIOperationQueueFlushTiming(
          tasm::timing::kPaintingUiOperationExecuteStart,
          pipeline_options.pipeline_id);

  tasm::TimingCollector::Instance()->Mark(tasm::timing::kRenderPageStartAir);

  if (card->compile_options().radon_mode_ ==
      CompileOptionRadonMode::RADON_MODE_RADON) {
    lepus::Value p1(std::move(page_ptr));
    BASE_STATIC_STRING_DECL(kCreatePage0, "$createPage0");
    BASE_STATIC_STRING_DECL(kUpdatePage0, "$updatePage0");
    card->GetVm()->Call(kCreatePage0, p1);
    card->GetVm()->Call(kUpdatePage0, p1);
  } else {
    BASE_STATIC_STRING_DECL(kRenderPage0, "$renderPage0");
    lepus::Value ret = card->GetVm()->Call(
        kRenderPage0, lepus::Value(std::move(page_ptr)), lepus::Value(true),
        lepus::Value(page_proxy()->element_manager()->AirRoot()->GetData()));
    // In some cases, some element may fail to execute the flush operation due
    // to exceptions in the execution of lepus code. As a result, layout and
    // other operations are not necessary.
    bool lepus_success = ret.IsBool() && ret.Bool();
    if (!lepus_success) {
      return;
    }
  }
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kRenderPageEndAir);

  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "OnPatchFinishInnerForAir");
  page_proxy()->element_manager()->AirRoot()->SetFontFaces();
  pipeline_options.is_first_screen = true;
  EnsureAirTouchEventHandler();
  page_proxy()->element_manager()->OnPatchFinishInnerForAir(pipeline_options);
#endif
}

void TemplateAssembler::DidRenderTemplate(PipelineOptions& pipeline_options) {
  // ssr actions
  if (page_proxy_.IsWaitingSSRHydrate()) {
    LOGI("start to try hydrate SSR page, this:" << this << ", url" << url_);
    page_proxy_.HydrateOnFirstScreenIfPossible(this, pipeline_options);
    delegate_.OnSSRHydrateFinished(url_);
    LOGI("end to try hydrate SSR page, this:" << this << ", url" << url_);
  }

  // reset flag
  template_loaded_ = true;
}

void TemplateAssembler::DidLoadTemplate() {
  // exec callback
  // Pending post js when pre_painting_ is enabled
  if (!pre_painting_) {
    OnNativeAppReady();
  }
  delegate_.OnTemplateLoaded(url_);
  SendFontScaleChanged(font_scale_);
  delegate_.OnTasmFinishByNative();

  // reset flag
  is_loading_template_ = false;

  // print log
  LOGI("end TemplateAssembler::LoadTemplate, url:" << url_
                                                   << " len: " << source_size_);

  // timing actions
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadBundleEnd);
  auto card = FindEntry(DEFAULT_ENTRY_NAME);
  if (card && card->GetVm()) {
    card->GetVm()->UpdateGCTiming(false);
  }
}

void TemplateAssembler::LoadTemplateBundle(
    const std::string& url, LynxTemplateBundle template_bundle,
    const std::shared_ptr<TemplateData>& template_data,
    PipelineOptions& pipeline_options, const bool enable_pre_painting,
    bool enable_dump_element_tree) {
  // TODO (nihao.royal) add testbench for LoadTemplateBundle.
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::TemplateAssemblerRecorder::RecordLoadTemplateBundle(
      url, template_bundle.GetBinary(), template_data, record_id_);
  auto& client = page_proxy_.element_manager();
  if (client != nullptr) {
    client->SetRecordId(record_id_);
  }
#endif
  pre_painting_ = enable_pre_painting;
  if (pre_painting_) {
    page_proxy_.SetPrePaintingStage(PrePaintingStage::kStartPrePainting);
  }

  source_size_ = template_bundle.total_size_;
  url_ = url;

  if (page_proxy_.element_manager()) {
    page_proxy_.element_manager()->SetEnableDumpElementTree(
        enable_dump_element_tree);
  }
  TimingCollector::Instance()->Mark(tasm::timing::kTemplateBundleParseStart,
                                    template_bundle.decode_start_timestamp_);
  TimingCollector::Instance()->Mark(tasm::timing::kTemplateBundleParseEnd,
                                    template_bundle.decode_end_timestamp_);
  LoadTemplateInternal(
      url, template_data, pipeline_options,
      [this, template_bundle = std::move(template_bundle)](
          const std::shared_ptr<TemplateEntry>& card_entry) mutable {
        return card_entry->InitWithTemplateBundle(this,
                                                  std::move(template_bundle));
      });
  ClearCacheData();
}

void TemplateAssembler::LoadTemplate(
    const std::string& url, std::vector<uint8_t> source,
    const std::shared_ptr<TemplateData>& template_data,
    PipelineOptions& pipeline_options, const bool enable_pre_painting,
    bool enable_recycle_template_bundle) {
#if ENABLE_TESTBENCH_RECORDER
  // test-bench actions
  tasm::recorder::TemplateAssemblerRecorder::RecordLoadTemplate(
      url, source, template_data, record_id_);
  auto& client = page_proxy_.element_manager();
  if (client != nullptr) {
    client->SetRecordId(record_id_);
  }
#endif
  source_size_ = source.size();
  url_ = url;
  pre_painting_ = enable_pre_painting;
  if (pre_painting_) {
    page_proxy_.SetPrePaintingStage(PrePaintingStage::kStartPrePainting);
  }
  LoadTemplateInternal(
      url, template_data, pipeline_options,
      [this, source = std::move(source), enable_recycle_template_bundle](
          const std::shared_ptr<TemplateEntry>& card_entry) mutable {
        if (!FromBinary(card_entry, std::move(source))) {
          return false;
        }

        if (enable_recycle_template_bundle) {
          delegate_.RecycleTemplateBundle(
              card_entry->GetTemplateBundleRecycler());
        }

        return true;
      });
  ClearCacheData();
}

// LoadTemplate function will execute the following functions in sequence
// 1. OnLoadTemplate
// 2. OnDecodeTemplate
// 3. Decode
// 4. DidDecodeTemplate
// 5. OnVMExecute
// 6. VMExecute
// 7. DidVMExecute
// 8. OnRenderTemplate
// 9. RenderTemplate
// 10. DidRenderTemplate
// 11. DidLoadTemplate
void TemplateAssembler::LoadTemplateInternal(
    const std::string& url, const std::shared_ptr<TemplateData>& template_data,
    PipelineOptions& pipeline_options,
    base::MoveOnlyClosure<bool, const std::shared_ptr<TemplateEntry>&>
        entry_initializer) {
  // Trace LoadTemplate
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY_VITALS, "LynxLoadTemplate",
      [&url, instance_id = instance_id_](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("url", url);
        ctx.event()->add_debug_annotations("instance_id",
                                           std::to_string(instance_id));
      });

#if ENABLE_TRACE_SYSTRACE
  // This trace event is used for developer to get url info using systrace which
  // doesn't support args
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS,
              "InstanceId::" + std::to_string(instance_id_) + ", url::" + url);
#endif

  Scope scope(this);

  // Before exec load template, do some preparation
  // 1. exec timing actions
  // 2. exec test-bench actions
  // 3. print log
  // 4. update global props
  if (!OnLoadTemplate(pipeline_options)) {
    LOGE("OnLoadTemplate check failed");
    return;
  }

  // Get page template entry
  auto card = FindEntry(DEFAULT_ENTRY_NAME);

  // In radon/radon-diff mode, if template_data == nullptr &&
  // global_props_.IsNil() && page_proxy_.GetDefaultPageData().IsEmpty(), the
  // data processor will not be executed. Thus, in this case, Card's init data
  // must be nil. JS source can be posted to JS before vm's execution.
  // Be aware that the template may not have been decoded yet, so the correct
  // page config value may not be available.
  bool js_posted_before_vm = template_data == nullptr &&
                             global_props_.IsNil() &&
                             page_proxy_.GetDefaultPageData().IsEmpty();
  {
    // Trace Decode
    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "LynxDecode");

    // Before exec decode template, do some preparation. Only timing actions
    // now.
    OnDecodeTemplate();

    if (!entry_initializer(card)) {
      LOGE("Decoding template failed");
      return;
    }

    // After decode template, exec some aftercare
    // 1. ssr actions
    // 2. if need js_posted_before_vm, post js(if in air strict mode, js runtime
    // is not enabled, no need to post js)
    // 3. timing actions
    // If EnableDataProcessorOnJs(), the JS source can be sent to the JS thread
    // here.
    DidDecodeTemplate(template_data, card,
                      (EnableDataProcessorOnJs() || js_posted_before_vm) &&
                          ShouldPostDataToJs(),
                      pipeline_options);
  }

  {
    // Trace VM Execute
    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "VMExecute");

    // Before vm execute template, do some preparation. Only timing actions now.
    OnVMExecute();

    // Get VM & exec VM.
    if (!card->GetVm()->Execute()) {
      base::LynxError error{error::E_APP_BUNDLE_LOAD_RENDER_FAILED,
                            "vm execute failed"};
      ReportError(std::move(error));
      return;
    }

    // After VM Execute, exec some aftercare
    // 1. timing actions
    // 2. set radon info
    // 3. ssr actions
    DidVMExecute();
  }

  {
    // Trace DOM ready
    TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "LynxDomReady");

    // Before render template, do some preparation.
    // 1. update global props if needed
    // 2. get init data to render template
    // 3. execute screen metrics override
    // 4. post js if needed(if in air strict mode, js rntime is not enabled, no
    // need to post js)
    // If EnableDataProcessorOnJs(), the preceding logic has already sent the
    // js source to the JS thread, so this operation is no longer necessary
    // here.
    auto data =
        OnRenderTemplate(template_data, card,
                         !EnableDataProcessorOnJs() && !js_posted_before_vm &&
                             ShouldPostDataToJs(),
                         pipeline_options);

    // render template
    RenderTemplate(card, data, pipeline_options);

    // After render template, exec some aftercare
    // 1. ssr actions
    // 2. timing actions
    // 3. reset flag
    DidRenderTemplate(pipeline_options);
  }
  // After load template, exec some aftercare
  // 1. exec callback
  // 2. print log
  // 3. reset flag
  // 4. timing actions
  DidLoadTemplate();
}

// TODO(fulei.bill) support pre_painting_ in ReloadTemplate
void TemplateAssembler::ReloadTemplate(
    const std::shared_ptr<TemplateData>& template_data,
    UpdatePageOption& update_page_option, PipelineOptions& pipeline_options) {
#if ENABLE_TESTBENCH_RECORDER
  // test-bench actions
  tasm::recorder::TemplateAssemblerRecorder::RecordReloadTemplate(template_data,
                                                                  record_id_);
#endif
  Scope scope(this);
  if (is_loading_template_) {
    // TODO(zhoupeng.z): this error should not be a 10X fatal, change the error
    // code later.
    base::LynxError error{error::E_APP_BUNDLE_LOAD_RENDER_FAILED,
                          "ReloadTemplate in another loading process!!!"};
    ReportError(std::move(error));
    return;
  }

  if (!template_loaded_) {
    // reload before loadTemplate is forbidden in Lynx.
    auto error = base::LynxError(
        error::E_APP_BUNDLE_RELOAD_EARLY_RELOAD,
        std::string("ReloadTemplate before LoadTemplate!"),
        "Please loadTemplate first", base::LynxErrorLevel::Error);
    ReportError(std::move(error));
    return;
  }

  // print log
  LOGI("start TemplateAssembler::ReloadTemplate, url:"
       << url_ << " len: " << source_size_ << " this:" << this);

  is_loading_template_ = true;
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "LynxReloadTemplate",
              [&](lynx::perfetto::EventContext ctx) {
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("url");
                debug->set_string_value(url_);
              });
  // Before template load start.
  UpdateGlobalPropsWithDefaultProps(pipeline_options);
  auto card = FindEntry(DEFAULT_ENTRY_NAME);
  if (card && card->GetVm()) {
    card->GetVm()->CleanClosuresInCycleReference();
  }
  if (card && card->GetVm()) {
    card->GetVm()->UpdateGCTiming(true);
  }
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadBundleStart);
  // when reloadTemplate, we will use loadTemplateStart to mock
  // SETUP_DECODE_START, SETUP_DECODE_END
  // SETUP_LEPUS_EXECUTE_START, SETUP_LEPUS_EXECUTE_END
  // SETUP_SET_INIT_DATA_START, SETUP_SET_INIT_DATA_END
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kParseStart);
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kParseEnd);
  tasm::TimingCollector::Instance()->MarkFrameworkTiming(
      tasm::timing::kVmExecuteStart);
  tasm::TimingCollector::Instance()->MarkFrameworkTiming(
      tasm::timing::kVmExecuteEnd);
  tasm::TimingCollector::Instance()->MarkFrameworkTiming(
      tasm::timing::kSetInitDataStart);
  tasm::TimingCollector::Instance()->MarkFrameworkTiming(
      tasm::timing::kSetInitDataEnd);

  // actual_fmp_start_ and actual_fmp_end_ should be reset.
  actual_fmp_start_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
  actual_fmp_end_ = 0;

  // No need to decode and set page config here.
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_VITALS, "LynxDomReady");

  TemplateData data = ProcessTemplateData(template_data, true);

  // destroy old components
  if (EnableFiberArch()) {
    if (card && card->GetVm()) {
      card->GetVm()->Call(BASE_STATIC_STRING(kRemoveComponents));
    }
  } else {
    page_proxy_.RemoveOldComponentBeforeReload();
  }

  // destroy card and create card
  delegate_.OnJSAppReload(GenerateTemplateDataPostedToJs(data),
                          pipeline_options);

  // update data
  update_page_option.from_native = true;
  update_page_option.reload_template = true;

  UpdateTemplate(data, update_page_option, pipeline_options);

  // Here no need to call delegate_.OnDataUpdated();
  // Because this update is like a new template loaded, but not a update.

  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_VITALS);

  // If enable fiber arch, card's name is empty, trigger OnNativeAppReady and do
  // not check its name.
  if (!card->GetName().empty() || EnableFiberArch()) {
    OnNativeAppReady();
  }

  delegate_.OnTemplateLoaded(url_);
  SendFontScaleChanged(font_scale_);
  delegate_.OnTasmFinishByNative();
  is_loading_template_ = false;
  LOGI("end TemplateAssembler::ReloadTemplate");
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kLoadBundleEnd);
  if (card && card->GetVm()) {
    card->GetVm()->UpdateGCTiming(false);
  }
}

void TemplateAssembler::ReloadTemplate(
    const std::shared_ptr<TemplateData>& template_data,
    const lepus::Value& global_props, UpdatePageOption& update_page_option,
    PipelineOptions& pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "LynxReloadTemplateWithGlobalProps");
  if (!global_props.IsNil()) {
    UpdateGlobalProps(global_props, false, pipeline_options);
  }
  ReloadTemplate(template_data, update_page_option, pipeline_options);
}

void TemplateAssembler::ReloadFromJS(const runtime::UpdateDataTask& task,
                                     PipelineOptions& pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "ReloadFromJS");
  Scope scope(this);
  LOGI("Lynx ReloadFromJS. url: " << url_);

  // get default entry
  const auto& card = FindEntry(tasm::DEFAULT_ENTRY_NAME);
  if (card && card->GetVm()) {
    card->GetVm()->CleanClosuresInCycleReference();
  }

  // destroy old components
  if (EnableFiberArch()) {
    if (card && card->GetVm()) {
      card->GetVm()->Call(BASE_STATIC_STRING(kRemoveComponents));
    }
  } else {
    // trigger old components's unmount lifecycle;
    page_proxy_.RemoveOldComponentBeforeReload();
  }

  TemplateData data(task.data_, false);
  // destroy card and create card instance
  delegate_.OnJSAppReload(GenerateTemplateDataPostedToJs(data),
                          pipeline_options);

  UpdatePageOption update_page_option;
  update_page_option.reload_from_js = true;
  update_page_option.reload_template = true;

  // update template
  UpdateTemplate(data, update_page_option, pipeline_options);

  SendFontScaleChanged(font_scale_);
}

void TemplateAssembler::AddFont(const lepus::Value& font) {
  page_proxy()->element_manager()->AddFontFace(font);
}

void TemplateAssembler::DidPreloadComponent(
    LazyBundleLoader::CallBackInfo callback_info) {
  if (callback_info.Success()) {
    InsertLynxTemplateBundle(callback_info.component_url,
                             std::move(*callback_info.bundle));
  } else {
    ReportError(ConstructLynxErrorForLazyBundle(
        callback_info.component_url, callback_info.error_code,
        callback_info.error_msg, kNetworkSuggestion, true));
  }
}

void TemplateAssembler::DidLoadComponent(
    LazyBundleLoader::CallBackInfo callback_info,
    PipelineOptions& pipeline_options) {
  auto component_url = callback_info.component_url;
  if (callback_info.Success()) {
    LoadComponentWithCallbackInfo(std::move(callback_info), pipeline_options);
  } else {
    ReportError(ConstructLynxErrorForLazyBundle(
        callback_info.component_url, callback_info.error_code,
        callback_info.error_msg, kNetworkSuggestion));
    auto error_value = lazy_bundle::ConstructErrorMessageForMTS(
        callback_info.component_url, callback_info.error_code,
        callback_info.error_msg, callback_info.sync);
    component_loader_->MarkComponentLoadedFailed(
        callback_info.component_url, callback_info.error_code, error_value);
    // Only SendGlobalEvent When Failed.
    SendLazyBundleGlobalEvent(callback_info.component_url, error_value);
  }

  if (component_loader_->DispatchOnComponentLoaded(this, component_url)) {
    // TODO(kechenglong): SetNeedsLayout if and only if needed.
    page_proxy()->element_manager()->SetNeedsLayout();
    page_proxy()->element_manager()->OnPatchFinish(pipeline_options);
    page_proxy()->element_manager()->painting_context()->Flush();
  }
}

void TemplateAssembler::LoadComponentWithCallbackInfo(
    LazyBundleLoader::CallBackInfo callback_info,
    PipelineOptions& pipeline_options) {
  const auto& url = callback_info.component_url;
  auto sync = callback_info.sync;
  auto callback_id = callback_info.callback_id;

  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LazyBundle.Load",
              [sync, url](lynx::perfetto::EventContext ctx) {
                ctx.event()->set_name("LoadComponentWithCallback");
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("sync");
                debug->set_string_value(std::to_string(sync));
                auto* url_debug = ctx.event()->add_debug_annotations();
                url_debug->set_name("url");
                url_debug->set_string_value(url);
              });
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::TemplateAssemblerRecorder::RecordLoadComponentWithCallback(
      url, callback_info.data, sync, callback_id, record_id_);
#endif  // ENABLE_TESTBENCH_RECORDER
  LOGI("TemplateAssembler::LoadComponentWithCallback: "
       << url << " sync: " << sync << " callback_id: " << callback_id);
  std::shared_ptr<TemplateEntry> component_entry = FindTemplateEntry(url);
  bool is_success = true;
  if (!component_entry) {
    // if the lazy bundle is first loaded, then need to build
    // templateEntry for it.
    // If returns null, this means the decoding or executing of the dynamic
    // component is failed.
    component_entry = std::make_shared<TemplateEntry>();
    is_success = BuildComponentEntryInternal(
        component_entry, url,
        [this, &url,
         &callback_info](const std::shared_ptr<TemplateEntry>& entry) -> bool {
          component_loader_->StartRecordDecodeTime(url);

          bool res =
              callback_info.bundle
                  ? entry->InitWithTemplateBundle(this, *callback_info.bundle)
                  : this->FromBinary(entry, std::move(callback_info.data),
                                     false);
          if (!res) {
            return false;
          }
          component_loader_->EndRecordDecodeTime(url);
          return true;
        });
  }

  lepus::Value callback_msg;  // message send to js callback.
  if (is_success) {
    // decode success
    callback_msg = lazy_bundle::ConstructSuccessMessageForBTS(url);
    if (callback_id < 0) {
      component_loader_->MarkComponentLoadedSuccess(
          url, lazy_bundle::ConstructSuccessMessageForMTS(
                   url, sync, component_entry->GetBinaryEvalResult(),
                   lazy_bundle::LazyBundleState::STATE_SUCCESS,
                   component_loader_->GetPerfInfo(url)));
    }
    DidComponentLoaded(component_entry);
  } else {
    auto err_code = error::E_LAZY_BUNDLE_LOAD_DECODE_FAILED;
    // decode fail
    std::string err_msg =
        ConstructDecodeErrorMessage(false, url, component_entry->GetErrorMsg());
    callback_msg =
        lazy_bundle::ConstructErrorMessageForBTS(url, err_code, err_msg);
    if (callback_id < 0) {
      // trigger lazy bundle event
      component_loader_->MarkComponentLoadedFailed(
          url, err_code,
          lazy_bundle::ConstructErrorMessageForMTS(url, err_code, err_msg,
                                                   sync));
    }
  }

  if (callback_id >= 0) {
    // invoke load component callback
    delegate_.CallJSApiCallbackWithValue(piper::ApiCallBack(callback_id),
                                         callback_msg);
    if (is_success) {
      // update target lazy bundle via ids
      page_proxy()->OnLazyBundleLoadedFromJS(url, callback_info.component_ids,
                                             pipeline_options);
    }
  }
}

void TemplateAssembler::DidComponentLoaded(
    const std::shared_ptr<TemplateEntry>& component_entry) {
  if (EnableFiberArch()) {
    return;
  }
  if (component_entry != nullptr) {
    component_entry->GetVm()->UpdateTopLevelVariable(kGlobalPropsKey,
                                                     global_props_);
    component_entry->GetVm()->UpdateTopLevelVariable(
        kSystemInfo, GenerateSystemInfo(nullptr));
  }
}

bool TemplateAssembler::BuildComponentEntryInternal(
    const std::shared_ptr<TemplateEntry>& component_entry,
    const std::string& url,
    const base::MoveOnlyClosure<bool, const std::shared_ptr<TemplateEntry>&>&
        entry_initializer) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LazyBundle::BuildTemplateEntry", "url",
              url);
  component_entry->SetIsCard(false);
  component_entry->SetName(url);
  component_entry->SetLepusObserver(lepus_observer_);
  if (!entry_initializer(component_entry)) {
    return false;
  }

  delegate_.OnComponentDecoded(component_entry->CreateTasmRuntimeBundle());

  // Check whether the lazy bundle is compatible with the page. When
  // the dsl of the lazy bundle is different with the dsl of the page,
  // or the mode of the lazy bundle is different with the mode of the
  // page, an error will be reported.
  std::string error;
  if (!component_entry->IsCompatibleWithRootEntry(
          *(FindEntry(DEFAULT_ENTRY_NAME)), error)) {
    auto lynx_error = ConstructLynxErrorForLazyBundle(
        url, error::E_LAZY_BUNDLE_LOAD_DECODE_FAILED, error,
        "Please check that the lazy bundles and the page are compatible "
        "with each other. ");
    LOGE(lynx_error.error_message_);
    component_entry->SetErrorMsg(lynx_error.error_message_);
    ReportError(std::move(lynx_error));
    return false;
  }

  constexpr char kLepusErrorSuggestion[] = "Please check your lepus code. ";

  InsertEntry(url, component_entry);
  if (!component_entry->GetVm()) {
    constexpr char kNullContextError[] = "LazyBundle's context is null. ";
    auto lynx_error = ConstructLynxErrorForLazyBundle(
        url, error::E_LAZY_BUNDLE_LOAD_DECODE_FAILED, kNullContextError,
        kLepusErrorSuggestion);
    LOGE(lynx_error.error_message_);
    component_entry->SetErrorMsg(lynx_error.error_message_);
    ReportError(std::move(lynx_error));
    return false;
  }

  OnDynamicJSSourcePrepared(url);

  // When entry does not EnableReuseContext, it means that the entry will init a
  // new vm, and we need to set some initial data to the vm before the vm
  // executes the script.
  if (!component_entry->EnableReuseContext()) {
    component_entry->GetVm()->SetPropertyToLynx(
        BASE_STATIC_STRING(kGlobalPropsKey), global_props_);
    component_entry->GetVm()->SetPropertyToLynx(
        BASE_STATIC_STRING(kSystemInfo), tasm::GenerateSystemInfo(nullptr));
  }

  if (!component_entry->Execute()) {
    auto lynx_error = ConstructLynxErrorForLazyBundle(
        url, error::E_LAZY_BUNDLE_LOAD_DECODE_FAILED, "vm execute failed",
        kLepusErrorSuggestion);
    component_entry->SetErrorMsg(lynx_error.error_message_);
    return false;
  }

  return true;
}

void TemplateAssembler::SetPageConfigRadonMode() const {
  if (!page_config_) {
    return;
  }
  page_config_->SetRadonMode("RadonDiff");
}

void TemplateAssembler::SetPageConfig(
    const std::shared_ptr<PageConfig>& config) {
  if (config) {
    page_config_ = config;

    page_config_->DecodePageConfigFromJsonStringWhileUndefined(
        platform_config_json_string_);
    // pass page config to android/iOS side after VM->Execute()
    // see `SetPageConfig` called by `LoadTemplate/LoadComponent`
    // in template_assembler.cc
  }
}

void TemplateAssembler::ReportError(int32_t error_code, const std::string& msg,
                                    base::LynxErrorLevel level) {
  ReportError(base::LynxError(error_code, msg, "", level));
}

void TemplateAssembler::ReportError(base::LynxError error) {
  if (!error.error_message_.empty()) {
    delegate_.OnErrorOccurred(std::move(error));
  }
}

void TemplateAssembler::ReportGCTimingEvent(const char* start,
                                            const char* end) {
  std::string start_info(start);
  std::string end_info(end);
  tasm::report::EventTracker::OnEvent([start_info = std::move(start_info),
                                       end_info = std::move(end_info)](
                                          tasm::report::MoveOnlyEvent& event) {
    rapidjson::Document doc_start;
    rapidjson::Document doc_end;
    doc_start.Parse(start_info);
    doc_end.Parse(end_info);
    if (doc_start.HasParseError() || !doc_start.IsObject() ||
        doc_end.HasParseError() || !doc_end.IsObject()) {
      return;
    }
    event.SetName("lynxsdk_gc_timing_info");
    uint64_t gc_count = doc_end["gc_duration"].GetUint64() -
                        doc_start["gc_duration"].GetUint64();
    if (gc_count > 0) {
      event.SetProps("gc_count", gc_count);
      event.SetProps("gc_duration", doc_end["gc_duration"].GetUint64() -
                                        doc_start["gc_duration"].GetUint64());
    }
    event.SetProps("gc_heapsize_growth",
                   doc_end["gc_heapsize"].GetUint64() -
                       doc_start["gc_heapsize"].GetUint64());
    event.SetProps("gc_count_begin", doc_start["gc_duration"].GetUint64());
    event.SetProps("gc_heapsize_end", doc_end["gc_heapsize"].GetUint64());
  });
}

fml::RefPtr<fml::TaskRunner> TemplateAssembler::GetLepusTimedTaskRunner() {
  return delegate_.GetLepusTimedTaskRunner();
}

std::unique_ptr<lepus::Value> TemplateAssembler::GetCurrentData() {
  // If enbale fiber arch, get data from lepus runtime, otherwise, get data from
  // page_proxy.
  if (EnableFiberArch()) {
    auto default_entry = FindEntry(tasm::DEFAULT_ENTRY_NAME);
    if (default_entry && default_entry->GetVm()) {
      // If getCurrentData is executed, call the getPageData function of
      // LepusRuntime without passing any parameters to get all data from the
      // page.
      return std::make_unique<lepus::Value>(lepus::Value::Clone(
          default_entry->GetVm()->Call(BASE_STATIC_STRING(kGetPageData))));
    }
    return nullptr;
  }
  return page_proxy()->GetData();
}

lepus::Value TemplateAssembler::GetPageDataByKey(
    const std::vector<std::string>& keys) {
  // If enbale fiber arch, get data from lepus runtime, otherwise, get data from
  // page_proxy.
  if (EnableFiberArch()) {
    auto default_entry = FindEntry(tasm::DEFAULT_ENTRY_NAME);
    if (default_entry && default_entry->GetVm()) {
      // When executing getPageDataByKey, still call the getPageData function of
      // LepusRuntime, but pass in the keys converted to lepus::Array as a
      // parameter to obtain the data corresponding to these keys.
      auto ary = lepus::CArray::Create();
      for (const auto& key : keys) {
        ary->emplace_back(key);
      }
      return lepus::Value::Clone(default_entry->GetVm()->Call(
          BASE_STATIC_STRING(kGetPageData), lepus::Value(std::move(ary))));
    }
    return lepus::Value();
  }
  return page_proxy()->GetDataByKey(keys);
}

void TemplateAssembler::UpdateComponentData(const runtime::UpdateDataTask& task,
                                            PipelineOptions& pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxUpdateComponentDataByJS",
              [update_data_type = static_cast<uint32_t>(task.type_),
               instance_id = instance_id_,
               stacks = task.stacks_](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations(
                    "update_data_type", std::to_string(update_data_type));
                ctx.event()->add_debug_annotations("instance_id_",
                                                   std::to_string(instance_id));
                ctx.event()->add_debug_annotations("stacks", stacks);
              });

  Scope scope(this);
  LOGI("TemplateAssembler::UpdateComponentData. this:"
       << this << " url:" << url_
       << " update_data_type:" << static_cast<uint32_t>(task.type_));
  ComponentUpdateReporter updateReporter(task, page_proxy_);
  lepus_value v =
      task.data_.GetProperty(BASE_STATIC_STRING(k_actual_first_screen));
  if (v.IsTrue()) {
    page_proxy_.UpdateComponentData(task.component_id_, task.data_,
                                    pipeline_options);

    if (actual_fmp_end_ == 0) {
      actual_fmp_end_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    }
  } else {
    page_proxy_.UpdateComponentData(task.component_id_, task.data_,
                                    pipeline_options);
  }

  delegate_.CallJSApiCallback(task.callback_);
}

void TemplateAssembler::SelectComponent(const std::string& component_id,
                                        const std::string& id_selector,
                                        const bool single,
                                        piper::ApiCallBack callback) {
  std::vector<std::string> target_comp_ids =
      page_proxy_.SelectComponent(component_id, id_selector, single);
  auto array = lepus::CArray::Create();
  for (const auto& comp_id : target_comp_ids) {
    array->emplace_back(comp_id);
  }
  delegate_.CallJSApiCallbackWithValue(callback,
                                       lepus::Value(std::move(array)));
}

void TemplateAssembler::ElementAnimate(const std::string& component_id,
                                       const std::string& id_selector,
                                       const lepus::Value& args) {
  NodeSelectRoot root = NodeSelectRoot::ByComponentId(component_id);
  NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                            id_selector);
  options.only_current_component = false;
  auto elements = page_proxy_.SelectElements(root, options);
  if (elements.empty() || elements[0] == nullptr) {
    return;
  }
  elements[0]->Animate(args);
}

void TemplateAssembler::GetComponentContextDataAsync(
    const std::string& component_id, const std::string& key,
    piper::ApiCallBack callback) {
  lepus::Value ctx_value =
      page_proxy_.GetComponentContextDataByKey(component_id, key);
  delegate_.CallJSApiCallbackWithValue(callback, ctx_value);
}

lepus::Value TemplateAssembler::TriggerLepusBridge(
    const std::string& method_name, const lepus::Value& msg) {
  LOGV("TriggerLepusBridge triggered in template assembler"
       << method_name << " this:" << this);
  return delegate_.TriggerLepusMethod(method_name, msg);
}

void TemplateAssembler::TriggerLepusBridgeAsync(const std::string& method_name,
                                                const lepus::Value& arguments,
                                                bool is_air) {
  LOGV("TriggerLepusBridge Async triggered in template assembler"
       << method_name << " this:" << this);
  delegate_.TriggerLepusMethodAsync(method_name, arguments, is_air);
}

void TemplateAssembler::InvokeLepusCallback(const int32_t callback_id,
                                            const std::string& entry_name,
                                            const lepus::Value& data) {
  const auto& current_entry = FindEntry(entry_name);
  current_entry->InvokeLepusBridge(callback_id, data);
}

void TemplateAssembler::InvokeLepusComponentCallback(
    const int64_t callback_id, const std::string& entry_name,
    const lepus::Value& data) {
  touch_event_handler_->HandleJSCallbackLepusEvent(callback_id, this, data);
}

void TemplateAssembler::LepusInvokeUIMethod(
    std::vector<int32_t> ui_impl_ids, const std::string& method,
    const lepus::Value& params, lepus::Context* context,
    std::unique_ptr<lepus::Value> callback_closure) {
  if (ui_impl_ids.empty()) {
    if (callback_closure && callback_closure->IsJSFunction()) {
      context->CallClosure(
          *callback_closure,
          LynxGetUIResult(std::move(ui_impl_ids), LynxGetUIResult::UNKNOWN,
                          "No node in the input parameter")
              .StatusAsLepusValue());
    }
    return;
  }
  if (EnableFiberArch()) {
    auto callback = context->GetCallbackManager()->CacheTask(
        context, std::move(callback_closure));
    page_proxy()->element_manager()->catalyzer()->Invoke(
        ui_impl_ids[0], method, pub::ValueImplLepus(params),
        fml::MakeCopyable(
            [ui_impl_ids = std::move(ui_impl_ids), callback, context](
                const int32_t code, const pub::Value& data) mutable {
              const auto result_dict = lepus::Dictionary::Create();
              BASE_STATIC_STRING_DECL(kCode, "code");
              BASE_STATIC_STRING_DECL(kData, "data");
              result_dict->SetValue(kCode, code);
              result_dict->SetValue(
                  kData, pub::ValueUtils::ConvertValueToLepusValue(data));
              context->GetCallbackManager()->InvokeTask(
                  callback, lepus::Value(std::move(result_dict)));
            }));
    return;
  }
#if ENABLE_AIR
  delegate_.LepusInvokeUIMethod(std::move(ui_impl_ids), method, params, context,
                                std::move(callback_closure));
#endif
}

void TemplateAssembler::TriggerComponentEvent(const std::string& event_name,
                                              const lepus::Value& msg) {
  if (!template_loaded_) {
    return;
  }
  if (!EnableLynxAir()) {
    EnsureTouchEventHandler();
    touch_event_handler_->HandleTriggerComponentEvent(this, event_name, msg);
  }
#if ENABLE_AIR
  else {
    EnsureAirTouchEventHandler();
    air_touch_event_handler_->TriggerComponentEvent(this, event_name, msg);
  }
#endif
}

void TemplateAssembler::CallJSFunctionInLepusEvent(
    const std::string& component_id, const std::string& name,
    const lepus::Value& params) {
  if (!template_loaded_) {
    return;
  }
  EnsureTouchEventHandler();
  touch_event_handler_->CallJSFunctionInLepusEvent(component_id, name, params);
}

void TemplateAssembler::TriggerLepusGlobalEvent(const std::string& event_name,
                                                const lepus::Value& msg) {
  if (!template_loaded_) {
    return;
  }
  SendGlobalEventToLepus(event_name, std::move(msg));
  LOGI("TemplateAssembler TriggerLepusGlobalEvent event" << event_name
                                                         << " this:" << this);
}

void TemplateAssembler::TriggerWorkletFunction(std::string component_id,
                                               std::string worklet_module_name,
                                               std::string method_name,
                                               lepus::Value args,
                                               piper::ApiCallBack callback) {
#if ENABLE_LEPUSNG_WORKLET
  if (!template_loaded_) {
    return;
  }

  int comp_id = 0;
  BaseComponent* component;
  if (component_id.empty() || component_id == PAGE_ID) {
    component = page_proxy()->Page();
  } else if (!base::StringToInt(component_id, &comp_id, 10)) {
    ReportError(error::E_WORKLET_MODULE_EXCEPTION,
                "Component_id error, make sure component_id is either 'card' "
                "or of int type, now it is" +
                    component_id);
    return;
  } else {
    component = page_proxy()->ComponentWithId(comp_id);
  }

  EnsureTouchEventHandler();

  std::optional<lepus::Value> call_result =
      worklet::LepusElement::TriggerWorkletFunction(
          this, component, worklet_module_name, method_name, args,
          touch_event_handler_->GetTaskHandler());

  if (call_result.has_value()) {
    delegate_.CallJSApiCallbackWithValue(callback, *call_result);
  }
#endif  // ENABLE_LEPUSNG_WORKLET
}

void TemplateAssembler::Destroy() {
  LOGI("TemplateAssembler::Destroy url:" << url_ << " this:" << this);
  destroyed_ = true;
  page_proxy_.Destroy();
  signal_context_.WillDestroy();
}

void TemplateAssembler::GetDecodedJSSource(
    std::unordered_map<std::string, std::string>& js_source) {
  ForEachEntry([&js_source](const auto& entry) {
    const auto& sources = entry->GetJsBundle().GetAllJsFiles();
    for (const auto& [url, source] : sources) {
      if (source.IsSourceCode()) {
        auto buffer = source.GetBuffer();
        js_source.emplace(url, std::string(reinterpret_cast<const char*>(
                                               source.GetBuffer()->data()),
                                           buffer->size()));
      }
    }
  });
}

lepus::Value& TemplateAssembler::GetComponentInfoMap(
    const std::string& entry_name) {
  return FindEntry(entry_name)->component_info_map();
}

lepus::Value& TemplateAssembler::GetComponentPathMap(
    const std::string& entry_name) {
  return FindEntry(entry_name)->component_path_map();
}

void TemplateAssembler::SendBubbleEvent(const std::string& name, int tag,
                                        lepus::DictionaryPtr dict) {
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::TemplateAssemblerRecorder::RecordBubbleEvent(
      name, tag, page_proxy()->element_manager()->root()->impl_id(),
      lepus::Value(dict), record_id_);
#endif
  if (!template_loaded_) {
    LOGI("Lynx SendBubbleEvent failed, template_loaded_=false"
         << " this:" << this);
    return;
  }
  if (destroyed()) {
    LOGI("Lynx SendBubbleEvent failed, destroyed=true"
         << " this:" << this);
    return;
  }
  EnsureTouchEventHandler();

  touch_event_handler_->HandleBubbleEvent(
      this, FindEntry(DEFAULT_ENTRY_NAME)->GetName(), name, tag, dict);
}

/**
 * Sends a gesture event to an element with the specified tag and gesture ID.
 *
 * @param tag The tag of the element to send the gesture event to.
 * @param gesture_id The ID of the gesture to send.
 * @param name The name of the gesture callback to invoke.
 * @param params The parameters to pass to the gesture callback.
 */
void TemplateAssembler::SendGestureEvent(int tag, int gesture_id,
                                         const std::string& name,
                                         const lepus::Value& params) {
  // Invoke the gesture callback using the touch event handler.
  EnsureTouchEventHandler();
  touch_event_handler_->HandleGestureEvent(this, name, tag, gesture_id, params);
}

void TemplateAssembler::SendCustomEvent(const std::string& name, int tag,
                                        const lepus::Value& params,
                                        const std::string& pname) {
#if ENABLE_TESTBENCH_RECORDER
  if (page_proxy()->element_manager()->root()) {
    tasm::recorder::TemplateAssemblerRecorder::RecordCustomEvent(
        name, tag, page_proxy()->element_manager()->root()->impl_id(), params,
        pname, record_id_);
  }
#endif
  if (destroyed()) {
    LOGI("Lynx SendCustomEvent failed, destroyed=true"
         << " this:" << this);
    return;
  }
  if (!EnableLynxAir()) {
    EnsureTouchEventHandler();
    touch_event_handler_->HandleCustomEvent(this, name, tag, params, pname);
  }
#if ENABLE_AIR
  else {
    EnsureAirTouchEventHandler();
    air_touch_event_handler_->HandleCustomEvent(this, name, tag, params, pname);
  }
#endif
}

void TemplateAssembler::SendAirComponentEvent(const std::string& event_name,
                                              const int component_id,
                                              const lepus::Value& params,
                                              const std::string& param_name) {
#if ENABLE_AIR
  if (EnableLynxAir()) {
    EnsureAirTouchEventHandler();
    air_touch_event_handler_->SendComponentEvent(this, event_name, component_id,
                                                 params, param_name);
  }
#endif
}

void TemplateAssembler::OnPseudoStatusChanged(int32_t id, uint32_t pre_status,
                                              uint32_t current_status) {
  DCHECK(pre_status >= 0 &&
         pre_status <= std::numeric_limits<PseudoState>::max());
  DCHECK(current_status >= 0 &&
         current_status <= std::numeric_limits<PseudoState>::max());
  EnsureTouchEventHandler();
  touch_event_handler_->HandlePseudoStatusChanged(
      id, static_cast<PseudoState>(pre_status),
      static_cast<PseudoState>(current_status));
}

void TemplateAssembler::SendTouchEvent(const std::string& name,
                                       const EventInfo& info) {
  if (!template_loaded_) {
    LOGI("Lynx SendTouchEvent failed, template_loaded_=false"
         << " this:" << this);
    return;
  }
  if (destroyed()) {
    LOGI("Lynx SendTouchEvent failed, destroyed=true"
         << " this:" << this);
    return;
  }
  if (!EnableLynxAir()) {
    EnsureTouchEventHandler();
    touch_event_handler_->HandleTouchEvent(
        this, FindEntry(DEFAULT_ENTRY_NAME)->GetName(), name, info);
#if ENABLE_TESTBENCH_RECORDER
    tasm::recorder::TemplateAssemblerRecorder::RecordTouchEvent(
        name, page_proxy()->element_manager()->root()->impl_id(), info,
        record_id_);
#endif
  }
#if ENABLE_AIR
  else {
    EnsureAirTouchEventHandler();
    // TODO(jiyishen): optimize parameters to pass info instead of info's
    // context
    air_touch_event_handler_->HandleTouchEvent(
        this, FindEntry(DEFAULT_ENTRY_NAME)->GetName(), name, info.tag, info.x,
        info.y, info.client_x, info.client_y, info.page_x, info.page_y);
  }
#endif
}

TemplateData TemplateAssembler::GenerateTemplateDataPostedToJs(
    const TemplateData& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ConvertValueWithReadOnly");
  if (EnableDataProcessorOnJs()) {
    return TemplateData::CopyPlatformData(value);
  } else {
    return TemplateData::ShallowCopy(value);
  }
}

void TemplateAssembler::UpdateMetaData(
    const std::shared_ptr<TemplateData>& template_data,
    const lepus::Value& global_props, UpdatePageOption& update_page_option,
    PipelineOptions& pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "UpdateMetaData");
  if (destroyed()) {
    return;
  }
  bool global_props_changed = !global_props.IsNil();
  bool data_changed = template_data != nullptr;

  if (global_props_changed) {
    UpdateGlobalProps(global_props, !data_changed, pipeline_options);
  }

  if (data_changed) {
    update_page_option.global_props_changed = global_props_changed;
    UpdateDataByPreParsedData(template_data, update_page_option,
                              pipeline_options);
  }
}

void TemplateAssembler::UpdateDataByPreParsedData(
    const std::shared_ptr<TemplateData>& template_data,
    UpdatePageOption& update_page_option, PipelineOptions& pipeline_options) {
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::TemplateAssemblerRecorder::RecordUpdateDataByPreParsedData(
      template_data, update_page_option, record_id_);
#endif
  if (template_data == nullptr || destroyed()) {
    return;
  }

  if (page_proxy_.HasSSRRadonPage()) {
    LOGI("TemplateAssembler::Update Data for SSR");
    std::vector<base::String> keys_updated;
    ssr::SsrDataUpdateManager::GetUpdatedKeys(template_data->GetValue(),
                                              keys_updated);
    TemplateData data = ProcessTemplateData(template_data, false);
    page_proxy()->UpdateDataForSsr(keys_updated, data.GetValue(),
                                   pipeline_options);
    return;
  }

  LOGI("TemplateAssembler::UpdateDataByPreParsedData url:"
       << url_ << " this:" << this
       << " reset:" << update_page_option.reset_page_data
       << " state:" << (template_loaded_ ? "after" : "before")
       << " loadTemplate enablePreUpdateData:"
       << this->enable_pre_update_data_);

  if (template_loaded_) {
    uint64_t update_data_trigger = base::CurrentSystemTimeMicroseconds();

    TemplateData data = ProcessTemplateData(template_data, false);
    const auto& timing_flag = tasm::GetTimingFlag(data.GetValue());
    tasm::TimingCollector::Scope<Delegate> scope(&delegate_, pipeline_options);
    if (!timing_flag.empty()) {
      pipeline_options.need_timestamps = true;
      delegate_.BindPipelineIDWithTimingFlag(pipeline_options.pipeline_id,
                                             timing_flag);
      tasm::TimingCollector::Instance()->Mark(tasm::timing::kSetStateTrigger,
                                              update_data_trigger);
    }

    bool post_data_before_update =
        EnableDataProcessorOnJs() &&
        LynxEnv::GetInstance().EnablePostDataBeforeUpdateTemplate();
    if (pre_painting_) {
      FindEntry(tasm::DEFAULT_ENTRY_NAME)
          ->SetInitData(GenerateTemplateDataPostedToJs(data));
      OnJSPrepared(url_, pipeline_options);
      page_proxy_.SetPrePaintingStage(PrePaintingStage::kStartUpdatePage);
    } else if (post_data_before_update) {
      OnDataUpdatedByNative(GenerateTemplateDataPostedToJs(data),
                            update_page_option.reset_page_data);
    }

    UpdateTemplate(data, update_page_option, pipeline_options);

    if (pre_painting_) {
      OnNativeAppReady();
      pre_painting_ = false;
    } else if (!post_data_before_update) {
      OnDataUpdatedByNative(GenerateTemplateDataPostedToJs(data),
                            update_page_option.reset_page_data);
    }

    // invoke tasm finish only when needed.
    delegate_.OnTasmFinishByNative();
  } else if (enable_pre_update_data_) {
    if (!update_page_option.reset_page_data) {
      cache_data_.emplace_back(template_data);
    }
  }
}

bool TemplateAssembler::UpdateConfig(const lepus::Value& config,
                                     bool noticeDelegate,
                                     PipelineOptions& pipeline_options) {
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::TemplateAssemblerRecorder::RecordUpdateConfig(
      config, noticeDelegate, record_id_);
#endif
  if (destroyed()) {
    return false;
  }
  if (noticeDelegate) {
    // must be called before page update, so that the theme in native should
    // be updated before page theme callback function runs
    delegate_.OnConfigUpdated(config);
  }
  lepus::Value configToJS;
  if (page_proxy_.UpdateConfig(config, configToJS, true, pipeline_options)) {
    delegate_.OnCardConfigDataChanged(configToJS);
    return true;
  }
  return false;
}

void TemplateAssembler::UpdateDataByJS(const runtime::UpdateDataTask& task,
                                       PipelineOptions& pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxUpdateDataByJS",
              [update_data_type = static_cast<uint32_t>(task.type_),
               instance_id = instance_id_, stacks = std::move(task.stacks_)](
                  lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations(
                    "update_data_type", std::to_string(update_data_type));
                ctx.event()->add_debug_annotations("instance_id_",
                                                   std::to_string(instance_id));
                ctx.event()->add_debug_annotations("stacks", stacks);
              });

  LOGI("TemplateAssembler::UpdateDataByJS this:"
       << this << " url:" << url_
       << " update_data_type:" << static_cast<uint32_t>(task.type_));
  ComponentUpdateReporter updateReporter(task, page_proxy_);
  if (task.data_.IsObject()) {
    auto table = task.data_.Table();
    if (table->Contains(BASE_STATIC_STRING(CARD_CONFIG_STR))) {
      UpdateConfig(table->GetValue(BASE_STATIC_STRING(CARD_CONFIG_STR)), true,
                   pipeline_options);
      return;
    }
  }
  UpdatePageOption update_page_option;
  update_page_option.from_native = false;
  if (UpdateGlobalDataInternal(task.data_, update_page_option,
                               pipeline_options)) {
    // data.value_.Table()->dump();
    delegate_.OnDataUpdated();
  }
}

bool TemplateAssembler::FromBinary(const std::shared_ptr<TemplateEntry>& entry,
                                   std::vector<uint8_t> source, bool is_card) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FromBinary");

  auto ReportDecodeError = [this](bool is_card,
                                  const std::shared_ptr<TemplateEntry>& entry,
                                  const std::string& error_msg) {
    if (is_card) {
      auto msg =
          ConstructDecodeErrorMessage(is_card, entry->GetName(), error_msg);
      base::LynxError error{error::E_APP_BUNDLE_LOAD_PARSE_FAILED, msg};
      this->ReportError(std::move(error));
    } else {
      this->ReportError(ConstructLynxErrorForLazyBundle(
          entry->GetName(), error::E_LAZY_BUNDLE_LOAD_DECODE_FAILED, error_msg,
          "Decode error, please check your project config or contact lynx"));
    }
  };

  if (source.empty()) {
    constexpr char kEmptyTemplate[] =
        "the template file size is 0. Maybe the device is not connected to the "
        "network. ";
    ReportDecodeError(is_card, entry, kEmptyTemplate);
    return false;
  }

  auto input_stream =
      std::make_unique<lepus::ByteArrayInputStream>(std::move(source));

  auto reader = std::make_unique<TemplateBinaryReader>(this, entry.get(),
                                                       std::move(input_stream));

  reader->SetIsCardType(is_card);
  reader->SetTemplateUrl(url_.substr(0, url_.find("?")));

  if (!reader->Decode()) {
    ReportDecodeError(is_card, entry, reader->error_message_);
    return false;
  }

  entry->SetLazyReader(std::move(reader));
  return true;
}

bool TemplateAssembler::UpdateGlobalDataInternal(
    const lepus_value& value, const UpdatePageOption& update_page_option,
    PipelineOptions& pipeline_options) {
  Scope scope(this);

  if (!value.IsObject()) {
    return false;
  }

  bool result = page_proxy_.UpdateGlobalDataInternal(value, update_page_option,
                                                     pipeline_options);
  lepus_value v = value.GetProperty(BASE_STATIC_STRING(k_actual_first_screen));
  if (v.IsTrue()) {
    if (actual_fmp_end_ == 0) {
      actual_fmp_end_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    }
  }
  return result;
}

void TemplateAssembler::OnDataUpdatedByNative(tasm::TemplateData value,
                                              const bool reset) {
  delegate_.OnDataUpdatedByNative(std::move(value), reset);
}

void TemplateAssembler::NotifyGlobalPropsChanged(const lepus::Value& value) {
  delegate_.OnGlobalPropsUpdated(value);
}

void TemplateAssembler::EnsureTouchEventHandler() {
  if (touch_event_handler_ == nullptr) {
    auto& client = page_proxy_.element_manager();
    if (client != nullptr) {
      touch_event_handler_ = std::make_unique<TouchEventHandler>(
          client->node_manager(), delegate_, support_component_js_,
          UseLepusNG(), target_sdk_version_);
    } else {
      LYNX_ERROR(error::E_EVENT_EXCEPTION,
                 "Element manager of page proxy is nullptr",
                 "This error is caught by native, please ask Lynx for help.");
    }
  }
}

void TemplateAssembler::EnsureAirTouchEventHandler() {
#if ENABLE_AIR
  if (!air_touch_event_handler_) {
    if (page_proxy_.element_manager()) {
      air_touch_event_handler_ = std::make_unique<AirTouchEventHandler>(
          page_proxy_.element_manager()->air_node_manager());
    } else {
      LYNX_ERROR(error::E_EVENT_EXCEPTION,
                 "Element manager of page proxy is nullptr",
                 "This error is caught by native, please ask Lynx for help.");
    }
  }
#endif
}

void TemplateAssembler::OnFontScaleChanged(float scale) {
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::TemplateAssemblerRecorder::RecordUpdateFontScale(
      scale, "updateFontScale", record_id_);
#endif
  if (scale == font_scale_) {
    return;
  }
  font_scale_ = scale;
  SendFontScaleChanged(font_scale_);
}

void TemplateAssembler::SendFontScaleChanged(float scale) {
  // SendFontScaleChanged to front-end
  auto arguments = lepus::CArray::Create();
  auto params = lepus::CArray::Create();
  auto dic = lepus::Dictionary::Create();
  BASE_STATIC_STRING_DECL(kScale, "scale");
  dic->SetValue(kScale, scale);

  params->emplace_back(std::move(dic));
  // name
  BASE_STATIC_STRING_DECL(kOnFontScaleChanged, "onFontScaleChanged");
  arguments->emplace_back(kOnFontScaleChanged);
  // params
  arguments->emplace_back(std::move(params));
  delegate_.CallJSFunction("GlobalEventEmitter", "emit",
                           lepus_value(std::move(arguments)));
}

void TemplateAssembler::SendGlobalEvent(const std::string& name,
                                        const lepus::Value& info) {
  auto args = lepus::CArray::Create();
  args->emplace_back(name);
  // info be ShallowCopy first to avoid to be marked const.
  args->emplace_back(lepus_value::ShallowCopy(info));
  runtime::MessageEvent event(runtime::kMessageEventTypeSendGlobalEvent,
                              runtime::ContextProxy::Type::kCoreContext,
                              runtime::ContextProxy::Type::kJSContext,
                              lepus::Value(std::move(args)));
  delegate_.DispatchMessageEvent(std::move(event));
}

void TemplateAssembler::SetFontScale(float scale) {
  LOGI("TemplateAssembler::SetFontScale:" << scale);
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::TemplateAssemblerRecorder::RecordUpdateFontScale(
      scale, "setFontScale", record_id_);
#endif
  font_scale_ = scale;
}

void TemplateAssembler::SetPlatformConfig(
    std::string platform_config_json_string) {
  platform_config_json_string_ = std::move(platform_config_json_string);
}

void TemplateAssembler::OnScreenMetricsSet(float width, float height) {
  LOGI("TemplateAssembler::OnScreenMetricsSet with width: "
       << width << " height: " << height);
  auto& client = page_proxy_.element_manager();
  if (client == nullptr) {
    LOGE("Update ScreenMetrics failed since element manager is null!!");
    return;
  }

  auto kWidth_str = BASE_STATIC_STRING(kWidth);
  auto kHeight_str = BASE_STATIC_STRING(kHeight);

  auto input = lepus::Value::CreateObject();
  input.SetProperty(
      kWidth_str,
      lepus::Value(width *
                   client->GetLynxEnvConfig().PhysicalPixelsPerLayoutUnit()));
  input.SetProperty(
      kHeight_str,
      lepus::Value(height *
                   client->GetLynxEnvConfig().PhysicalPixelsPerLayoutUnit()));
  lepus::Value result;
  if (EnableFiberArch()) {
    result =
        FindEntry(tasm::DEFAULT_ENTRY_NAME)
            ->GetVm()
            ->Call(BASE_STATIC_STRING(kProcessData), input,
                   lepus::Value(BASE_STATIC_STRING(SCREEN_METRICS_OVERRIDER)));
  } else if (page_proxy() != nullptr) {
    result = page_proxy()->OnScreenMetricsSet(input);
  }

  auto width_res = result.GetProperty(kWidth_str);
  auto height_res = result.GetProperty(kHeight_str);
  if (width_res.IsNumber() && height_res.IsNumber()) {
    width = width_res.Number() /
            client->GetLynxEnvConfig().PhysicalPixelsPerLayoutUnit();
    height = height_res.Number() /
             client->GetLynxEnvConfig().PhysicalPixelsPerLayoutUnit();
  } else {
    LOGE(
        "getScreenMetricsOverride should return table with width and height "
        "fields as numbers!!");
  }

  // update screen size info for EventReporter
  report::EventTracker::UpdateGenericInfo(instance_id_, "screen_height",
                                          height);
  report::EventTracker::UpdateGenericInfo(instance_id_, "screen_width", width);

  // update element tree and layout tree
  client->UpdateScreenMetrics(width, height);
  return;
}

void TemplateAssembler::UpdateViewport(float width, int32_t width_mode,
                                       float height, int32_t height_mode) {
  Scope scope(this);
  page_proxy_.element_manager()->OnUpdateViewport(width, width_mode, height,
                                                  height_mode, true);
}

void TemplateAssembler::OnLazyBundlePerfReady(const lepus::Value& perf_info) {
  // trigger client-event
  delegate_.OnDynamicComponentPerfReady(perf_info);
}

std::string TemplateAssembler::GetTargetUrl(const std::string& current,
                                            const std::string& target) {
  // Use target component name to get target url. If not found, use target
  // name as target url.
  std::string url = target;
  const auto& current_entry = FindEntry(current);
  const auto& declarations = current_entry->lazy_bundle_declarations();
  const auto& target_iter = declarations.find(target);
  if (target_iter != declarations.end()) {
    url = target_iter->second;
  }
  return url;
}

std::shared_ptr<TemplateEntry> TemplateAssembler::RequireTemplateEntry(
    RadonLazyComponent* lazy_bundle, const std::string& url,
    const lepus::Value& callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LazyBundle::RequireTemplateEntry",
              [url](lynx::perfetto::EventContext ctx) {
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("url");
                debug->set_string_value(url);
              });
#if ENABLE_TESTBENCH_RECORDER
  // To record every template require for lazy bundle
  tasm::recorder::RecordRequireTemplateScope scope(this, url, record_id_);
#endif  // ENABLE_TESTBENCH_RECORDER
  LOGI("LoadLazyBundle RequireTemplate: " << url);
  auto lifecycle_option =
      std::make_unique<LazyBundleLifecycleOption>(url, instance_id_);
  lifecycle_option->callback = callback;
  lifecycle_option->enable_fiber_arch = EnableFiberArch();
  if (lazy_bundle) {
    lifecycle_option->component_instance = lazy_bundle;
    lifecycle_option->component_uid = lazy_bundle->Uid();
  }

  auto entry = FindTemplateEntry(url);
  LazyBundleState state = LazyBundleState::STATE_CACHE;

  auto request_scope = CreateRequestScope(url);
  // if not found target entry, try to look up preloaded bundles
  if (entry == nullptr) {
    LOGI("RequireTemplate: Check preloaded bundles: " << url);
    entry = BuildTemplateEntryFromPreload(url);
    state = LazyBundleState::STATE_PRELOAD;
  }

  if (entry) {
    // Just trigger closure. It means:
    // 1. the lazy bundle is already loaded.
    // 2. the lazy bundle is already preloaded.
    if (lazy_bundle) {
      lazy_bundle->SetLazyBundleState(
          state, lazy_bundle::ConstructSuccessMessageForMTS(
                     url, true, lepus::Value(), state));
    } else {
      TriggerLepusClosure(callback,
                          lazy_bundle::ConstructSuccessMessageForMTS(
                              url, true, entry->GetBinaryEvalResult(), state));
    }
    lifecycle_option->mode = state;
    return entry;
  }

  LOGI("RequireTemplate: Request Template: " << url);
  return RequestTemplateEntryInternal(std::move(lifecycle_option), lazy_bundle);
}

std::shared_ptr<TemplateEntry> TemplateAssembler::BuildTemplateEntryFromPreload(
    const std::string& url) {
  auto preload_bundle = GetPreloadTemplateBundle(url);
  if (preload_bundle) {
    LOGI("LoadLazyBundle Find Entry from Preload: " << url);
    auto entry = std::make_shared<TemplateEntry>();
    if (BuildComponentEntryInternal(
            entry, url,
            [this, bundle = std::move(*preload_bundle)](
                const std::shared_ptr<TemplateEntry>& entry) -> bool {
              return entry->InitWithTemplateBundle(this, std::move(bundle));
            })) {
      DidComponentLoaded(entry);
      return entry;
    }
  }
  return nullptr;
}

std::shared_ptr<TemplateEntry> TemplateAssembler::RequestTemplateEntryInternal(
    std::unique_ptr<LazyBundleLifecycleOption> lifecycle_option,
    RadonLazyComponent* lazy_bundle) {
  /**
   * There could be three situations in which a request is sent.
   * 1. A sync request is sent, so results are immediately available
   * 2. An async request is sent, so no result is available
   * 3. No request is sent, because the previous request with the same url has
   * not yet been called back
   */
  if (component_loader_) {
    LazyBundleLoader::RequireScope scope{component_loader_, lazy_bundle};
    std::string url = lifecycle_option->component_url;
    component_loader_->AppendUrlToLifecycleOptionMap(
        url, std::move(lifecycle_option));
    // TODO(zhoupeng): no need to pass lazy_bundle pointer any more
    if (component_loader_->RequireTemplateCollected(lazy_bundle, url,
                                                    instance_id_)) {
      // situation 1 or 2, need to check the entry map again
      auto entry = FindTemplateEntry(url);
      if (entry != nullptr) {
        // situation 1, return the result
        return entry;
      }
    }
    // situation 2 or 3, make a mark
    component_loader_->MarkComponentLoading(url);
  }
  return nullptr;
}

std::shared_ptr<TemplateEntry> TemplateAssembler::QueryComponent(
    const std::string& url) {
  LOGI("QueryComponent from LEPUS: " << url);
  // check if the lazy-bundle is already loaded.
  auto entry = FindTemplateEntry(url);
  if (entry == nullptr && component_loader_ != nullptr) {
    component_loader_->RequireTemplate(nullptr, url, instance_id_);
  }
  // check if the lazy-bundle is loaded success.
  return FindTemplateEntry(url);
}

void TemplateAssembler::PreloadLazyBundles(
    const std::vector<std::string>& urls) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LazyBundle::Preload", "preload_urls",
              std::accumulate(urls.cbegin(), urls.cend(), std::string(", ")));
  if (component_loader_) {
    component_loader_->PreloadTemplates(urls);
  }
}

void TemplateAssembler::OnDynamicJSSourcePrepared(
    const std::string& component_url) {
  runtime::MessageEvent event(
      runtime::kMessageEventTypeOnDynamicJSSourcePrepared,
      runtime::ContextProxy::Type::kCoreContext,
      runtime::ContextProxy::Type::kJSContext, lepus::Value(component_url));
  delegate_.DispatchMessageEvent(std::move(event));
}

void TemplateAssembler::PrintMsgToJS(const std::string& level,
                                     const std::string& msg) {
  delegate_.PrintMsgToJS(level, msg);
  // Post msg to devtool when using LynxAir, which doesn't have js runtime.
  if (lepus_observer_ != nullptr) {
    lepus_observer_->OnConsoleMessage(level, msg);
  }
}

void TemplateAssembler::ExecuteDataProcessor(TemplateData& input) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "dataProcessor",
              [functionName =
                   input.PreprocessorName()](lynx::perfetto::EventContext ctx) {
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("processor_name");
                debug->set_string_value(functionName);
              });
  lepus::Value closure;
  auto isBuildInProcessor = [](const std::string& name) {
    static const std::unordered_set<std::string> protected_processor_names = {
        REACT_PRE_PROCESS_LIFECYCLE, REACT_ERROR_PROCESS_LIFECYCLE,
        REACT_SHOULD_COMPONENT_UPDATE, REACT_SHOULD_COMPONENT_UPDATE_KEY};
    return protected_processor_names.find(name) !=
           protected_processor_names.end();
  };

  const auto& functionName = input.PreprocessorName();
  if (functionName.empty()) {
    // Use default preprocessor
    closure = default_processor_;
  } else if (isBuildInProcessor(functionName)) {
    ReportError(
        error::E_DATA_FLOW_UPDATE_INVALID_PROCESSOR,
        "built-in function cannot be called as user registered processor: " +
            functionName);
  } else {
    closure = processor_with_name_[functionName];
  }

  if (closure.IsNil()) {
    tasm::TimingCollector::Instance()->MarkFrameworkTiming(
        tasm::timing::kDataProcessorEnd);
    return;
  }

  lepus::Value env = lepus::Value(lepus::Dictionary::Create());
  env.SetProperty(BASE_STATIC_STRING(kGlobalPropsKey), global_props_);
  env.SetProperty(BASE_STATIC_STRING(kSystemInfo), GenerateSystemInfo(nullptr));

  auto card = FindEntry(DEFAULT_ENTRY_NAME);
  input.SetValue(card->GetVm()->CallClosure(closure, input.GetValue(), env));

  return;
}

// TODO(fulei.bill) currently we just pending JS event dispatch
// we will load appservice in later MRs
void TemplateAssembler::OnJSPrepared(const std::string& url,
                                     const PipelineOptions& pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "TemplateAssembler::OnJSPrepared",
              "url", url);
  auto card = FindEntry(DEFAULT_ENTRY_NAME);
  delegate_.OnJSSourcePrepared(card->CreateTasmRuntimeBundle(),
                               lepus::Value::Clone(global_props_),
                               card->GetName(), GetPageDSL(),
                               GetBundleModuleMode(), url, pipeline_options);
}

bool TemplateAssembler::InnerTranslateResourceForTheme(
    std::string& ret, const std::string& res_id, const std::string& theme_key,
    bool isFinalFallback) {
  DCHECK(page_proxy_.themed().hasTransConfig &&
         page_proxy_.themed().currentTransMap && !res_id.empty());
  auto& mapRef = page_proxy_.themed().currentTransMap;
  auto mapSize = mapRef->size();
  for (unsigned int i = 0; i < mapSize; ++i) {
    auto& transMap = mapRef->at(i);
    auto targetRes =
        (isFinalFallback ? transMap.curFallbackRes_ : transMap.currentRes_);
    if (targetRes == nullptr) continue;
    if (!theme_key.empty()) {
      if (transMap.name_ != theme_key) {
        continue;
      }
    }
    auto resVal = targetRes->find(res_id);
    if (resVal == targetRes->end()) continue;
    ret = resVal->second;
    return true;
  }
  return false;
}

lepus::Value TemplateAssembler::GetI18nResources(
    const lepus::Value& locale, const lepus::Value& channel,
    const lepus::Value& fallback_url) {
  if (!locale.IsString()) {
    LOGE("GetI18NResources locale must be string");
    return lepus::Value();
  }
  if (!channel.IsString()) {
    LOGE("GetI18NResources channel must be string");
    return lepus::Value();
  }
  if (channel.IsEmpty()) {
    LOGE("GetI18NResources channel must not empty");
    return lepus::Value();
  }
  std::string channelVal;
  channelVal.append(channel.StdString()).append("__");
  updateLocale(locale, channel);
  channelVal.append(locale_);
  return i18n.GetData(this, channelVal, fallback_url.StdString());
}

void TemplateAssembler::updateLocale(const lepus::Value& locale,
                                     const lepus::Value& channel) {
  auto key_str = locale.String();
  if (!key_str.empty()) {
    locale_ = key_str.str();
  }
}

void TemplateAssembler::OnI18nResourceChanged(const std::string& new_data) {
  // step1: change getI18Resource return val
  delegate_.OnI18nResourceChanged(new_data);
  // step2: Send onI18nResourceReady to front-end
  auto arguments = lepus::CArray::Create();
  auto params = lepus::CArray::Create();
  auto dic = lepus::Dictionary::Create();
  params->emplace_back(std::move(dic));
  // name
  BASE_STATIC_STRING_DECL(kOnI18nResourceReady, "onI18nResourceReady");
  arguments->emplace_back(kOnI18nResourceReady);
  // params
  arguments->emplace_back(std::move(params));
  delegate_.CallJSFunction("GlobalEventEmitter", "emit",
                           lepus_value(std::move(arguments)));
}

void TemplateAssembler::OnI18nResourceFailed() {
  auto arguments = lepus::CArray::Create();
  auto params = lepus::CArray::Create();
  auto dic = lepus::Dictionary::Create();
  params->emplace_back(std::move(dic));
  // name
  BASE_STATIC_STRING_DECL(kOnI18nResourceFailed, "onI18nResourceFailed");
  arguments->emplace_back(kOnI18nResourceFailed);
  // params
  arguments->emplace_back(std::move(params));
  delegate_.CallJSFunction("GlobalEventEmitter", "emit",
                           lepus_value(std::move(arguments)));
}

void TemplateAssembler::UpdateI18nResource(const std::string& key,
                                           const std::string& new_data) {
  if (new_data.empty()) {
    OnI18nResourceFailed();
    return;
  }
  // should ensure that the i18n resource at both ends is set before sending
  // events to the front end set lepus i18n resource
  bool is_async = i18n.UpdateData(key, new_data);
  // set js i18n resource, and send 'onI18nResourceReady' event to js
  OnI18nResourceChanged(new_data);
  if (!is_async) {  // check if sync return
    auto param = lepus::Dictionary::Create();
    BASE_STATIC_STRING_DECL(kLocale, "locale");
    param->SetValue(kLocale, locale_);
    SendGlobalEventToLepus("i18nResourceReady", lepus::Value(std::move(param)));
  }
}

void TemplateAssembler::ReFlushPage() {
  // Case: i18n may call ReFlushPage while loadingTemplate, this may cause
  // render twice.
  if (is_loading_template_) {
    return;
  }
  UpdatePageOption update_page_option;
  update_page_option.update_first_time = true;
  page_proxy_.ForceUpdate(update_page_option);
}

void TemplateAssembler::FilterI18nResource(const lepus::Value& channel,
                                           const lepus::Value& locale,
                                           const lepus::Value& reserve_keys) {
  if (!locale.IsString()) {
    LOGE("FilterI18nResource, locale must be string");
    return;
  }
  if (!channel.IsString()) {
    LOGE("FilterI18nResource, channel must be string");
    return;
  }
  if (!reserve_keys.IsArrayOrJSArray()) {
    LOGE("FilterI18nResource, reserveKeys must be array");
    return;
  }
  std::string channelVal;
  channelVal.append(channel.StdString()).append("__");
  channelVal.append(locale.StdString());
  i18n.SetChannelConfig(channelVal, reserve_keys);
}

void TemplateAssembler::OnPageConfigDecoded(
    const std::shared_ptr<PageConfig>& config) {
  delegate_.OnPageConfigDecoded(config);
  report::EventTracker::UpdateGenericInfoByPageConfig(instance_id_, config);
}

bool TemplateAssembler::UseLepusNG() {
  if (!page_proxy()->HasSSRRadonPage()) {
    auto context = FindEntry(DEFAULT_ENTRY_NAME)->GetVm();
    return context ? context->IsLepusNGContext() : true;
  } else {
    return default_use_lepus_ng_;
  }
}

void TemplateAssembler::SetCSSVariables(const std::string& component_id,
                                        const std::string& id_selector,
                                        const lepus::Value& properties,
                                        PipelineOptions& pipeline_options) {
  page_proxy()->SetCSSVariables(component_id, id_selector, properties,
                                pipeline_options);
}

void TemplateAssembler::SetNativeProps(const NodeSelectRoot& root,
                                       const tasm::NodeSelectOptions& options,
                                       const lepus::Value& native_props,
                                       PipelineOptions& pipeline_options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateAssembler::SetNativeProps",
              [&native_props](lynx::perfetto::EventContext ctx) {
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("setNativeProps");
                /* Concatenate all the keys in updating data */
                std::string ss;
                ForEachLepusValue(native_props, [&ss](const lepus::Value& key,
                                                      const lepus::Value& val) {
                  ss.append(key.StdString())
                      .append(" , ")
                      .append(val.StdString())
                      .append("\n");
                });
                debug->set_string_value(ss);
              });
  auto elements = page_proxy_.SelectElements(root, options);
  for (auto ele : elements) {
    if (ele != nullptr) {
      ele->SetNativeProps(native_props, pipeline_options);
    }
  }
}

void TemplateAssembler::SendLazyBundleGlobalEvent(const std::string& url,
                                                  const lepus::Value& err) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "TemplateAssembler::SendLazyBundleGlobalEvent", "url", url);
  auto param = lepus::CArray::Create();
  param->push_back(err);
  auto arguments = lepus::CArray::Create();
  // name
  BASE_STATIC_STRING_DECL(kOnDynamicComponentEvent, "onDynamicComponentEvent");
  arguments->emplace_back(kOnDynamicComponentEvent);
  // params
  arguments->emplace_back(std::move(param));
  // for backward compatible (@nihao.royal)
  delegate_.CallJSFunction("GlobalEventEmitter", "emit",
                           lepus_value(std::move(arguments)));
}

void TemplateAssembler::SendLazyBundleBindEvent(const std::string& url,
                                                const std::string& event_name,
                                                const lepus::Value& msg,
                                                int imp_id) {
  EnsureTouchEventHandler();
  touch_event_handler_->HandleCustomEvent(this, event_name, imp_id, msg,
                                          lazy_bundle::kDetail);
}

#if ENABLE_TESTBENCH_RECORDER
void TemplateAssembler::SetRecordID(int64_t record_id) {
  record_id_ = record_id;
}
int64_t TemplateAssembler::GetRecordID() const { return record_id_; }
#endif

void TemplateAssembler::SetLepusEventListener(const std::string& name,
                                              const lepus::Value& listener) {
  lepus_event_listeners_[name] = listener;
}

void TemplateAssembler::RemoveLepusEventListener(const std::string& name) {
  auto iter = lepus_event_listeners_.find(name);
  if (iter != lepus_event_listeners_.end()) {
    lepus_event_listeners_.erase(iter);
  }
}

std::pair<ComponentMould*, std::string> TemplateAssembler::FindComponentMould(
    const std::string& entry_name, const std::string& component_name, int tid) {
  auto component_mould_finder =
      [this, tid](const std::string& entry_name) -> ComponentMould* {
    ComponentMould* component_mould = nullptr;
    auto component_moulds = FindEntry(entry_name)->component_moulds();
    auto cm_it = component_moulds.find(tid);
    if (cm_it != component_moulds.end()) {
      component_mould = cm_it->second.get();
    }
    return component_mould;
  };

  ComponentMould* mould = nullptr;
  if (DEFAULT_ENTRY_NAME != entry_name) {
    // if entry is lazy bundle, find in it's moulds first
    mould = component_mould_finder(entry_name);
    if (mould && mould->name() == component_name) {
      return std::make_pair(mould, entry_name);
    }
  }
  mould = component_mould_finder(DEFAULT_ENTRY_NAME);
  return std::make_pair(mould, DEFAULT_ENTRY_NAME);
}

void TemplateAssembler::SendGlobalEventToLepus(const std::string& name,
                                               const lepus_value& params) {
  auto iter = lepus_event_listeners_.find(name);
  if (iter == lepus_event_listeners_.end() || !iter->second.IsCallable()) {
    return;
  }
  lepus::Value closure = iter->second;
  TriggerLepusClosure(closure, params);
}

void TemplateAssembler::TriggerEventBus(const std::string& name,
                                        const lepus_value& params) {
  // lynx air supports lepus
  if (!UseLepusNG() && !EnableLynxAir()) {
    return;
  }

  BASE_STATIC_STRING_DECL(kGlobalEventEmitter, "GlobalEventEmitter");
  BASE_STATIC_STRING_DECL(kToggle, "toggle");
  BASE_STATIC_STRING_DECL(kCallLepusModuleMethod, "callLepusModuleMethod");
  ForEachEntry([&kGlobalEventEmitter, &kToggle, &kCallLepusModuleMethod, &name,
                &params](const auto& entry) {
    if (entry->GetVm()) {
      entry->GetVm()->Call(kCallLepusModuleMethod,
                           lepus::Value(kGlobalEventEmitter),
                           lepus::Value(kToggle), lepus::Value(name), params);
    }
  });
}

void TemplateAssembler::RenderToBinary(
    base::MoveOnlyClosure<void, RadonNode*, tasm::TemplateAssembler*>
        binarizer) {
  page_proxy_.RenderToBinary(binarizer, this);
}

TemplateData TemplateAssembler::ProcessTemplateData(
    const std::shared_ptr<TemplateData>& template_data, bool is_first_screen) {
  tasm::TimingCollector::Instance()->MarkFrameworkTiming(
      tasm::timing::kDataProcessorStart);
  TemplateData data;

  if (EnableDataProcessorOnJs()) {
    if (template_data == nullptr) {
      data = TemplateData();
    } else {
      data = TemplateData(template_data->value(), template_data->IsReadOnly(),
                          template_data->PreprocessorName());
      data.SetPlatformData(template_data->ObtainPlatformData());
    }

    // When JS DataProcessor is enabled, ProcessInitData & ProcessTemplateData
    // don’t really execute and return immediately, which results in the loss of
    // timing profiling points. To fix this issue, ensure that the timing
    // profiling points are recorded before returning.
    tasm::TimingCollector::Instance()->MarkFrameworkTiming(
        tasm::timing::kDataProcessorEnd);

    return data;
  }

  if (EnableFiberArch()) {
    data = ProcessTemplateDataForFiber(template_data, is_first_screen);
  } else {
    data = ProcessTemplateDataForRadon(template_data, is_first_screen);
  }

  tasm::TimingCollector::Instance()->MarkFrameworkTiming(
      tasm::timing::kDataProcessorEnd);

  return data;
}

TemplateData TemplateAssembler::ProcessTemplateDataForFiber(
    const std::shared_ptr<TemplateData>& template_data, bool is_first_screen) {
  TemplateData data;
  data.SetReadOnly(template_data ? template_data->IsReadOnly() : false);

  // Call processData function with template data and processor name. If the
  // result is object, let data be the result. Otherwise, let data be template
  // data.
  const auto& res =
      FindEntry(tasm::DEFAULT_ENTRY_NAME)
          ->GetVm()
          ->Call(BASE_STATIC_STRING(kProcessData),
                 template_data ? template_data->GetValue() : lepus::Value(),
                 template_data ? lepus::Value(template_data->PreprocessorName())
                               : lepus::Value(base::String()));
  if (res.IsObject()) {
    data.SetValue(res);
  } else if (template_data) {
    data.SetValue(template_data->GetValue());
  }

  return data;
}

TemplateData TemplateAssembler::ProcessTemplateDataForRadon(
    const std::shared_ptr<TemplateData>& template_data, bool is_first_screen) {
  TemplateData data;
  data.SetReadOnly(false);

  std::string processor_name;
  if (template_data != nullptr || global_props_.IsObject() ||
      !page_proxy_.GetDefaultPageData().IsEmpty()) {
    if (template_data != nullptr) {
      data.SetValue(template_data->GetValue());
      data.SetPreprocessorName(template_data->PreprocessorName());
      data.SetReadOnly(template_data->IsReadOnly());
    } else {
      data.SetValue(lepus::Value::CreateObject());
    }

    if (!page_proxy_.GetDefaultPageData().IsEmpty()) {
      if (data.GetValue().IsEmpty()) {
        data.SetValue(lepus::Value::CreateObject());
      }
      ForEachLepusValue(
          page_proxy_.GetDefaultPageData(),
          [&data](const lepus::Value& key, const lepus::Value& value) {
            auto key_str = key.String();
            if (data.GetValue().GetProperty(key_str).IsEmpty()) {
              data.value().SetProperty(key_str, value);
            }
          });
    }
    if (page_proxy_.HasSSRRadonPage()) {
      // TODO(zhixuan): Support diff global props for ssr.
      page_proxy_.DiffHydrationData(data.GetValue());
    }

    // Only exec the following code on first screen.
    // In PrePainting Mode, globalProps should also be attached to avoid bk
    is_first_screen |= pre_painting_;
    if (!page_proxy_.GetEnableRemoveComponentExtraData() &&
        global_props_.IsObject() && is_first_screen) {
      // Backward Compatible, should be deleted later. (@nihao.royal)
      // globalProps should be visited through the second param in DataProcessor
      data.value().SetProperty(BASE_STATIC_STRING(kGlobalPropsKey),
                               global_props_);
    }
    ExecuteDataProcessor(data);
  }

  return data;
}

void TemplateAssembler::RenderPageWithSSRData(
    std::vector<uint8_t> ssr_byte_array,
    const std::shared_ptr<TemplateData>& template_data,
    PipelineOptions& pipeline_options) {
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::TemplateAssemblerRecorder::RecordLoadTemplate(
      "", ssr_byte_array, template_data, record_id_, false);
  auto& client = page_proxy_.element_manager();
  if (client != nullptr) {
    client->SetRecordId(record_id_);
  }
#endif

  LOGI("start TemplateAssembler::RenderPageWithSSRData, this:"
       << this << ", url:" << url_);
  tasm::TimingCollector::Instance()->Mark(tasm::timing::kRenderPageStartSSR);

  Scope scope(this);
  page_proxy_.ResetHydrateInfo();

  if (!ssr::SSRRenderUtils::DecodeSSRData(this, std::move(ssr_byte_array),
                                          template_data, pipeline_options,
                                          GetInstanceId())) {
    return;
  }

  template_loaded_ = true;

  auto card = FindEntry(tasm::DEFAULT_ENTRY_NAME);
  if (card->GetVm() && card->GetVm()->IsVMContext()) {
    card->SetVm(nullptr);
  }

  tasm::TimingCollector::Instance()->Mark(tasm::timing::kRenderPageEndSSR);
  LOGI("end TemplateAssembler::RenderPageWithSSRData, this:" << this << ", url:"
                                                             << url_);
}

Themed& TemplateAssembler::Themed() { return page_proxy_.themed(); }

void TemplateAssembler::SetThemed(
    const Themed::PageTransMaps& page_trans_maps) {
  Themed().ResetWithPageTransMaps(page_trans_maps);
}

// For fiber
void TemplateAssembler::CallLepusMethod(const std::string& method_name,
                                        lepus::Value args,
                                        const piper::ApiCallBack& callback,
                                        uint64_t trace_flow_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TemplateAssembler::CallLepusMethod",
              [&](perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(callback.trace_flow_id());
                ctx.event()->add_terminating_flow_ids(trace_flow_id);
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("methodName");
                debug->set_string_value(method_name);
              });

  Scope scope(this);
  LOGI("TemplateAssembler::CallLepusMethod. this:"
       << this << " url:" << url_ << " method name: " << method_name);

  const auto ret_value =
      context(tasm::DEFAULT_ENTRY_NAME)->Call(method_name, args);
  if (callback.IsValid()) {
    delegate_.CallJSApiCallbackWithValue(callback, ret_value);
  }
}

void TemplateAssembler::SetWhiteBoard(
    const std::shared_ptr<WhiteBoard>& white_board) {
  white_board_delegate_ =
      std::make_shared<WhiteBoardTasmDelegate>(this, white_board);
}

lepus::Value TemplateAssembler::TriggerLepusClosure(
    const lepus::Value& closure, const lepus::Value& params) {
  if (!closure.IsCallable()) {
    return lepus::Value();
  }
  auto card = FindEntry(DEFAULT_ENTRY_NAME);
  if (params.IsArrayOrJSArray()) {
    std::vector<lepus::Value> params_vec;
    size_t size = params.Array()->size();
    for (size_t i = 0; i < size; ++i) {
      params_vec.push_back(params.GetProperty(static_cast<int>(i)));
    }
    return card->GetVm()->CallClosureArgs(closure, params_vec);
  } else {
    return card->GetVm()->CallClosure(closure, params);
  }
}

void TemplateAssembler::SendAirPageEvent(const std::string& event,
                                         const lepus::Value& value) {
#if ENABLE_AIR
  if (EnableLynxAir()) {
    EnsureAirTouchEventHandler();
    air_touch_event_handler_->SendPageEvent(this, event, value);
  }
#endif
}

void TemplateAssembler::InvokeAirCallback(int64_t id,
                                          const std::string& entry_name,
                                          const lepus::Value& data) {
  auto card = FindEntry(DEFAULT_ENTRY_NAME);
  auto callback_manager = card->GetVm()->GetCallbackManager();
  callback_manager->InvokeTask(id, data);
}

void TemplateAssembler::DumpElementTree(
    const std::shared_ptr<TemplateEntry>& card) {
  auto template_bundle = card->GetCompleteTemplateBundle();
  auto& element_manager = page_proxy_.element_manager();
  if (template_bundle && element_manager) {
    auto page_node = lepus::Value(TreeResolver::CloneElementRecursively(
        element_manager->GetPageElement(), true));
    ElementBundle element_bundle = ElementBundle(std::move(page_node));
    template_bundle->SetElementBundle(std::move(element_bundle));
    delegate_.OnTemplateBundleReady(std::move(*template_bundle));
    page_proxy_.element_manager()->SetEnableDumpElementTree(false);
  }
}

void TemplateAssembler::SyncAndroidPackageExternalPath(
    const std::string& path) {
  android_package_external_path = path;
}

void TemplateAssembler::OnReceiveMessageEvent(runtime::MessageEvent event) {
  // TODO(songshourui.null): If LynxRuntime is standalone in the future, there
  // might be situations where LynxRuntime is initialized earlier than
  // TemplateAssembler, causing TemplateAssembler to not be initialized yet and
  // leading to event loss. A possible solution could be to implement a global
  // task cache, similar to the one in LynxRuntime.
  auto proxy = GetContextProxy(event.GetOriginType());
  if (proxy == nullptr) {
    return;
  }
  proxy->DispatchEvent(event);
}

ContextProxyInLepus* TemplateAssembler::GetContextProxy(
    runtime::ContextProxy::Type type) {
  if (type >= runtime::ContextProxy::Type::kUnknown ||
      type < runtime::ContextProxy::Type::kJSContext) {
    return nullptr;
  }
  auto index = static_cast<int32_t>(type);
  if (context_proxy_vector_[index] == nullptr) {
    context_proxy_vector_[index] =
        std::make_unique<ContextProxyInLepus>(delegate_, type);
  }
  return context_proxy_vector_[index].get();
}

lepus::Value TemplateAssembler::GetCustomSection(const std::string& key) {
  // TODO(zhoupeng.z): support to get custom section from lazy bundles
  return FindEntry(tasm::DEFAULT_ENTRY_NAME)->GetCustomSection(key);
}

void TemplateAssembler::OnNativeAppReady() {
  runtime::MessageEvent event(runtime::kMessageEventTypeOnNativeAppReady,
                              runtime::ContextProxy::Type::kCoreContext,
                              runtime::ContextProxy::Type::kJSContext,
                              lepus::Value());
  delegate_.DispatchMessageEvent(std::move(event));
}

bool TemplateAssembler::LoadTemplateForSSRRuntime(std::vector<uint8_t> source) {
  source_size_ = source.size();
  url_ = "";

  // load page config and style sheet for ssr runtime.
  auto card_entry = FindEntry(DEFAULT_ENTRY_NAME);
  if (!FromBinary(card_entry, std::move(source))) {
    return false;
  }

  SetPageConfigClient();
  return true;
}

}  // namespace tasm
}  // namespace lynx
