// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LIST_ITEM_TRANSFORMER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LIST_ITEM_TRANSFORMER_H_

#include <node_api.h>

#include <array>
#include <optional>

namespace lynx {
namespace tasm {
namespace harmony {

// Owns and invokes ArkTS transform and reset callbacks on the UI thread.
class ListItemTransformer {
 public:
  ListItemTransformer(napi_env env, napi_ref transform_callback,
                      napi_ref reset_callback)
      : env_(env),
        transform_callback_(transform_callback),
        reset_callback_(reset_callback) {}

  ~ListItemTransformer() {
    napi_delete_reference(env_, transform_callback_);
    napi_delete_reference(env_, reset_callback_);
  }

  ListItemTransformer(const ListItemTransformer&) = delete;
  ListItemTransformer& operator=(const ListItemTransformer&) = delete;

  // Returns [scaleX, scaleY, translationX, translationY], or no value on
  // failure.
  std::optional<std::array<float, 4>> TransformItem(float main_axis_size,
                                                    float main_axis_offset,
                                                    bool is_vertical,
                                                    bool is_rtl);

  // Invokes reset without arguments and returns the same four transform values.
  std::optional<std::array<float, 4>> ResetItem();

 private:
  std::optional<std::array<float, 4>> InvokeCallback(napi_ref callback_ref,
                                                     size_t argc,
                                                     const napi_value* args);

  napi_env env_;
  napi_ref transform_callback_;
  napi_ref reset_callback_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LIST_ITEM_TRANSFORMER_H_
