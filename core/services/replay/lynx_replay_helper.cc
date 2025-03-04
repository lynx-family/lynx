// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/lynx_replay_helper.h"

namespace lynx {
namespace piper {

piper::Value ReplayHelper::convertRapidJsonStringToJSIValue(
    Runtime &runtime, rapidjson::Value &value) {
  if (strcmp(value.GetString(), "NaN") == 0) {
    return piper::Value(std::numeric_limits<double>::quiet_NaN());
  }
  return piper::String::createFromUtf8(runtime, value.GetString());
}

piper::Value ReplayHelper::convertRapidJsonNumberToJSIValue(
    Runtime &runtime, rapidjson::Value &value) {
  double v = 0;
  if (value.IsInt()) {
    v = value.GetInt();
  } else if (value.IsFloat()) {
    v = value.GetDouble();
  } else if (value.IsDouble()) {
    v = value.GetDouble();
  } else if (value.IsInt64()) {
    v = value.GetInt64();
  } else if (value.IsUint()) {
    v = value.GetUint64();
  } else if (value.IsUint64()) {
    v = value.GetUint64();
  }
  return piper::Value(v);
}

std::optional<piper::Value>
ReplayHelper::convertRapidJsonLynxValObjectToJSIValue(Runtime &runtime,
                                                      rapidjson::Value &value) {
  if (value.GetType() != rapidjson::kObjectType ||
      !value.GetObject().HasMember("__lynx_val__")) {
    return std::nullopt;
  }
  auto lynx_val = value.GetObject().FindMember("__lynx_val__");
  return convertRapidJsonObjectToJSIValue(runtime, lynx_val->value);
}

piper::Value ReplayHelper::convertRapidJsonObjectToJSIValue(
    Runtime &runtime, rapidjson::Value &value) {
  switch (value.GetType()) {
    case rapidjson::kStringType:
      return convertRapidJsonStringToJSIValue(runtime, value);
    case rapidjson::kNumberType:
      return convertRapidJsonNumberToJSIValue(runtime, value);
    case rapidjson::kNullType:
      return piper::Value::null();
    case rapidjson::kFalseType:
      return piper::Value(false);
    case rapidjson::kTrueType:
      return piper::Value(true);
    case rapidjson::kArrayType: {
      int length = value.Size();
      auto array_opt =
          piper::Array::createWithLength(runtime, static_cast<size_t>(length));
      if (!array_opt) {
        return piper::Value();
      }
      for (int index = 0; index < length; index++) {
        array_opt->setValueAtIndex(
            runtime, index,
            convertRapidJsonObjectToJSIValue(runtime, value[index]));
      }
      return piper::Value(*array_opt);
    }
    case rapidjson::kObjectType: {
      auto lynx_val_obj =
          convertRapidJsonLynxValObjectToJSIValue(runtime, value);
      if (lynx_val_obj) {
        return piper::Value(runtime, *lynx_val_obj);
      }
      piper::Object v(runtime);
      for (auto p = value.GetObject().begin(); p != value.GetObject().end();
           p++) {
        v.setProperty(runtime, (*p).name.GetString(),
                      convertRapidJsonObjectToJSIValue(runtime, (*p).value));
      }
      return v;
    }
    default:
      return piper::Value::undefined();
  }
}

}  // namespace piper
}  // namespace lynx
