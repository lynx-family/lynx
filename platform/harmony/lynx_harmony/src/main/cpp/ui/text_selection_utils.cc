// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// cspell:ignore Udmf

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/text_selection_utils.h"

#include <database/pasteboard/oh_pasteboard.h>
#include <database/udmf/udmf.h>
#include <database/udmf/uds.h>
#include <resourcemanager/ohresmgr.h>

#include <cstdlib>

#include "core/resource/lynx_resource_loader_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {

std::string GetTextSelectionLocalizedString(const char* resource_name,
                                            const char* fallback) {
  NativeResourceManager* resource_manager =
      lynx::harmony::LynxResourceLoaderHarmony::resource_manager;
  char* value = nullptr;
  if (resource_manager &&
      OH_ResourceManager_GetStringByName(resource_manager, resource_name,
                                         &value) == SUCCESS &&
      value) {
    std::string result(value);
    std::free(value);
    return result;
  }
  std::free(value);
  return fallback;
}

bool CopyTextSelectionToPasteboard(const std::string& selected_text) {
  if (selected_text.empty()) {
    return false;
  }

  OH_Pasteboard* pasteboard = OH_Pasteboard_Create();
  OH_UdsPlainText* plain_text = OH_UdsPlainText_Create();
  OH_UdmfRecord* record = OH_UdmfRecord_Create();
  OH_UdmfData* data = OH_UdmfData_Create();
  bool success = pasteboard && plain_text && record && data;
  if (success) {
    success =
        OH_UdsPlainText_SetContent(plain_text, selected_text.c_str()) == 0;
  }
  if (success) {
    success = OH_UdmfRecord_AddPlainText(record, plain_text) == 0;
  }
  if (success) {
    success = OH_UdmfData_AddRecord(data, record) == 0;
  }
  if (success) {
    success = OH_Pasteboard_SetData(pasteboard, data) == 0;
  }

  if (plain_text) {
    OH_UdsPlainText_Destroy(plain_text);
  }
  if (record) {
    OH_UdmfRecord_Destroy(record);
  }
  if (data) {
    OH_UdmfData_Destroy(data);
  }
  if (pasteboard) {
    OH_Pasteboard_Destroy(pasteboard);
  }
  return success;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
