// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_HARMONY_LYNX_TEMPLATE_BUNDLE_HARMONY_H_
#define CORE_RENDERER_DOM_HARMONY_LYNX_TEMPLATE_BUNDLE_HARMONY_H_

#include <node_api.h>

#include <memory>
#include <string>

#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {

namespace harmony {
class LynxTemplateBundleHarmony {
 public:
  LynxTemplateBundleHarmony() = default;
  ~LynxTemplateBundleHarmony() = default;

  tasm::LynxTemplateBundle& GetBundle() { return *bundle_; };
  void SetBundle(tasm::LynxTemplateBundle bundle);

  static napi_value Init(napi_env env, napi_value exports);
  static napi_value New(napi_env env, napi_callback_info info);
  static napi_value ParseTemplate(napi_env env, napi_callback_info info);
  static napi_value GetExtraInfo(napi_env env, napi_callback_info info);
  static napi_value GetContainsElementTree(napi_env env,
                                           napi_callback_info info);
  static napi_value InitWithOption(napi_env env, napi_callback_info info);
  static napi_value PostJsCacheGenerationTask(napi_env env,
                                              napi_callback_info info);

  napi_value GetExtraInfo(napi_env env);
  napi_value GetContainsElementTree(napi_env env);
  napi_value InitWithOption(napi_env env, int32_t count, bool enable);
  napi_value PostJsCacheGenerationTask(napi_env env,
                                       std::string bytecode_source_url,
                                       bool use_v8);

 private:
  std::unique_ptr<tasm::LynxTemplateBundle> bundle_;
};
}  // namespace harmony
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_HARMONY_LYNX_TEMPLATE_BUNDLE_HARMONY_H_
