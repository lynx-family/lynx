// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/public/value.h"

#include <utility>

namespace clay {

Value::Value(Value&& that) {
  value_ = std::move(that.value_);
  that.value_.emplace<std::monostate>();
  if (IsExternal()) {
    wrapper_ = std::move(that.wrapper_);
  } else {
    wrapper_.reset();
  }
}

Value::Value(std::initializer_list<Value>&& v) {
  Value::Array array;
  for (auto& item : v) {
    array.emplace_back(std::move(const_cast<Value&>(item)));
  }
  value_ = std::make_shared<Array>(std::move(array));
}

Value::Value(std::initializer_list<std::pair<std::string, Value>>&& v) {
  Value::Map map;
  for (auto& item : v) {
    auto& ref = const_cast<std::pair<std::string, Value>&>(item);
    map.emplace(std::move(ref.first), std::move(ref.second));
  }
  value_ = std::make_shared<Map>(std::move(map));
}

Value& Value::operator=(Value&& that) {
  value_ = std::move(that.value_);
  that.value_.emplace<std::monostate>();
  if (IsExternal()) {
    wrapper_ = std::move(that.wrapper_);
  } else {
    wrapper_.reset();
  }
  return *this;
}

Value::~Value() = default;

}  // namespace clay
