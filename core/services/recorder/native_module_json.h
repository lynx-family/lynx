// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_NATIVE_MODULE_JSON_H_
#define CORE_SERVICES_RECORDER_NATIVE_MODULE_JSON_H_

#include <cmath>
#include <vector>

#include "core/runtime/js/jsi/jsi.h"
#include "core/services/recorder/recorder_constants.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace recorder {

// Shared argument representation for recording and replay. Allocation belongs
// to the caller; this helper does not depend on the recorder singleton.
inline rapidjson::Value NativeModuleValueToJson(
    const runtime::js::Value& res, runtime::js::Runtime* rt,
    rapidjson::Document::AllocatorType& allocator,
    std::vector<const runtime::js::Object*>* visited_objs) {
  runtime::js::Scope scope(*rt);

  rapidjson::Value return_val(rapidjson::kNullType);

  if (res.isBool()) {
    return_val.SetBool(res.getBool());
  } else if (res.isNumber()) {
    double number = res.getNumber();
    if (std::isnan(number)) {
      return_val.SetString("NaN", allocator);
    } else {
      return_val.SetDouble(number);
    }
  } else if (res.isString()) {
    return_val.SetString(res.getString(*rt).utf8(*rt), allocator);
  } else if (res.isSymbol()) {
    return_val.SetString(*res.getSymbol(*rt).toString(*rt), allocator);
  } else if (res.isObject()) {
    runtime::js::Object piper_obj = res.getObject(*rt);

    // Preserve the recorder representation without invoking toJSON().
    if (visited_objs != nullptr) {
      for (const auto* obj : *visited_objs) {
        if (obj && runtime::js::Object::strictEquals(*rt, *obj, piper_obj)) {
          return_val.SetString("[Circular Reference]", allocator);
          return return_val;
        }
      }
    }
    struct VisitedGuard {
      std::vector<const runtime::js::Object*>* objects;
      ~VisitedGuard() {
        if (objects) objects->pop_back();
      }
    } guard{visited_objs};
    if (visited_objs) visited_objs->push_back(&piper_obj);

    if (piper_obj.isArray(*rt)) {
      // Parse Array
      return_val.SetArray();
      runtime::js::Array piper_array = piper_obj.getArray(*rt);
      auto array_size = piper_array.size(*rt);
      if (array_size) {
        for (size_t index = 0; index != *array_size; index++) {
          auto val_opt = piper_array.getValueAtIndex(*rt, index);
          if (!val_opt) {
            return return_val;
          }
          return_val.PushBack(
              NativeModuleValueToJson(*val_opt, rt, allocator, visited_objs),
              allocator);
        }
      }
    } else if (piper_obj.isArrayBuffer(*rt)) {
      // Parse Array Buffer
      auto buffer = piper_obj.getArrayBuffer(*rt);
      if (buffer.size(*rt) != 0) {
        return_val.SetUint(static_cast<uint32_t>(*buffer.data(*rt)));
      }

    } else if (piper_obj.isFunction(*rt)) {
      // TODO(kechenglong): parse function if needed
      return_val.SetString(kParamFunction, allocator);

    } else if (piper_obj.isHostObject(*rt)) {
      // Parse HostObject
      return_val.SetObject();
      auto weak_host_obj = piper_obj.getHostObject(*rt);
      auto host_obj = weak_host_obj.lock();
      if (!host_obj) {
        return return_val;
      }
      auto property_names = host_obj->getPropertyNames(*rt);
      for (auto& property_name : property_names) {
        rapidjson::Value key(rapidjson::kStringType);
        key.SetString(property_name.utf8(*rt), allocator);
        runtime::js::Value piper_val = host_obj->get(rt, property_name);
        rapidjson::Value val =
            NativeModuleValueToJson(piper_val, rt, allocator, visited_objs);
        return_val.AddMember(key, val, allocator);
      }

    } else {
      // Parse Object
      auto property_names_array_opt = piper_obj.getPropertyNames(*rt);
      if (!property_names_array_opt) {
        return return_val;
      }
      auto array_size_opt = property_names_array_opt->size(*rt);
      if (!array_size_opt) {
        return return_val;
      }
      return_val.SetObject();
      for (size_t index = 0; index != *array_size_opt; index++) {
        auto property_name =
            property_names_array_opt->getValueAtIndex(*rt, index);
        if (!property_name) {
          return return_val;
        }
        rapidjson::Value key = NativeModuleValueToJson(*property_name, rt,
                                                       allocator, visited_objs);
        auto piper_val =
            piper_obj.getProperty(*rt, property_name->getString(*rt));
        if (!piper_val) {
          return return_val;
        }
        rapidjson::Value val =
            NativeModuleValueToJson(*piper_val, rt, allocator, visited_objs);
        return_val.AddMember(key, val, allocator);
      }
    }
  } else {
    if (res.isNull()) {
      return_val.SetNull();
    } else {
      return_val.SetString(res.toString(*rt)->utf8(*rt), allocator);
    }
  }

  return return_val;
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_RECORDER_NATIVE_MODULE_JSON_H_
