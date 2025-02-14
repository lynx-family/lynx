// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/napi/napi_value.h"

#include "third_party/binding/napi/napi_object.h"
#include "third_party/binding/napi/shim/shim_napi.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace binding {

Napi::Value ToNAPI(Value&& value, Napi::Env env) {
  switch (value.GetType()) {
    case ValueType::kEmpty:
      return Napi::Value();
    case ValueType::kNull:
      return env.Null();
    case ValueType::kUndefined:
      return env.Undefined();
    case ValueType::kBoolean:
      return Napi::Boolean::New(env, *value.Data<bool>());
    case ValueType::kNumber:
      return Napi::Number::New(env, *value.Data<double>());
    case ValueType::kString:
      return Napi::String::New(env, *value.Data<std::string>());
    case ValueType::kArray: {
      auto array = Napi::Array::New(env);
      switch (value.GetArrayType()) {
        case ArrayType::kTypeBoolean:
        case ArrayType::kTypeInt8:
        case ArrayType::kTypeUint8:
        case ArrayType::kTypeUint8Clamped:
        case ArrayType::kTypeInt16:
        case ArrayType::kTypeUint16:
        case ArrayType::kTypeInt32: {
          auto* int32_array = value.Data<std::vector<int32_t>>();
          for (uint32_t i = 0; i < int32_array->size(); ++i) {
            array.Set(i, int32_array->at(i));
          }
          break;
        }
        case ArrayType::kTypeUint32: {
          auto* uint32_array = value.Data<std::vector<uint32_t>>();
          for (uint32_t i = 0; i < uint32_array->size(); ++i) {
            array.Set(i, uint32_array->at(i));
          }
          break;
        }
        case ArrayType::kTypeFloat32: {
          auto* float_array = value.Data<std::vector<float>>();
          for (uint32_t i = 0; i < float_array->size(); ++i) {
            array.Set(i, float_array->at(i));
          }
          break;
        }
        case ArrayType::kTypeFloat64: {
          auto* double_array = value.Data<std::vector<double>>();
          for (uint32_t i = 0; i < double_array->size(); ++i) {
            array.Set(i, double_array->at(i));
          }
          break;
        }
        case ArrayType::kTypeString: {
          auto* string_array = value.Data<std::vector<std::string>>();
          for (uint32_t i = 0; i < string_array->size(); ++i) {
            array.Set(i, string_array->at(i));
          }
          break;
        }
        case ArrayType::kTypeValue: {
          auto* value_array = value.Data<std::vector<Value>>();
          for (uint32_t i = 0; i < value_array->size(); ++i) {
            array.Set(i, ToNAPI(std::move(value_array->at(i)), env));
          }
          break;
        }
        default: {
          BINDING_NOTREACHED();
        }
      }
      return array;
    }
    case ValueType::kTypedArray: {
      switch (value.GetArrayType()) {
        case ArrayType::kTypeInt32: {
          auto* int32_array = value.Data<std::vector<int32_t>>();
          Napi::Int32Array array =
              Napi::Int32Array::New(env, int32_array->size());
          memcpy(array.Data(), int32_array->data(),
                 int32_array->size() * sizeof(int32_t));
          return array;
        }
        case ArrayType::kTypeUint32: {
          auto* uint32_array = value.Data<std::vector<uint32_t>>();
          Napi::Uint32Array array =
              Napi::Uint32Array::New(env, uint32_array->size());
          memcpy(array.Data(), uint32_array->data(),
                 uint32_array->size() * sizeof(uint32_t));
          return array;
        }
        case ArrayType::kTypeFloat32: {
          auto* float_array = value.Data<std::vector<float>>();
          Napi::Float32Array array =
              Napi::Float32Array::New(env, float_array->size());
          memcpy(array.Data(), float_array->data(),
                 float_array->size() * sizeof(float));
          return array;
        }
        default: {
          BINDING_NOTREACHED();
        }
      }
    }
    case ValueType::kArrayBufferView: {
      switch (value.GetArrayType()) {
        case ArrayType::kTypeInt32: {
          auto* raw_buffer = value.Data<std::vector<char>>();
          Napi::Int32Array array =
              Napi::Int32Array::New(env, raw_buffer->size() / sizeof(int32_t));
          memcpy(array.Data(), raw_buffer->data(), raw_buffer->size());
          return array;
        }
        case ArrayType::kTypeUint32: {
          auto* raw_buffer = value.Data<std::vector<char>>();
          Napi::Uint32Array array = Napi::Uint32Array::New(
              env, raw_buffer->size() / sizeof(uint32_t));
          memcpy(array.Data(), raw_buffer->data(), raw_buffer->size());
          return array;
        }
        case ArrayType::kTypeFloat32: {
          auto* raw_buffer = value.Data<std::vector<char>>();
          Napi::Float32Array array =
              Napi::Float32Array::New(env, raw_buffer->size() / sizeof(float));
          memcpy(array.Data(), raw_buffer->data(), raw_buffer->size());
          return array;
        }
        default: {
          BINDING_NOTREACHED();
        }
      }
    }
    case ValueType::kArrayBuffer: {
      auto* array_buffer = value.Data<Value::ArrayBufferData>();
      auto rv = Napi::ArrayBuffer::New(env, array_buffer->size_);
      if (array_buffer->data_) {
        // Do not be smart and just copy data as some platform does not support
        // creating ArrayBuffer from external data.
        memcpy(rv.Data(), array_buffer->data_, array_buffer->size_);
      }
      return rv;
    }
    case ValueType::kObject: {
      return ToNAPI(std::move(*value.Data<Object>()));
    }
    case ValueType::kDictionary: {
      Napi::Object obj = Napi::Object::New(env);
      auto* data = value.Data<Value::DictionaryData>();
      for (auto& kv : data->kv) {
        obj.Set(kv.first.c_str(), ToNAPI(std::move(kv.second), env));
      }
      return obj;
    }
    case ValueType::KSet: {
      auto set_constructor = env.Global().Get("Set").As<Napi::Function>();
      auto set = set_constructor.New({});
      auto add = set.As<Napi::Object>().Get("add").As<Napi::Function>();
      switch (value.GetSetType()) {
        case SetType::kTypeString: {
          auto& set_raw = *value.Data<std::unordered_set<std::string>>();
          for (auto& item : set_raw) {
            add.Call(set, { Napi::String::New(env, item) });
          }
          return set;
        }
        default:
          BINDING_NOTREACHED();
      }
    }
  }
}

Napi::Value ToNAPI(Napi::Value value, Napi::Env env) {
  BINDING_DCHECK(value.Env() == env);
  return value;
}

}  // namespace binding
}  // namespace lynx
