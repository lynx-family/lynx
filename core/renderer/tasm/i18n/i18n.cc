// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/tasm/i18n/i18n.h"

#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/json_parser.h"

#if OS_ANDROID
#include "core/renderer/tasm/i18n/i18n_binder_android.h"
#elif OS_IOS
#include "core/renderer/tasm/i18n/i18n_binder_darwin.h"
#endif

namespace lynx {
namespace tasm {

bool I18n::UpdateData(const std::string& key, const std::string& new_data) {
  auto& i18n_wrapper = i18n_record_[key];
  bool is_sync = true;
  if (i18n_wrapper.status == I18n_STATUS_IDLE) {  // meaningless data
    return is_sync;
  }
  lepus::Value lepus_data =
      lynx::lepus::jsonValueTolepusValue(new_data.c_str());

  if (i18n_wrapper.reserve_keys.size() > 0 && lepus_data.IsObject()) {
    // filter lepus_data with reserve keys, only exist when reserve key is not
    // empty
    lepus::Value new_value(lepus::Dictionary::Create());
    tasm::ForEachLepusValue(
        lepus_data, [&new_value, &i18n_wrapper](const lepus::Value& key,
                                                const lepus::Value& val) {
          auto key_string = key.ToString();
          if (i18n_wrapper.reserve_keys.find(key_string) !=
              i18n_wrapper.reserve_keys.end()) {
            new_value.SetProperty(key_string, val);
          }
        });
    i18n_wrapper.lepus_data = new_value;
  } else {
    i18n_wrapper.lepus_data = lepus_data;
  }

  if (i18n_wrapper.status == I18n_STATUS_FINISH) {
    is_sync = false;  // async response
    i18n_wrapper.status = I18n_STATUS_IDLE;
  }
  return is_sync;
}

lepus::Value I18n::GetData(TemplateAssembler* tasm, const std::string& channel,
                           const std::string& fallback_url) {
  auto& i18n_wrapper = i18n_record_[channel];
  if (i18n_wrapper.lepus_data.IsObject() ||
      i18n_wrapper.status != I18n_STATUS_IDLE) {
    return i18n_wrapper.lepus_data;
  } else {
    i18n_wrapper.status = I18n_STATUS_WAIT;
  }
  LOGI("run GetI18NResources from channel: " << channel);
  tasm->GetDelegate().GetI18nResource(channel, fallback_url);
  i18n_wrapper.status = I18n_STATUS_FINISH;
  return i18n_wrapper.lepus_data;
}

void I18n::SetChannelConfig(const std::string& channel,
                            const lepus::Value& reserve_keys) {
  LOGI("SetChannelConfig for channel: " << channel);
  auto& i18n_wrapper = i18n_record_[channel];
  if (i18n_wrapper.lepus_data.IsObject() ||
      i18n_wrapper.status != I18n_STATUS_IDLE) {
    // it means data of the channel is ready
    return;
  } else {
    if (reserve_keys.IsArrayOrJSArray()) {
      // save reserve_keys with set for filter i18n resource
      std::unordered_set<std::string> array_keys_;

      for (auto i = 0; i < reserve_keys.GetLength(); ++i) {
        if (reserve_keys.GetProperty(i).IsString()) {
          array_keys_.insert(reserve_keys.GetProperty(i).ToString());
        }
      }
      i18n_wrapper.reserve_keys = array_keys_;
    }
  }
}

void I18n::Bind(intptr_t ptr) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "i18n::Bind");
#if OS_ANDROID
  I18nBinderAndroid binder;
  binder.Bind(ptr);
#elif OS_IOS
  I18nBinderDarwin binder;
  binder.Bind(ptr);
#endif
}

}  // namespace tasm
}  // namespace lynx
