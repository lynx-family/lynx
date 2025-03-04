// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/big_int/jsbi.h"

#include <string>

#include "base/include/expected.h"

namespace lynx {
namespace piper {

#define REGISTER_BIGINT_METHOD(method, operator)                              \
  do {                                                                        \
    if (methodName.compare(operator) == 0) {                                  \
      return Function::createFromHostFunction(                                \
          *rt, PropNameID::forAscii(*rt, operator), 0,                        \
          [this](Runtime& rt, const Value& thisVal, const Value* args,        \
                 size_t count) -> base::expected<Value, JSINativeException> { \
            return method(&rt, args, count, operator);                        \
          });                                                                 \
    }                                                                         \
  } while (0)

std::vector<PropNameID> JSBI::getPropertyNames(Runtime& rt) {
  std::vector<PropNameID> vec;
  vec.push_back(piper::PropNameID::forUtf8(rt, CONSTRUCTOR_BIG_INT));
  vec.push_back(piper::PropNameID::forUtf8(rt, OPERATOR_ADD));
  vec.push_back(piper::PropNameID::forUtf8(rt, OPERATOR_SUBTRACT));
  vec.push_back(piper::PropNameID::forUtf8(rt, OPERATOR_MULTIPLY));
  vec.push_back(piper::PropNameID::forUtf8(rt, OPERATOR_DIVIDE));
  vec.push_back(piper::PropNameID::forUtf8(rt, OPERATOR_REMAINDER));
  vec.push_back(piper::PropNameID::forUtf8(rt, OPERATOR_EQUAL));
  vec.push_back(piper::PropNameID::forUtf8(rt, OPERATOR_NOT_EQUAL));
  vec.push_back(piper::PropNameID::forUtf8(rt, OPERATOR_LESS_THAN));
  vec.push_back(piper::PropNameID::forUtf8(rt, OPERATOR_LESS_THAN_OR_EQUAL));
  vec.push_back(piper::PropNameID::forUtf8(rt, OPERATOR_GREATER_THAN));
  vec.push_back(piper::PropNameID::forUtf8(rt, OPERATOR_GREATER_THAN_OR_EQUAL));

  return vec;
}

Value JSBI::get(Runtime* rt, const PropNameID& name) {
  const std::string methodName = name.utf8(*rt);

  REGISTER_BIGINT_METHOD(BigInt, CONSTRUCTOR_BIG_INT);
  REGISTER_BIGINT_METHOD(operate, OPERATOR_ADD);
  REGISTER_BIGINT_METHOD(operate, OPERATOR_SUBTRACT);
  REGISTER_BIGINT_METHOD(operate, OPERATOR_MULTIPLY);
  REGISTER_BIGINT_METHOD(operate, OPERATOR_DIVIDE);
  REGISTER_BIGINT_METHOD(operate, OPERATOR_REMAINDER);
  REGISTER_BIGINT_METHOD(operate, OPERATOR_EQUAL);
  REGISTER_BIGINT_METHOD(operate, OPERATOR_NOT_EQUAL);
  REGISTER_BIGINT_METHOD(operate, OPERATOR_LESS_THAN);
  REGISTER_BIGINT_METHOD(operate, OPERATOR_LESS_THAN_OR_EQUAL);
  REGISTER_BIGINT_METHOD(operate, OPERATOR_GREATER_THAN);
  REGISTER_BIGINT_METHOD(operate, OPERATOR_GREATER_THAN_OR_EQUAL);

  return piper::Value::undefined();
}

base::expected<Value, JSINativeException> JSBI::BigInt(
    Runtime* rt, const Value* args, size_t count,
    const std::string& func_name) {
  piper::Scope scope(*rt);
  if (count > 0) {
    const Value* value = &args[0];
    std::string number;

    if (value->isString()) {
      number = value->getString(*rt).utf8(*rt);
    } else if (value->isNumber()) {
      // double to long
      number = std::to_string(static_cast<long>(value->getNumber()));
    } else {
      return piper::Value::undefined();
    }

    auto bigint = BigInt::createWithString(*rt, number);
    if (!bigint) {
      return base::unexpected(
          BUILD_JSI_NATIVE_EXCEPTION("BigInt create failed."));
    }
    return piper::Value(*bigint);
  }

  return piper::Value::undefined();
}

base::expected<Value, JSINativeException> JSBI::operate(
    Runtime* rt, const Value* args, size_t count,
    const std::string& func_name) {
  if (count >= 2) {
    piper::Scope scope(*rt);
    const Value* num1 = &args[0];
    const Value* num2 = &args[1];

    if (num1->isObject() && num2->isObject()) {
      auto num1_value = num1->getObject(*rt).getProperty(*rt, BIG_INT_VAL);
      if (!num1_value) {
        return piper::Value::undefined();
      }
      const std::string num1_str = num1_value->getString(*rt).utf8(*rt);

      auto num2_value = num2->getObject(*rt).getProperty(*rt, BIG_INT_VAL);
      if (!num2_value) {
        return piper::Value::undefined();
      }
      const std::string num2_str = num2_value->getString(*rt).utf8(*rt);

      BigInteger big_int_num1 = BigInteger(num1_str);
      BigInteger big_int_num2 = BigInteger(num2_str);

      std::optional<piper::BigInt> bigint_res;
      if (func_name.compare(OPERATOR_ADD) == 0) {
        BigInteger res = big_int_num1 + big_int_num2;

        bigint_res = BigInt::createWithString(*rt, res.toString());
      } else if (func_name.compare(OPERATOR_SUBTRACT) == 0) {
        BigInteger res = big_int_num1 - big_int_num2;

        bigint_res = BigInt::createWithString(*rt, res.toString());
      } else if (func_name.compare(OPERATOR_MULTIPLY) == 0) {
        BigInteger res = big_int_num1 * big_int_num2;

        bigint_res = BigInt::createWithString(*rt, res.toString());
      } else if (func_name.compare(OPERATOR_DIVIDE) == 0) {
        BigInteger res = big_int_num1 / big_int_num2;

        bigint_res = BigInt::createWithString(*rt, res.toString());
      } else if (func_name.compare(OPERATOR_REMAINDER) == 0) {
        BigInteger res = big_int_num1 % big_int_num2;

        bigint_res = BigInt::createWithString(*rt, res.toString());
      } else if (func_name.compare(OPERATOR_EQUAL) == 0) {
        bool isEqual = big_int_num1 == big_int_num2;

        return piper::Value(isEqual);
      } else if (func_name.compare(OPERATOR_NOT_EQUAL) == 0) {
        bool isNotEqual = big_int_num1 != big_int_num2;

        return piper::Value(isNotEqual);
      } else if (func_name.compare(OPERATOR_LESS_THAN) == 0) {
        bool isLessThan = big_int_num1 < big_int_num2;

        return piper::Value(isLessThan);
      } else if (func_name.compare(OPERATOR_LESS_THAN_OR_EQUAL) == 0) {
        bool isLessThanOrEqual = big_int_num1 <= big_int_num2;

        return piper::Value(isLessThanOrEqual);
      } else if (func_name.compare(OPERATOR_GREATER_THAN) == 0) {
        bool isGreaterThan = big_int_num1 > big_int_num2;

        return piper::Value(isGreaterThan);
      } else if (func_name.compare(OPERATOR_GREATER_THAN_OR_EQUAL) == 0) {
        bool isGreaterThanOrEqual = big_int_num1 >= big_int_num2;

        return piper::Value(isGreaterThanOrEqual);
      }
      if (bigint_res) {
        return *bigint_res;
      }
      return base::unexpected(
          BUILD_JSI_NATIVE_EXCEPTION("Invalid BigInt operator."));
    } else {
      return piper::Value::undefined();
    }
  }

  return piper::Value::undefined();
}

}  // namespace piper
}  // namespace lynx
