// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_COMMON_HARMONY_PROP_BUNDLE_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_COMMON_HARMONY_PROP_BUNDLE_HARMONY_H_

#include <napi/native_api.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/value/base_value.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/ui_wrapper/common/native_prop_bundle.h"

namespace lynx {

namespace tasm {

class GestureDetector;

class PropBundleHarmony : public NativePropBundle {
  using PropMap = std::unordered_map<std::string, lepus::Value>;

 public:
  PropBundleHarmony();
  explicit PropBundleHarmony(
      const PropMap& props,
      const base::flex_optional<std::vector<lepus::Value>>& event);
  ~PropBundleHarmony() override;

  napi_value GetJSProps() const;

  napi_value GetJSEventHandler() const;

  fml::RefPtr<PropBundle> ShallowCopy() override;

  static napi_value CreateNapiValue(napi_env env, const lepus::Value& value);

  static void Init(napi_env env) { env_ = env; }

 private:
  static napi_env env_;
  static void AssembleArray(napi_env env, napi_value array, uint32_t index,
                            const lepus::Value& value);
  static void AssembleMap(napi_env env, napi_value object, const char* key,
                          const lepus::Value& value);
  static napi_value GetNapiValue(napi_ref ref);
};

class PropBundleCreatorHarmony : public PropBundleCreator {
 public:
  fml::RefPtr<PropBundle> CreatePropBundle() override;
};

}  // namespace tasm

}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_COMMON_HARMONY_PROP_BUNDLE_HARMONY_H_
