// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_VALUE_CONVERTER_H_
#define CLAY_LYNX_ADAPTOR_VALUE_CONVERTER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/value/lynx_value_api.h"
#include "clay/public/value.h"
#include "core/public/pub_value.h"

namespace lynx {

class ValueConverter {
 public:
  static clay::Value CreateClayValue(
      const pub::Value& source,
      std::vector<std::unique_ptr<pub::Value>>* prev_value_vector = nullptr,
      int depth = 0);

  static clay::Value CreateClayValue(const uint8_t* data, size_t size);
  static clay::Value CreateClayValue(const uint32_t* data, size_t size);

  static clay::Value CreateClayValue(lynx_api_env env, lynx_value val);
  static lynx_value CreateLynxValue(lynx_api_env env, const clay::Value& val);
  static lynx_value CreateLynxValue(lynx_api_env env,
                                    const clay::Value::Map& map);
  static lynx_value CreateLynxValue(lynx_api_env env,
                                    const clay::Value::Array& array);

 private:
};

}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_VALUE_CONVERTER_H_
