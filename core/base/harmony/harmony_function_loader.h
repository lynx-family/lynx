// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_HARMONY_HARMONY_FUNCTION_LOADER_H_
#define CORE_BASE_HARMONY_HARMONY_FUNCTION_LOADER_H_

#include <arkui/native_node.h>

namespace lynx {
namespace base {
namespace harmony {

#define HARMONY_COMPAT_FUNCTIONS                                               \
  X(ArkUI_SnapshotOptions*, oh_create_snapshot_option_func, (),                \
    "OH_ArkUI_CreateSnapshotOptions")                                          \
  X(ArkUI_ErrorCode, oh_destroy_snapshot_option_func,                          \
    (ArkUI_SnapshotOptions*), "OH_ArkUI_DestroySnapshotOptions")               \
  X(ArkUI_ErrorCode, oh_snapshot_option_set_scale_func,                        \
    (ArkUI_SnapshotOptions*, float), "OH_ArkUI_SnapshotOptions_SetScale")      \
  X(ArkUI_ErrorCode, oh_get_node_snapshot_func,                                \
    (ArkUI_NodeHandle, ArkUI_SnapshotOptions*, OH_PixelmapNative**),           \
    "OH_ArkUI_GetNodeSnapshot")                                                \
  X(ArkUI_CrossLanguageOption*, oh_create_cross_language_option_func, (),      \
    "OH_ArkUI_CrossLanguageOption_Create")                                     \
  X(void, oh_destroy_cross_language_option_func, (ArkUI_CrossLanguageOption*), \
    "OH_ArkUI_CrossLanguageOption_Destroy")                                    \
  X(ArkUI_ErrorCode, oh_set_node_utils_set_cross_language_option_func,         \
    (ArkUI_NodeHandle, ArkUI_CrossLanguageOption*),                            \
    "OH_ArkUI_NodeUtils_SetCrossLanguageOption")                               \
  X(void, oh_set_cross_language_option_set_attribute_setting_status_func,      \
    (ArkUI_CrossLanguageOption*, bool),                                        \
    "OH_ArkUI_CrossLanguageOption_SetAttributeSettingStatus")                  \
  X(bool, oh_get_cross_language_option_get_attribute_setting_status_func,      \
    (ArkUI_CrossLanguageOption*),                                              \
    "OH_ArkUI_CrossLanguageOption_GetAttributeSettingStatus")

struct HarmonyCompatFunctionsHandler {
#define X(ret, name, args, symbol) ret(*name) args = nullptr;
  HARMONY_COMPAT_FUNCTIONS
#undef X
};

HarmonyCompatFunctionsHandler* GetHarmonyCompatFunctionsHandler();
}  // namespace harmony
}  // namespace base
}  // namespace lynx
#endif  // CORE_BASE_HARMONY_HARMONY_FUNCTION_LOADER_H_
