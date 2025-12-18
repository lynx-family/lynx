// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/common/harmony/prop_bundle_harmony.h"

#include <cstdint>
#include <utility>

#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace tasm {
using base::NapiHandleScope;

PropBundleHarmony::PropBundleHarmony() = default;

PropBundleHarmony::PropBundleHarmony(
    const PropMap& props,
    const base::flex_optional<std::vector<lepus::Value>>& event)
    : NativePropBundle(props, event) {}

PropBundleHarmony::~PropBundleHarmony() = default;

fml::RefPtr<PropBundle> PropBundleHarmony::ShallowCopy() {
  auto prop = fml::MakeRefCounted<PropBundleHarmony>(props_, event_handler_);
  return prop;
}

napi_value PropBundleHarmony::GetJSProps() const {
  napi_value result = nullptr;
  napi_create_object(env_, &result);
  napi_env env = env_;
  std::for_each(
      props_.begin(), props_.end(),
      [env, result](const std::pair<std::string, const lepus::Value&>& entry) {
        AssembleMap(env, result, entry.first.c_str(), entry.second);
      });
  return result;
}

napi_value PropBundleHarmony::GetJSEventHandler() const {
  napi_value js_event_handler;
  if (event_handler_ && !event_handler_->empty()) {
    napi_create_array(env_, &js_event_handler);
    napi_env env = env_;
    size_t index = 0;
    std::for_each(event_handler_->begin(), event_handler_->end(),
                  [env, js_event_handler, &index](const lepus::Value& handler) {
                    napi_value event_value = CreateNapiValue(env, handler);
                    napi_set_element(env, js_event_handler, index, event_value);
                    ++index;
                  });
    return js_event_handler;
  } else {
    napi_get_undefined(env_, &js_event_handler);
    return js_event_handler;
  }
}

napi_value PropBundleHarmony::CreateNapiValue(napi_env env,
                                              const lepus::Value& value) {
  napi_value result = nullptr;
  if (value.IsNil()) {
    napi_get_null(env, &result);
  } else if (value.IsString()) {
    napi_create_string_utf8(env, value.CString(), NAPI_AUTO_LENGTH, &result);
  } else if (value.IsInt32()) {
    napi_create_int32(env, value.Int32(), &result);
  } else if (value.IsInt64()) {
    napi_create_int64(env, value.Int64(), &result);
  } else if (value.IsUInt32()) {
    napi_create_uint32(env, value.UInt32(), &result);
  } else if (value.IsUInt64()) {
    napi_create_int64(env, value.Int64(), &result);
  } else if (value.IsNumber()) {
    napi_create_double(env, value.Number(), &result);
  } else if (value.IsArrayOrJSArray()) {
    napi_create_array(env, &result);
    ForEachLepusValue(value, [&env, &result](const lepus::Value& index,
                                             const lepus::Value& val) {
      AssembleArray(env, result, index.Int64(), val);
    });
  } else if (value.IsByteArray()) {
    result = base::NapiUtil::CreateArrayBuffer(env, value.ByteArray().get(),
                                               value.ByteArray()->GetLength());
  } else if (value.IsObject()) {
    napi_create_object(env, &result);
    ForEachLepusValue(
        value, [&env, &result](const lepus::Value& k, const lepus::Value& v) {
          AssembleMap(env, result, k.CString(), v);
        });
  } else if (value.IsBool()) {
    napi_get_boolean(env, value.Bool(), &result);
  } else if (value.IsUndefined()) {
    // default is undefined
  } else {
    LOGE("PropBundleHarmony, unknown type :" << value.Type());
  }
  return result;
}

void PropBundleHarmony::AssembleArray(napi_env env, napi_value array,
                                      uint32_t index,
                                      const lepus::Value& value) {
  napi_value result = CreateNapiValue(env, value);
  napi_set_element(env, array, index, result);
}

void PropBundleHarmony::AssembleMap(napi_env env, napi_value object,
                                    const char* key,
                                    const lepus::Value& value) {
  napi_value k;
  napi_create_string_latin1(env, key, NAPI_AUTO_LENGTH, &k);
  napi_value result = CreateNapiValue(env, value);
  napi_set_property(env, object, k, result);
}

napi_value PropBundleHarmony::GetNapiValue(napi_ref ref) {
  napi_value ret = nullptr;
  napi_status status = napi_get_reference_value(env_, ref, &ret);
  if (status != napi_ok) {
    return nullptr;
  }
  return ret;
}

fml::RefPtr<PropBundle> PropBundleCreatorHarmony::CreatePropBundle() {
  return fml::MakeRefCounted<PropBundleHarmony>();
}

napi_env PropBundleHarmony::env_ = nullptr;

}  // namespace tasm

}  // namespace lynx
