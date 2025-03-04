// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/worklet/base/worklet_utils.h"

#include "core/renderer/utils/value_utils.h"
#include "core/runtime/bindings/napi/worklet/napi_loader_ui.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace worklet {

Napi::String ValueConverter::ConvertStdStringToNapiString(
    Napi::Env env, const std::string& value) {
  return Napi::String::New(env, value);
}

Napi::Boolean ValueConverter::ConvertLepusBoolToNapiBoolean(Napi::Env env,
                                                            bool value) {
  return Napi::Boolean::New(env, value);
}

Napi::Number ValueConverter::ConvertLepusInt32ToNapiNumber(Napi::Env env,
                                                           int32_t value) {
  return Napi::Number::New(env, value);
}

Napi::Number ValueConverter::ConvertLepusUInt32ToNapiNumber(Napi::Env env,
                                                            uint32_t value) {
  return Napi::Number::New(env, value);
}

Napi::Number ValueConverter::ConvertLepusInt64ToNapiNumber(Napi::Env env,
                                                           int64_t value) {
  return Napi::Number::New(env, value);
}

Napi::Number ValueConverter::ConvertLepusUInt64ToNapiNumber(Napi::Env env,
                                                            uint64_t value) {
  return Napi::Number::New(env, value);
}

Napi::Number ValueConverter::ConvertLepusNumberToNapiNumber(
    Napi::Env env, const lepus::Value& value) {
  return Napi::Number::New(env, value.Number());
}

Napi::Array ValueConverter::ConvertLepusValueToNapiArray(
    Napi::Env env, const lepus::Value& value) {
  Napi::Array ary = Napi::Array::New(env);
  tasm::ForEachLepusValue(
      value, [&ary, &env](const lepus::Value& key, const lepus::Value& val) {
        Napi::Value napi_value = ConvertLepusValueToNapiValue(env, val);
        ary.Set(key.Number(), napi_value);
      });
  return ary;
}

Napi::Object ValueConverter::ConvertLepusValueToNapiObject(
    Napi::Env env, const lepus::Value& value) {
  Napi::Object obj = Napi::Object::New(env);
  tasm::ForEachLepusValue(value, [&obj, &env](const lepus::Value& key,
                                              const lepus::Value& val) {
    Napi::Value napi_key = ConvertStdStringToNapiString(env, key.StdString());
    Napi::Value napi_value = ConvertLepusValueToNapiValue(env, val);
    obj.Set(napi_key, napi_value);
  });
  return obj;
}

Napi::Value ValueConverter::ConvertLepusValueToNapiValue(
    Napi::Env env, const lepus::Value& value) {
  Napi::Value res;
  if (value.IsString()) {
    res = ConvertStdStringToNapiString(env, value.StdString());
  } else if (value.IsBool()) {
    res = ConvertLepusBoolToNapiBoolean(env, value.Bool());
  } else if (value.IsInt32()) {
    res = ConvertLepusInt32ToNapiNumber(env, value.Int32());
  } else if (value.IsUInt32()) {
    res = ConvertLepusUInt32ToNapiNumber(env, value.UInt32());
  } else if (value.IsInt64()) {
    res = ConvertLepusInt64ToNapiNumber(env, value.Int64());
  } else if (value.IsUInt64()) {
    res = ConvertLepusUInt64ToNapiNumber(env, value.UInt64());
  } else if (value.IsNumber()) {
    res = ConvertLepusNumberToNapiNumber(env, value);
  } else if (value.IsArrayOrJSArray()) {
    res = ConvertLepusValueToNapiArray(env, value);
  } else if (value.IsObject()) {
    res = ConvertLepusValueToNapiObject(env, value);
  } else if (value.IsUndefined()) {
    res = env.Undefined();
  } else if (value.IsNil()) {
    res = env.Null();
  }
  return res;
}

lepus::Value ValueConverter::ConvertNapiValueToLepusValue(
    const Napi::Value& value) {
  auto ctx = NapiLoaderUI::GetQuickContextFromNapiEnv(value.Env());
  if (ctx == nullptr) {
    LOGE(
        "ValueConverter ConvertNapiValueToLepusValue failed, since can't find "
        "its context.");
    return lepus::Value();
  }

  return lepus::Value(ctx->context(), *reinterpret_cast<LEPUSValue*>(
                                          static_cast<napi_value>(value)))
      .ToLepusValue();
}

Napi::Value ValueConverter::ConvertPubValueToNapiValue(
    Napi::Env env, const pub::Value& value) {
  Napi::Value res;
  if (value.IsString()) {
    res = Napi::String::New(env, value.str());
  } else if (value.IsBool()) {
    res = Napi::Boolean::New(env, value.Bool());
  } else if (value.IsInt32()) {
    res = Napi::Number::New(env, value.Int32());
  } else if (value.IsUInt32()) {
    res = Napi::Number::New(env, value.UInt32());
  } else if (value.IsInt64()) {
    res = Napi::Number::New(env, value.Int64());
  } else if (value.IsUInt64()) {
    res = Napi::Number::New(env, value.UInt64());
  } else if (value.IsNumber()) {
    res = Napi::Number::New(env, value.Number());
  } else if (value.IsArray()) {
    res = ConvertPubValueToNapiArray(env, value);
  } else if (value.IsMap()) {
    res = ConvertPubValueToNapiObject(env, value);
  } else if (value.IsUndefined()) {
    res = env.Undefined();
  } else if (value.IsNil()) {
    res = env.Null();
  }
  return res;
}

Napi::Array ValueConverter::ConvertPubValueToNapiArray(
    Napi::Env env, const pub::Value& value) {
  Napi::Array ary = Napi::Array::New(env);
  value.ForeachArray([&ary, &env](int64_t index, const pub::Value& val) {
    Napi::Value napi_value = ConvertPubValueToNapiValue(env, val);
    ary.Set(index, napi_value);
  });
  return ary;
}

Napi::Object ValueConverter::ConvertPubValueToNapiObject(
    Napi::Env env, const pub::Value& value) {
  Napi::Object obj = Napi::Object::New(env);
  value.ForeachMap([&obj, &env](const pub::Value& key, const pub::Value& val) {
    Napi::Value napi_key = Napi::String::New(env, key.str());
    Napi::Value napi_value = ConvertPubValueToNapiValue(env, val);

    obj.Set(napi_key, napi_value);
  });
  return obj;
}

}  // namespace worklet
}  // namespace lynx
