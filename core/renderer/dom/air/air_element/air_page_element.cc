// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/air/air_element/air_page_element.h"

#include <utility>

#include "core/renderer/dom/air/air_element/air_component_element.h"
#include "core/renderer/dom/air/air_element/air_for_element.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/value_utils.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"

namespace lynx {
namespace tasm {

namespace {
constexpr static const char *kOnDataChanged = "onDataChanged";
}  // namespace

bool AirPageElement::UpdatePageData(const lepus::Value &table,
                                    const UpdatePageOption &update_page_option,
                                    PipelineOptions &pipeline_options) {
  auto array = lepus::CArray::Create();
  bool need_update = false;
  lepus::Value new_data;
  if (update_page_option.reload_template ||
      update_page_option.reset_page_data) {
    // reset to default Card data, then update by table
    new_data = lepus::Value::Clone(init_data_);
    tasm::ForEachLepusValue(
        table, [&new_data](const lepus::Value &key, const lepus::Value &value) {
          new_data.SetProperty(key.String(), value);
        });

  } else {
    new_data = table;
  }

  if (update_page_option.update_first_time) {
    tasm::ForEachLepusValue(
        new_data,
        [&data = data_](const lepus::Value &key, const lepus::Value &value) {
          data.SetProperty(key.String(), value);
        });
  } else {
    // find removed keys and set corresponding data to undefined when resetData
    // or reloadTemplate
    auto *tasm = static_cast<TemplateAssembler *>(context_->GetDelegate());
    if (tasm &&
        tasm->GetPageConfig()->GetEnableAirDetectRemovedKeysWhenUpdateData() &&
        (update_page_option.reload_template ||
         update_page_option.reset_page_data)) {
      ForEachLepusValue(
          data_, [&data = data_, &new_data, &array](const lepus::Value &key,
                                                    const lepus::Value &value) {
            auto key_str = key.String();
            if (!new_data.Contains(key_str) && !value.IsEmpty()) {
              if ((key_str.str() != kGlobalPropsKey) &&
                  (key_str.str() != kSystemInfo)) {
                lepus::Value data_value = data.GetProperty(key_str);
                data_value.SetUndefined();
                array->push_back(key);
                data.SetProperty(key_str, data_value);
              }
            }
          });
    }

    tasm::ForEachLepusValue(
        new_data, [this, &need_update, array](const lepus::Value &key,
                                              const lepus::Value &value) {
          auto key_str = key.String();
          lepus::Value ret = data_.GetProperty(key_str);
          if (!ret.IsEmpty()) {
            if (CheckTableShadowUpdated(ret, value) ||
                value.GetLength() != ret.GetLength()) {
              array->push_back(key);
              data_.SetProperty(key_str, value);
              need_update = true;
            }
          } else {
            array->push_back(key);
            data_.SetProperty(key_str, value);
            need_update = true;
          }
        });
  }

  if (need_update) {
    const auto &timing_flag = tasm::GetTimingFlag(table);
    if (!timing_flag.empty()) {
      pipeline_options.need_timestamps = true;
      static_cast<TemplateAssembler *>(context_->GetDelegate())
          ->GetDelegate()
          .BindPipelineIDWithTimingFlag(pipeline_options.pipeline_id,
                                        timing_flag);
    }
    tasm::TimingCollector::Scope<TemplateAssembler::Delegate> scope(
        &(static_cast<TemplateAssembler *>(context_->GetDelegate())
              ->GetDelegate()),
        pipeline_options);
    // UpdatePage0 for first screen is called in
    // TemplateAssembler::RenderTemplateForAir
    if (pipeline_options.need_timestamps) {
      element_manager()->painting_context()->MarkUIOperationQueueFlushTiming(
          timing::kPaintingUiOperationExecuteStart,
          pipeline_options.pipeline_id);
    }

    if (!update_page_option.update_first_time &&
        !update_page_option.from_native) {
      if (pipeline_options.need_timestamps) {
        tasm::TimingCollector::Instance()->Mark(timing::kSetStateTrigger);
      }
    }
    if (pipeline_options.need_timestamps) {
      tasm::TimingCollector::Instance()->Mark(timing::kRefreshPageStartAir);
    }

    lepus::Value params(std::move(array));
    lepus::Value page_id(element_manager()->AirRoot()->impl_id());
    BASE_STATIC_STRING_DECL(kUpdatePage0, "$updatePage0");
    lepus::Value ret = context_->Call(kUpdatePage0, params, data_, page_id);
    // In some cases, some elements may fail to execute the flush operation
    // due to exceptions in the execution of the lepus code. As a result,
    // layout and other UI operations are not safe.
    if (!(ret.IsBool() && ret.Bool())) {
      return false;
    }
    if (pipeline_options.need_timestamps) {
      tasm::TimingCollector::Instance()->Mark(timing::kRefreshPageEndAir);
    }

    LOGI("lynx_air, UpdatePageData, first_time="
         << update_page_option.update_first_time);
    // trigger lifecycle event
    if (!update_page_option.update_first_time &&
        update_page_option.from_native) {
      auto *tasm = static_cast<TemplateAssembler *>(context_->GetDelegate());
      if (tasm) {
        tasm->SendAirPageEvent(kOnDataChanged, lepus_value());
      }
    }
    SetFontFaces();

    element_manager()->OnPatchFinishInnerForAir(pipeline_options);
  }
  return true;
}

bool AirPageElement::RefreshWithGlobalProps(const lynx::lepus::Value &table,
                                            bool should_render) {
  context_->Call(BASE_STATIC_STRING(kUpdateGlobalProps), table, data_);
  return true;
}

void AirPageElement::DeriveFromMould(ComponentMould *data) {
  if (data == nullptr) {
    return;
  }
  auto init_data = data->data();
  if (!init_data.IsObject()) {
    return;
  }

  if (context_->IsLepusNGContext()) {
    init_data_ = lepus::Value(
        context_->context(), data->data().ToJSValue(context_->context(), true));
  } else {
    init_data_ = std::move(init_data);
  }

  data_ = lepus::Value::Clone(init_data_, context_->IsLepusNGContext());

  // make sure the data is table
  if (!data_.IsObject()) {
    data_ = lepus::Value::CreateObject(context_);
  }
}

lepus::Value AirPageElement::GetData() { return lepus::Value(data_); }

lepus::Value AirPageElement::GetPageDataByKey(
    const std::vector<std::string> &keys) {
  lepus::Value result = lepus::Value(lepus::Dictionary::Create());
  std::for_each(keys.cbegin(), keys.cend(),
                [&result, &data = data_](const std::string &key) {
                  result.Table()->SetValue(key, data.GetProperty(key));
                });
  return result;
}

uint64_t AirPageElement::GetKeyForCreatedElement(uint32_t lepus_id) {
  uint64_t key = static_cast<uint64_t>(lepus_id);
  auto *for_element = this->GetCurrentForElement();
  auto *component_element = this->GetCurrentComponentElement();
  // If both the for element and component element are not null, this means that
  // the current element is both under the for and component. Check which is
  // 'closer', and use the closer element to determine the unique key. According
  // to the generation order, the later the element is created, the greater
  // lepus id.
  // For for element, use the unique id and active index to compute the unique
  // key. The unique id and the active index of for element are both 32 bits.
  // Shift the unique id and then perform a bitwise OR operation on active
  // index. This will generate a unique key. Compared to using strings as key,
  // computation of numbers is much more efficient. For component, just use the
  // unique id as the key.
  static const uint8_t shift = 32;
  if (for_element && component_element) {
    auto for_lepus_id = for_element->GetLepusId();
    auto comp_lepus_id = component_element->GetLepusId();
    int for_impl_id = for_element->impl_id();
    int comp_impl_id = component_element->impl_id();
    if (for_lepus_id > comp_lepus_id) {
      key = static_cast<uint64_t>(for_impl_id) << shift |
            static_cast<uint64_t>(for_element->ActiveIndex());
    } else {
      key = static_cast<uint64_t>(comp_impl_id);
    }
  } else if (for_element) {
    key = static_cast<uint64_t>(for_element->impl_id()) << shift |
          static_cast<uint64_t>(for_element->ActiveIndex());
  } else if (component_element) {
    key = static_cast<uint64_t>(component_element->impl_id());
  }
  return key;
}

void AirPageElement::FireComponentLifeCycleEvent(const std::string &name,
                                                 int component_id) {
  auto *tasm = static_cast<TemplateAssembler *>(context_->GetDelegate());
  tasm->SendAirComponentEvent(name, component_id, lepus::Value(), "");
}

void AirPageElement::FlushRecursively() {
  // style calc async only for first screen
  if (EnableAsyncCalc()) {
    UpdateFirstScreenListState();
    SetEnableAsyncCalc(false);
  }
  AirElement::FlushRecursively();
}

void AirPageElement::AppendLastElement() {
  if (!last_element_) {
    return;
  }
  {
    std::lock_guard<std::mutex> guard(first_screen_list_mutex_);
    first_screen_list_.emplace_back(last_element_);
    last_element_ = nullptr;
    ui_thread_cursor_++;
  }
  if (async_thread_cursor_ == ui_thread_cursor_ - 1) {
    CalcStyleAsync();
  }
}

AirElement *AirPageElement::GetNextElementForAsyncThread() {
  AirElement *element = nullptr;
  if (async_thread_cursor_ < ui_thread_cursor_) {
    std::lock_guard<std::mutex> guard(first_screen_list_mutex_);
    element = first_screen_list_[async_thread_cursor_ + 1];
  }
  return element;
}

void AirPageElement::CalcStyleAsync() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::CalcStyleAsync");
  static base::NoDestructor<fml::Thread> worker_thread("Lynx_Air_Async_Calc");
  // When the asynchronous thread is busy, we do not post new tasks.
  if (async_thread_cursor_ == ui_thread_cursor_ - 1) {
    worker_thread->GetTaskRunner()->PostTask([this]() {
      while (async_thread_cursor_ < ui_thread_cursor_) {
        AirElement *next = GetNextElementForAsyncThread();
        if (next) {
          next->CalcStyle();
          async_thread_cursor_++;
        }
      }
    });
  }
}

void AirPageElement::UpdateFirstScreenListState() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirElement::UpdateFirstScreenListState");
  long index = 0;
  if (last_element_) {
    AppendLastElement();
  }
  // The asynchronous thread is running, the main thread flush props which are
  // calculated by asynchronous thread
  for (; index <= async_thread_cursor_; ++index) {
    const auto &element = first_screen_list_[index];
    if (element->state_ & ElementState::kStyleCalculated) {
      element->FlushPropsResolveStyles(false);
    }
  }
  // The main thread resolves elements from the last index of the first screen
  // list
  while (ui_thread_cursor_ >= async_thread_cursor_ && ui_thread_cursor_ >= 0) {
    ui_thread_cursor_--;
    const auto &element = first_screen_list_[ui_thread_cursor_ + 1];
    if (element->CalcStyle(false)) {
      element->FlushPropsResolveStyles(false);
    } else {
      ui_thread_cursor_++;
      break;
    }
  }
  for (; index <= ui_thread_cursor_; ++index) {
    const auto &element = first_screen_list_[index];
    if (!(element->state_ & ElementState::kStyleCalculated)) {
      element->CalcStyle(true);
    }
    element->FlushPropsResolveStyles(false);
  }
  first_screen_list_.clear();
  SetEnableAsyncCalc(false);
}

void AirPageElement::InitFirstScreenList(size_t size) {
  first_screen_list_.reserve(size);
}

void AirPageElement::SetParsedStyles(const AirCompStylesMap &parsed_styles) {
  AirElement::SetParsedStyles(parsed_styles);
  // Using air_page to distinguish components
  AddFontFaces(parsed_styles, "$air_page");
}

void AirPageElement::AddFontFaces(const AirCompStylesMap &parsed_styles,
                                  std::string path) {
  if (component_fontfaces_map_.find(path) != component_fontfaces_map_.end()) {
    return;
  }
  BASE_STATIC_STRING_DECL(kFontFace, "$fontface");
  auto iter = parsed_styles.find(kFontFace.str());
  if (iter != parsed_styles.end()) {
    BASE_STATIC_STRING_DECL(kFontFamily, "font-family");
    const auto &style = iter->second->at(kPropertyIDFontFamily);
    tasm::ForEachLepusValue(
        style.GetValue(), [this, &kFontFamily](const lepus::Value &key,
                                               const lepus::Value &value) {
          const std::string &font_face_name =
              value.GetProperty(kFontFamily).StdString();
          if (!font_face_name.empty()) {
            auto token_ptr =
                std::shared_ptr<CSSFontFaceRule>(MakeCSSFontFaceToken(value));
            std::vector<std::shared_ptr<CSSFontFaceRule>> token_list;
            token_list.emplace_back(token_ptr);
            font_faces_.insert(
                std::make_pair(font_face_name, std::move(token_list)));
          }
        });
    component_fontfaces_map_.emplace(std::move(path), true);
  }
}

void AirPageElement::SetFontFaces() {
  if (!font_faces_.empty()) {
    element_manager()->SetFontFaces(font_faces_);
    font_faces_.clear();
  }
}

}  // namespace tasm
}  // namespace lynx
