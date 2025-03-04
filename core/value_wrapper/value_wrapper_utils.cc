// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/value_wrapper/value_wrapper_utils.h"

#include "core/base/js_constants.h"
#include "core/runtime/common/utils.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "core/value_wrapper/value_impl_piper.h"

namespace lynx {
namespace pub {

lepus::Value ValueUtils::ConvertValueToLepusValue(
    const Value& value,
    std::vector<std::unique_ptr<pub::Value>>* prev_value_vector, int depth) {
  if (value.backend_type() == pub::ValueBackendType::ValueBackendTypeLepus) {
    return (reinterpret_cast<const PubLepusValue*>(&value))->backend_value();
  } else {
    lepus::Value res;
    if (value.IsString()) {
      res = lepus::Value(value.str());
    } else if (value.IsBool()) {
      res = lepus::Value(value.Bool());
    } else if (value.IsInt32()) {
      res = lepus::Value(value.Int32());
    } else if (value.IsUInt32()) {
      res = lepus::Value(value.UInt32());
    } else if (value.IsInt64()) {
      res = lepus::Value(value.Int64());
    } else if (value.IsUInt64()) {
      res = lepus::Value(value.UInt64());
    } else if (value.IsNumber()) {
      res = lepus::Value(value.Number());
    } else if (value.IsArrayBuffer()) {
      int length = value.Length();
      std::unique_ptr<uint8_t[]> copy = std::make_unique<uint8_t[]>(length);
      memcpy(copy.get(), value.ArrayBuffer(), length);
      auto byte_array = lepus::ByteArray::Create(std::move(copy), length);
      res = lepus::Value(byte_array);
    } else if (value.IsArray()) {
      ScopedCircleChecker scoped_circle_checker;
      if (!scoped_circle_checker.CheckCircleOrCacheValue(prev_value_vector,
                                                         value, depth)) {
        res = ConvertValueToLepusArray(value, prev_value_vector, depth + 1);
      } else {
        res = lepus::Value();
        res.SetUndefined();
      }
    } else if (value.IsMap()) {
      ScopedCircleChecker scoped_circle_checker;
      if (!scoped_circle_checker.CheckCircleOrCacheValue(prev_value_vector,
                                                         value, depth)) {
        res = ConvertValueToLepusTable(value, prev_value_vector, depth + 1);
      } else {
        res = lepus::Value();
        res.SetUndefined();
      }
    } else if (value.IsUndefined()) {
      res = lepus::Value();
      res.SetUndefined();
    } else if (value.IsNil()) {
      res = lepus::Value();
      res.SetNil();
    }
    return res;
  }
}

lepus::Value ValueUtils::ConvertValueToLepusArray(
    const Value& value,
    std::vector<std::unique_ptr<pub::Value>>* prev_value_vector, int depth) {
  if (value.backend_type() == pub::ValueBackendType::ValueBackendTypeLepus) {
    return (reinterpret_cast<const PubLepusValue*>(&value))->backend_value();
  } else {
    auto array = lepus::CArray::Create();
    value.ForeachArray([&array, &prev_value_vector, depth](
                           int64_t index, const pub::Value& val) {
      array->emplace_back(
          ConvertValueToLepusValue(val, prev_value_vector, depth + 1));
    });
    return lepus::Value(std::move(array));
  }
}

lepus::Value ValueUtils::ConvertValueToLepusTable(
    const Value& value,
    std::vector<std::unique_ptr<pub::Value>>* prev_value_vector, int depth) {
  if (value.backend_type() == pub::ValueBackendType::ValueBackendTypeLepus) {
    return (reinterpret_cast<const PubLepusValue*>(&value))->backend_value();
  } else {
    auto dict = lepus::Dictionary::Create();
    value.ForeachMap([&dict, &prev_value_vector, depth](const pub::Value& key,
                                                        const pub::Value& val) {
      dict->SetValue(key.str(),
                     ConvertValueToLepusValue(val, prev_value_vector, depth));
    });
    return lepus::Value(std::move(dict));
  }
}

piper::Value ValueUtils::ConvertValueToPiperValue(piper::Runtime& rt,
                                                  const Value& value) {
  if (value.backend_type() == ValueBackendType::ValueBackendTypePiper) {
    return piper::Value(
        rt, static_cast<const ValueImplPiper&>(value).backend_value());
  } else if (value.backend_type() == ValueBackendType::ValueBackendTypeLepus) {
    auto result = piper::valueFromLepus(
        rt, static_cast<const PubLepusValue&>(value).backend_value(), nullptr);
    if (result.has_value()) {
      return std::move(*result);
    }
  } else {
    if (value.IsString()) {
      auto result = piper::String::createFromUtf8(rt, value.str());
      return piper::Value(result);
    } else if (value.IsBool()) {
      return piper::Value(value.Bool());
    } else if (value.IsInt32()) {
      return piper::Value(value.Int32());
    } else if (value.IsUInt32()) {
      return piper::Value(static_cast<double>(value.UInt32()));
    } else if (value.IsInt64()) {
      int64_t int64_value = value.Int64();
      // In JavaScript,  the max safe integer is 9007199254740991 and the min
      // safe integer is -9007199254740991, so when integer beyond limit, use
      // BigInt Object to define it. More information from
      // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number
      if (int64_value < piper::kMinJavaScriptNumber ||
          int64_value > piper::kMaxJavaScriptNumber) {
        auto bigint =
            piper::BigInt::createWithString(rt, std::to_string(int64_value));
        return bigint ? piper::Value(*bigint) : piper::Value();
      } else {
        return piper::Value(value.Number());
      }
    } else if (value.IsUInt64()) {
      uint64_t uint64_value = value.UInt64();
      if (uint64_value > piper::kMaxJavaScriptNumber) {
        auto bigint =
            piper::BigInt::createWithString(rt, std::to_string(uint64_value));
        return bigint ? piper::Value(*bigint) : piper::Value();
      } else {
        return piper::Value(value.Number());
      }
    } else if (value.IsNumber()) {
      return piper::Value(value.Number());
    } else if (value.IsArray()) {
      return ConvertValueToPiperArray(rt, value);
    } else if (value.IsMap()) {
      return ConvertValueToPiperObject(rt, value);
    } else if (value.IsArrayBuffer()) {
      uint8_t* bytes = value.ArrayBuffer();
      piper::ArrayBuffer buffer =
          piper::ArrayBuffer(rt, static_cast<const uint8_t*>(bytes),
                             static_cast<size_t>(value.Length()));
      return piper::Value(buffer);
    } else if (value.IsNil()) {
      return piper::Value(nullptr);
    }
  }
  return piper::Value();
}

piper::Value ValueUtils::ConvertValueToPiperArray(piper::Runtime& rt,
                                                  const Value& value) {
  if (value.backend_type() == ValueBackendType::ValueBackendTypePiper) {
    return piper::Value(
        rt, (reinterpret_cast<const ValueImplPiper*>(&value))->backend_value());
  } else {
    auto array = piper::Array::createWithLength(rt, value.Length());
    if (!array) {
      return piper::Value();
    }
    value.ForeachArray([&array, &rt](int64_t index, const Value& val) {
      array->setValueAtIndex(rt, static_cast<size_t>(index),
                             ConvertValueToPiperValue(rt, val));
    });
    return piper::Value(std::move(*array));
  }
}

piper::Value ValueUtils::ConvertValueToPiperObject(piper::Runtime& rt,
                                                   const Value& value) {
  if (value.backend_type() == ValueBackendType::ValueBackendTypePiper) {
    return piper::Value(
        rt, (reinterpret_cast<const ValueImplPiper*>(&value))->backend_value());
  } else {
    piper::Object object(rt);
    value.ForeachMap([&object, &rt](const Value& key, const Value& val) {
      object.setProperty(rt, key.str().c_str(),
                         ConvertValueToPiperValue(rt, val));
    });
    return piper::Value(std::move(object));
  }
}

bool ValueUtils::IsBigInt(piper::Runtime& rt, const piper::Object& obj) {
  auto big_int_opt = obj.getProperty(rt, piper::BIG_INT_VAL);
  if (big_int_opt && big_int_opt->isString()) {
    return true;
  }
  return false;
}

// static
bool ValueUtils::ConvertBigIntToStringIfNecessary(piper::Runtime& rt,
                                                  const piper::Object& obj,
                                                  std::string& result) {
  auto big_int_opt = obj.getProperty(rt, piper::BIG_INT_VAL);
  // getProperty will return undefined if there is no value for giving key.
  if (!big_int_opt || big_int_opt->isUndefined()) {
    return false;
  }
  auto big_int = big_int_opt->asString(rt);
  if (big_int) {
    result = big_int->utf8(rt);
  } else {
    rt.reportJSIException(
        BUILD_JSI_NATIVE_EXCEPTION("try to get bigint from js value fail!"));
  }
  return true;
}

// static
std::unique_ptr<uint8_t[]> ValueUtils::ConvertPiperToArrayBuffer(
    piper::Runtime& rt, const piper::Object& o, size_t& length) {
  auto buf = o.getArrayBuffer(rt);
  length = buf.size(rt);
  uint8_t* buffer = buf.data(rt);
  std::unique_ptr<uint8_t[]> result = std::make_unique<uint8_t[]>(length);
  memcpy(result.get(), buffer, length);
  return result;
}

// static
std::unique_ptr<Value> ValueUtils::ConvertPiperArrayToPubValue(
    piper::Runtime& rt, const piper::Array& arr,
    const std::shared_ptr<PubValueFactory>& factory) {
  lynx::piper::Scope scope(rt);
  auto result = factory->CreateArray();
  std::optional<size_t> size = arr.size(rt);
  if (!size) {
    return result;
  }
  for (size_t index = 0; index < *size; index++) {
    std::optional<lynx::piper::Value> ov = arr.getValueAtIndex(rt, index);
    if (!ov) {
      continue;
    }
    if (ov->isBool()) {
      result->PushBoolToArray(ov->getBool());
    } else if (ov->isNumber()) {
      result->PushDoubleToArray(ov->getNumber());
    } else if (ov->isNull() || ov->isUndefined()) {
      result->PushNullToArray();
    } else if (ov->isString()) {
      result->PushStringToArray(ov->getString(rt).utf8(rt));
    } else if (ov->isObject()) {
      lynx::piper::Object o = ov->getObject(rt);
      if (o.isArray(rt)) {
        auto sub_arr = o.getArray(rt);
        auto sub_arr_result = ConvertPiperArrayToPubValue(rt, sub_arr, factory);
        result->PushValueToArray(std::move(sub_arr_result));
      } else if (o.isArrayBuffer(rt)) {
        size_t length;
        result->PushArrayBufferToArray(ConvertPiperToArrayBuffer(rt, o, length),
                                       length);
      } else if (o.isFunction(rt)) {
        LOGW("not support function");
        result->PushNullToArray();
      } else {
        std::string r;
        auto ret = ConvertBigIntToStringIfNecessary(rt, o, r);
        if (ret) {
          result->PushBigIntToArray(r);
          continue;
        }
        auto dict = ConvertPiperObjectToPubValue(rt, o, factory);
        result->PushValueToArray(std::move(dict));
      }
    }
  }
  return result;
}

// static
std::unique_ptr<Value> ValueUtils::ConvertPiperObjectToPubValue(
    piper::Runtime& rt, const piper::Object& obj,
    const std::shared_ptr<PubValueFactory>& factory) {
  lynx::piper::Scope scope(rt);
  auto result = factory->CreateMap();
  auto array = obj.getPropertyNames(rt);
  if (!array) {
    return result;
  }
  std::optional<size_t> size = array->size(rt);
  if (!size) {
    return result;
  }
  for (size_t index = 0; index < *size; index++) {
    auto k = array->getValueAtIndex(rt, index);
    if (!k) {
      continue;
    }
    auto piper_key = k->getString(rt);
    auto key = piper_key.utf8(rt);
    auto ov = obj.getProperty(rt, piper_key);
    if (!ov) {
      continue;
    }
    if (ov->isBool()) {
      result->PushBoolToMap(key, ov->getBool());
    } else if (ov->isNumber()) {
      result->PushDoubleToMap(key, ov->getNumber());
    } else if (ov->isNull() || ov->isUndefined()) {
      // Do not save null or undefined types, align with old data conversion
      // types, and avoid breakchanges
      //      result->PushNullToMap(key);
    } else if (ov->isString()) {
      result->PushStringToMap(key, ov->getString(rt).utf8(rt));
    } else if (ov->isObject()) {
      lynx::piper::Object o = ov->getObject(rt);
      if (o.isArray(rt)) {
        auto sub_arr = o.getArray(rt);
        auto sub_arr_result = ConvertPiperArrayToPubValue(rt, sub_arr, factory);
        result->PushValueToMap(key, std::move(sub_arr_result));
      } else if (o.isArrayBuffer(rt)) {
        size_t length;
        result->PushArrayBufferToMap(
            key, ConvertPiperToArrayBuffer(rt, o, length), length);
      } else if (o.isFunction(rt)) {
        LOGW("not support function");
        result->PushNullToMap(key);
      } else {
        std::string r;
        auto ret = ConvertBigIntToStringIfNecessary(rt, o, r);
        if (ret) {
          result->PushBigIntToMap(key, r);
          continue;
        }
        auto dict = ConvertPiperObjectToPubValue(rt, o, factory);
        result->PushValueToMap(key, std::move(dict));
      }
    }
  }
  return result;
}

}  // namespace pub

}  // namespace lynx
