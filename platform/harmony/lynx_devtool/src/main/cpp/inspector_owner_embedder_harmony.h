// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_INSPECTOR_OWNER_EMBEDDER_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_INSPECTOR_OWNER_EMBEDDER_HARMONY_H_

#include <node_api.h>

#include <string>

#include "devtool/embedder/core/inspector_owner_embedder.h"

namespace lynx {
namespace devtool {

class InspectorOwnerHarmony;

class InspectorOwnerEmbedderHarmony : public InspectorOwnerEmbedder {
 public:
  InspectorOwnerEmbedderHarmony(napi_env env, napi_ref ref);
  ~InspectorOwnerEmbedderHarmony() override = default;

  void OnConsoleMessage(const std::string& message) override;
  void OnConsoleObject(const std::string& detail, int callback_id) override;

  void Destroy();

 private:
  napi_env env_;
  napi_ref ref_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_INSPECTOR_OWNER_EMBEDDER_HARMONY_H_
