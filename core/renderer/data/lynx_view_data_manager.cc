// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/data/lynx_view_data_manager.h"

#include "base/include/debug/lynx_assert.h"
#include "base/include/log/logging.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/runtime/lepus/json_parser.h"
#include "core/runtime/lepus/lepus_global.h"

namespace lynx {
namespace tasm {

lepus::Value* LynxViewDataManager::ParseData(const char* data) {
  lepus_value* value = new lepus_value;
  *value = lepus::jsonValueTolepusValue(data);
  if (!value->IsTable()) {
    LynxInfo(error::E_DATA_FLOW_UPDATE_INVALID_TYPE,
             "ParseData error, data is:%s", data);
    value->SetTable(lepus::Dictionary::Create());
  }
  return value;
}

void LynxViewDataManager::UpdateData(lepus::Value& dest,
                                     const lepus::Value& src) {
  if (src.IsTable()) {
    auto src_dict = src.Table();
    auto dest_dict = dest.Table();
    dest_dict->reserve(src_dict->size());
    for (const auto& [src_key, src_value] : *src_dict) {
      if (src_value.IsTable()) {
        lynx::lepus::Value old_value = dest_dict->GetValue(src_key);
        if (old_value.IsTable()) {
          if (old_value.Table()->IsConst()) {
            old_value = lynx::lepus::Value::Clone(old_value);
            dest_dict->SetValue(src_key, old_value);
          }
          // Merge table.
          auto target_table = old_value.Table();
          auto table = src_value.Table();
          if (target_table.get() != table.get()) {
            for (const auto& [key, value] : *table) {
              target_table->SetValue(key, value);
            }
          }
          continue;
        }
      }
      dest_dict->SetValue(src_key, src_value);
    }
  }
}

void LynxViewDataManager::ReleaseData(lepus::Value* obj) {
  if (obj != nullptr) {
    delete obj;
  }
}

}  // namespace tasm
}  // namespace lynx
