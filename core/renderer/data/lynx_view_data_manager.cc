// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/data/lynx_view_data_manager.h"

#include "base/include/debug/lynx_assert.h"
#include "base/include/log/logging.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/lepus_global.h"

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

bool LynxViewDataManager::UpdateData(lepus::Value* target,
                                     lepus::Value* value) {
  if (!target->IsTable()) {
    target->SetTable(lepus::Dictionary::Create());
  }
  auto data_dict = target->Table();

  if (!value->IsTable()) {
    return false;
  }
  auto dict = value->Table();
  for (auto iter = dict->begin(); iter != dict->end(); ++iter) {
    data_dict->SetValue(iter->first, dict->GetValue(iter->first));
  }
  return true;
}

void LynxViewDataManager::ReleaseData(lepus::Value* obj) {
  if (obj != nullptr) {
    delete obj;
  }
}

}  // namespace tasm
}  // namespace lynx
