// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/mts_renderer.h"

#include <utility>

#include "base/include/log/logging.h"
#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/pipeline/pipeline_context.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/base/tasm_utils.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/js/runtime_constant.h"
#include "core/runtime/lepus/vm_context.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"

namespace lynx {
namespace tasm {

class FiberMTSRenderer final : public MTSRenderer {
 public:
  explicit FiberMTSRenderer(TemplateAssembler& tasm) : MTSRenderer(tasm) {}

  TemplateData ProcessData(const std::shared_ptr<TemplateData>& template_data,
                           bool is_first_screen) override {
    TemplateData data;
    data.SetReadOnly(template_data ? template_data->IsReadOnly() : false);

    const auto& result =
        tasm_.FindEntry(DEFAULT_ENTRY_NAME)
            ->GetVm()
            ->Call(BASE_STATIC_STRING(kProcessData),
                   template_data ? template_data->GetValue() : lepus::Value(),
                   template_data
                       ? lepus::Value(template_data->PreprocessorName())
                       : lepus::Value(base::String()));
    if (result.IsObject()) {
      data.SetValue(result);
    } else if (template_data) {
      data.SetValue(template_data->GetValue());
    }

    return data;
  }

  void Load(const std::shared_ptr<TemplateEntry>& entry,
            const TemplateData& data,
            std::shared_ptr<PipelineOptions>& pipeline_options) override {
    TimingCollector::Instance()->Mark(timing::kCreateVdomStart);
    pipeline_options->is_first_screen = true;

    lepus::Value render_options(lepus::Dictionary::Create());
    if (tasm_.EnableDataProcessorOnJs()) {
      auto processor_name_key = BASE_STATIC_STRING(kProcessorName);
      render_options.SetProperty(processor_name_key,
                                 lepus::Value(data.PreprocessorName()));
      if (!tasm_.cache_data_.empty()) {
        auto data_key = BASE_STATIC_STRING(kData);
        auto cache_data_key = BASE_STATIC_STRING(kCacheData);
        auto cache_data = lepus::CArray::Create();
        for (const auto& cached_data : tasm_.cache_data_) {
          lepus::Value data_object(lepus::Dictionary::Create());
          data_object.SetProperty(data_key, cached_data->GetValue());
          data_object.SetProperty(
              processor_name_key,
              lepus::Value(cached_data->PreprocessorName()));
          cache_data->emplace_back(std::move(data_object));
        }
        render_options.SetProperty(cache_data_key,
                                   lepus::Value(std::move(cache_data)));
      }
    }
    render_options.SetProperty(BASE_STATIC_STRING(kPreLoadTemplate),
                               lepus::Value(tasm_.pre_painting_));
    render_options.SetProperty(BASE_STATIC_STRING(kPipelineOptions),
                               PipelineOptionsToLepusValue(pipeline_options));

    fml::RefPtr<Element> element_cache = entry->TryToGetElementCache();
    if (element_cache.get()) {
      TreeResolver::AttachRootToElementManager(
          element_cache, tasm_.page_proxy()->element_manager().get(),
          tasm_.style_sheet_manager(DEFAULT_ENTRY_NAME), true);
      render_options.SetProperty(BASE_STATIC_STRING(kInitPage),
                                 lepus::Value(element_cache));
    }

    if (!tasm_.page_proxy_.IsWaitingSSRHydrate()) {
      auto& context = entry->GetVm();
      tasm_.DispatchEventFromEngineToCoreContext(
          context, kRenderPage, runtime::kMessageEventTypeRenderPage,
          data.GetValue(), std::move(render_options));
    } else {
      tasm_.page_proxy()->element_manager()->ClearExtremeParsedStyles();
      if (tasm_.page_proxy()->HydrateByRootPage()) {
        auto css_manager =
            tasm_.FindEntry(DEFAULT_ENTRY_NAME)->GetStyleSheetManager();
        auto* page_element =
            tasm_.page_proxy()->element_manager()->GetPageElement();
        if (page_element) {
          page_element->ResetSheetRecursively(css_manager);
        }

        auto& context = entry->GetVm();
        auto page_ref =
            tasm_.page_proxy()->element_manager()->GetPageElementRef();
        render_options.SetProperty(BASE_STATIC_STRING(kInitPage),
                                   lepus::Value(page_ref));
        tasm_.DispatchEventFromEngineToCoreContext(
            context, kRenderPage, runtime::kMessageEventTypeRenderPage,
            data.GetValue(), std::move(render_options));
      }
    }

    TimingCollector::Instance()->Mark(timing::kCreateVdomEnd);
    TimingCollector::Instance()->Mark(timing::kMtsRenderEnd);

    tasm_.HandleSimpleStyleFontFaces(entry);
    tasm_.HandleSimpleStyleKeyframes(entry);

    if (pipeline_options->enable_unified_pixel_pipeline) {
      tasm_.GetCurrentPipelineContext()->RequestResolve();
    } else {
      tasm_.page_proxy()->element_manager()->OnPatchFinish(pipeline_options);
      if (tasm_.page_proxy()->element_manager()->GetEnableDumpElementTree()) {
        tasm_.DumpElementTree(entry);
      }
    }
  }

  void UpdateMetaData(
      const TemplateData* data, const lepus::Value* global_props,
      bool global_props_need_render, const UpdatePageOption& update_page_option,
      std::shared_ptr<PipelineOptions>& pipeline_options) override {
    if (data != nullptr && global_props != nullptr) {
      auto& context = tasm_.FindEntry(DEFAULT_ENTRY_NAME)->GetVm();
      if (context == nullptr) {
        LOGE("FiberMTSRenderer::UpdateMetaData: context is null");
        return;
      }
      auto options = update_page_option.ToLepusValue();
      tasm_.DispatchEventFromEngineToCoreContext(
          context, kUpdateMetaData, kUpdateMetaData, data->GetValue(),
          *global_props, std::move(options));
      return;
    }

    if (global_props != nullptr) {
      if (!tasm_.template_loaded_) {
        return;
      }
      auto& context = tasm_.FindEntry(DEFAULT_ENTRY_NAME)->GetVm();
      if (context == nullptr) {
        LOGE("FiberMTSRenderer::UpdateMetaData: context is null");
        return;
      }
      tasm_.DispatchEventFromEngineToCoreContext(
          context, kUpdateGlobalProps,
          runtime::kMessageEventTypeUpdateGlobalProps, *global_props);
    }

    if (data == nullptr) {
      return;
    }

    if (update_page_option.reload_template ||
        pipeline_options->need_timestamps) {
      TimingCollector::Instance()->Mark(timing::kCreateVdomStart);
    }

    auto options = update_page_option.ToLepusValue();
    options.SetProperty(BASE_STATIC_STRING(kPipelineOptions),
                        PipelineOptionsToLepusValue(pipeline_options));
    if (tasm_.pre_painting_) {
      options.SetProperty(BASE_STATIC_STRING(kTriggerLifeCycle),
                          lepus::Value(true));
    }
    if (tasm_.EnableDataProcessorOnJs()) {
      options.SetProperty(BASE_STATIC_STRING(kProcessorName),
                          lepus::Value(data->PreprocessorName()));
    }

    auto& context = tasm_.FindEntry(DEFAULT_ENTRY_NAME)->GetVm();
    tasm_.DispatchEventFromEngineToCoreContext(
        context, kUpdatePage, runtime::kMessageEventTypeUpdatePage,
        data->GetValue(), std::move(options));

    if (!update_page_option.reload_template &&
        pipeline_options->need_timestamps) {
      TimingCollector::Instance()->Mark(timing::kCreateVdomEnd);
      TimingCollector::Instance()->Mark(timing::kMtsRenderEnd);
    }
  }

  bool CanUpdateMetaDataAtomically() override {
    if (!tasm_.template_loaded_ ||
        !LynxEnv::GetInstance().EnableFiberUpdateMetaData()) {
      return false;
    }
    auto* engine_context_proxy =
        tasm_.GetContextProxy(runtime::ContextProxy::Type::kEngine);
    return engine_context_proxy != nullptr &&
           engine_context_proxy->HasEventListener(kUpdateMetaData);
  }

  void Reset(const std::shared_ptr<TemplateEntry>& entry) override {
    if (entry && entry->GetVm()) {
      auto& context = entry->GetVm();
      tasm_.DispatchEventFromEngineToCoreContext(
          context, kRemoveComponents,
          runtime::kMessageEventTypeRemoveComponents);
    }
  }
};

class RadonMTSRenderer final : public MTSRenderer {
 public:
  explicit RadonMTSRenderer(TemplateAssembler& tasm) : MTSRenderer(tasm) {}

  TemplateData ProcessData(const std::shared_ptr<TemplateData>& template_data,
                           bool is_first_screen) override {
    TemplateData data;
    data.SetReadOnly(false);

    if (template_data != nullptr || tasm_.global_props_.IsObject() ||
        !tasm_.page_proxy_.GetDefaultPageData().IsEmpty()) {
      if (template_data != nullptr) {
        data.SetValue(template_data->GetValue());
        data.SetPreprocessorName(template_data->PreprocessorName());
        data.SetReadOnly(template_data->IsReadOnly());
      } else {
        data.SetValue(lepus::Value(lepus::Dictionary::Create()));
      }

      if (!tasm_.page_proxy_.GetDefaultPageData().IsEmpty()) {
        if (data.GetValue().IsEmpty()) {
          data.SetValue(lepus::Value(lepus::Dictionary::Create()));
        }
        ForEachLepusValue(
            tasm_.page_proxy_.GetDefaultPageData(),
            [&data](const lepus::Value& key, const lepus::Value& value) {
              auto key_string = key.String();
              if (data.GetValue().GetProperty(key_string).IsEmpty()) {
                data.value().SetProperty(key_string, value);
              }
            });
      }
      if (tasm_.page_proxy_.HasSSRRadonPage()) {
        tasm_.page_proxy_.DiffHydrationData(data.GetValue());
      }

      is_first_screen |= tasm_.pre_painting_;
      if (!tasm_.page_proxy_.GetEnableRemoveComponentExtraData() &&
          tasm_.global_props_.IsObject() && is_first_screen) {
        data.value().SetProperty(BASE_STATIC_STRING(kGlobalPropsKey),
                                 tasm_.global_props_);
      }
      tasm_.ExecuteDataProcessor(data);
    }

    return data;
  }

  void Load(const std::shared_ptr<TemplateEntry>& entry,
            const TemplateData& data,
            std::shared_ptr<PipelineOptions>& pipeline_options) override {
    UpdatePageOption update_page_option;
    update_page_option.update_first_time = true;
    tasm_.page_proxy_.UpdateInLoadTemplate(data.GetValue(), update_page_option,
                                           pipeline_options);
  }

  void UpdateMetaData(
      const TemplateData* data, const lepus::Value* global_props,
      bool global_props_need_render, const UpdatePageOption& update_page_option,
      std::shared_ptr<PipelineOptions>& pipeline_options) override {
    if (global_props != nullptr) {
      tasm_.ForEachEntry(
          [default_entry = tasm_.FindEntry(DEFAULT_ENTRY_NAME).get(),
           global_props](const auto& entry) {
            if (entry.get() != default_entry) {
              entry->GetVm()->UpdateTopLevelVariable(kGlobalPropsKey,
                                                     *global_props);
            }
          });

      bool should_render = global_props_need_render && tasm_.template_loaded_ &&
                           !tasm_.page_proxy_.IsServerSideRendering();
      tasm_.page_proxy_.UpdateGlobalProps(*global_props, should_render,
                                          pipeline_options);
    }

    if (data != nullptr &&
        tasm_.UpdateGlobalDataInternal(data->GetValue(), update_page_option,
                                       pipeline_options)) {
      if ((update_page_option.from_native &&
           !update_page_option.reload_template &&
           !update_page_option.reload_from_js) ||
          (update_page_option.from_native &&
           update_page_option.reset_page_data)) {
        tasm_.delegate_.OnDataUpdated();
      }
    }
  }

  bool CanUpdateMetaDataAtomically() override { return false; }

  void Reset(const std::shared_ptr<TemplateEntry>& entry) override {
    tasm_.page_proxy_.RemoveOldComponentBeforeReload();
  }
};

std::unique_ptr<MTSRenderer> CreateMTSRenderer(TemplateAssembler& tasm,
                                               bool enable_fiber_arch) {
  if (enable_fiber_arch) {
    return std::make_unique<FiberMTSRenderer>(tasm);
  }
  return std::make_unique<RadonMTSRenderer>(tasm);
}

}  // namespace tasm
}  // namespace lynx
