// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/harmony/lynx_template_bundle_harmony.h"

#include <js_native_api.h>
#include <js_native_api_types.h>

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "core/runtime/jscache/js_cache_manager_facade.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"

namespace lynx {

namespace harmony {
void LynxTemplateBundleHarmony::SetBundle(tasm::LynxTemplateBundle bundle) {
  bundle_ = std::make_unique<tasm::LynxTemplateBundle>(std::move(bundle));
}

napi_value LynxTemplateBundleHarmony::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_STATIC_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_default, nullptr}

  napi_property_descriptor properties[] = {
      DECLARE_NAPI_STATIC_FUNCTION("nativeParseTemplate", ParseTemplate),
      DECLARE_NAPI_STATIC_FUNCTION("nativeGetExtraInfo", GetExtraInfo),
      DECLARE_NAPI_STATIC_FUNCTION("nativeGetContainsElementTree",
                                   GetContainsElementTree),
      DECLARE_NAPI_STATIC_FUNCTION("nativeInitWithOption", InitWithOption),
      DECLARE_NAPI_STATIC_FUNCTION("nativePostJsCacheGenerationTask",
                                   PostJsCacheGenerationTask),
  };
#undef DECLARE_NAPI_FUNCTION
  constexpr size_t size = std::size(properties);
  napi_value cons;
  napi_status status =
      napi_define_class(env, "TemplateBundle", NAPI_AUTO_LENGTH, New, nullptr,
                        size, properties, &cons);
  if (status != napi_ok) {
    LOGE("fail to define TemplateBundle class " << status);
    return nullptr;
  }

  status = napi_set_named_property(env, exports, "TemplateBundle", cons);
  if (status != napi_ok) {
    LOGE("fail to set TemplateBundle property " << status);
    return nullptr;
  }

  return exports;
}

napi_value LynxTemplateBundleHarmony::New(napi_env env,
                                          napi_callback_info info) {
  static napi_finalize finalize_cb = [](napi_env env, void* finalize_data,
                                        void* finalize_hint) {
    if (finalize_data != nullptr) {
      auto* bundle = static_cast<LynxTemplateBundleHarmony*>(finalize_data);
      delete bundle;
    }
  };

  napi_value js_this;
  auto status =
      napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
  if (status != napi_ok) {
    LOGE("get callback info failed " << status);
    return nullptr;
  }

  auto* bundle = new LynxTemplateBundleHarmony();
  status = napi_wrap(env, js_this, static_cast<void*>(bundle), finalize_cb,
                     nullptr, nullptr);
  if (status != napi_ok) {
    LOGE("fail to wrap bundle to js_this " << status);
    return nullptr;
  }

  return js_this;
}

napi_value LynxTemplateBundleHarmony::ParseTemplate(napi_env env,
                                                    napi_callback_info info) {
  size_t argc = 1;
  napi_value args[argc];
  napi_value js_this = nullptr;
  napi_status status =
      napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
  if (status != napi_ok) {
    LOGE("fail to get callback info " << status);
    return nullptr;
  }

  LynxTemplateBundleHarmony* bundle = nullptr;
  status = napi_unwrap(env, js_this, reinterpret_cast<void**>(&bundle));
  if (status != napi_ok) {
    LOGE("fail to unwrap bundle from js_this " << status);
    return nullptr;
  }

  std::vector<uint8_t> template_buffer = {};
  bool result =
      base::NapiUtil::ConvertToArrayBuffer(env, args[0], template_buffer);
  if (!result) {
    return nullptr;
  }

  napi_value error_value = nullptr;
  auto decoder = lynx::tasm::LynxBinaryReader::CreateLynxBinaryReader(
      std::move(template_buffer));
  if (!decoder.Decode()) {
    status =
        napi_create_string_utf8(env, decoder.error_message_.c_str(),
                                decoder.error_message_.length(), &error_value);
    return error_value;
  }

  bundle->SetBundle(decoder.GetTemplateBundle());
  napi_create_string_utf8(env, "", 0, &error_value);

  return error_value;
}

napi_value LynxTemplateBundleHarmony::GetExtraInfo(napi_env env,
                                                   napi_callback_info info) {
  napi_value js_this = nullptr;
  napi_status status =
      napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
  if (status != napi_ok) {
    LOGE("fail to get callback info " << status);
    return nullptr;
  }

  LynxTemplateBundleHarmony* bundle = nullptr;
  status = napi_unwrap(env, js_this, reinterpret_cast<void**>(&bundle));
  if (status != napi_ok) {
    LOGE("fail to unwrap bundle from js_this " << status);
    return nullptr;
  }

  return bundle->GetExtraInfo(env);
}

napi_value LynxTemplateBundleHarmony::GetExtraInfo(napi_env env) {
  lynx::lepus::Value extra_info = bundle_->GetExtraInfo();
  napi_value result = base::NapiConvertHelper::CreateNapiValue(env, extra_info);
  return result;
}

napi_value LynxTemplateBundleHarmony::GetContainsElementTree(
    napi_env env, napi_callback_info info) {
  napi_value js_this = nullptr;
  napi_status status =
      napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
  if (status != napi_ok) {
    LOGE("fail to get callback info " << status);
    return nullptr;
  }

  LynxTemplateBundleHarmony* bundle = nullptr;
  status = napi_unwrap(env, js_this, reinterpret_cast<void**>(&bundle));
  if (status != napi_ok) {
    LOGE("fail to unwrap bundle from js_this " << status);
    return nullptr;
  }

  return bundle->GetContainsElementTree(env);
}

napi_value LynxTemplateBundleHarmony::GetContainsElementTree(napi_env env) {
  bool result = bundle_->GetContainsElementTree();
  napi_value result_value = nullptr;
  auto status = napi_get_boolean(env, result, &result_value);
  if (status != napi_ok) {
    LOGE("fail to get boolean " << status);
  }
  return result_value;
}

napi_value LynxTemplateBundleHarmony::InitWithOption(napi_env env,
                                                     napi_callback_info info) {
  size_t argc = 2;
  napi_value args[argc];
  napi_value js_this = nullptr;
  napi_status status =
      napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
  if (status != napi_ok) {
    LOGE("fail to get callback info " << status);
    return nullptr;
  }

  LynxTemplateBundleHarmony* bundle = nullptr;
  status = napi_unwrap(env, js_this, reinterpret_cast<void**>(&bundle));
  if (status != napi_ok) {
    LOGE("fail to unwrap bundle from js_this " << status);
    return nullptr;
  }

  int32_t count = 0;
  status = napi_get_value_int32(env, args[0], &count);
  if (status != napi_ok) {
    LOGE("fail to get int32 value " << status);
    return nullptr;
  }

  bool enable = false;
  status = napi_get_value_bool(env, args[1], &enable);
  if (status != napi_ok) {
    LOGE("fail to get bool value " << status);
    return nullptr;
  }
  return bundle->InitWithOption(env, count, enable);
}

napi_value LynxTemplateBundleHarmony::InitWithOption(napi_env env,
                                                     int32_t count,
                                                     bool enable) {
  bundle_->PrepareLepusContext(count);
  bundle_->SetEnableVMAutoGenerate(enable);
  return nullptr;
}

napi_value LynxTemplateBundleHarmony::PostJsCacheGenerationTask(
    napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[argc];
  napi_value js_this = nullptr;
  napi_status status =
      napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
  if (status != napi_ok) {
    LOGE("fail to get callback info " << status);
    return nullptr;
  }

  LynxTemplateBundleHarmony* bundle = nullptr;
  status = napi_unwrap(env, js_this, reinterpret_cast<void**>(&bundle));
  if (status != napi_ok) {
    LOGE("fail to unwrap bundle from js_this " << status);
    return nullptr;
  }

  std::string bytecode_source_url =
      base::NapiUtil::ConvertToString(env, args[0]);
  if (bytecode_source_url.empty()) {
    return nullptr;
  }

  bool use_v8 = false;
  status = napi_get_value_bool(env, args[1], &use_v8);
  if (status != napi_ok) {
    LOGE("fail to get bool value " << status);
    return nullptr;
  }
  return bundle->PostJsCacheGenerationTask(env, bytecode_source_url, use_v8);
}

napi_value LynxTemplateBundleHarmony::PostJsCacheGenerationTask(
    napi_env env, std::string bytecode_source_url, bool use_v8) {
  lynx::piper::cache::JsCacheManagerFacade::PostCacheGenerationTask(
      *bundle_, bytecode_source_url,
      use_v8 ? lynx::piper::JSRuntimeType::v8
             : lynx::piper::JSRuntimeType::quickjs);
  return nullptr;
}

}  // namespace harmony
}  // namespace lynx
