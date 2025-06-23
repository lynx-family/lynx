// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DATA_HARMONY_PLATFORM_DATA_HARMONY_H_
#define CORE_RENDERER_DATA_HARMONY_PLATFORM_DATA_HARMONY_H_

#include <string>
#include <utility>

#include "core/renderer/data/platform_data.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

class PlatformDataHarmony : public PlatformData {
 public:
  PlatformDataHarmony() = default;
  ~PlatformDataHarmony() override = default;

  explicit PlatformDataHarmony(const std::string& json_string)
      : is_json_string_(true), json_string_(json_string) {}
  explicit PlatformDataHarmony(lepus::Value&& value) {
    value_converted_from_platform_data_ = std::move(value);
  }

 private:
  void EnsureConvertData() override;

  bool is_json_string_{false};
  std::string json_string_{};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DATA_HARMONY_PLATFORM_DATA_HARMONY_H_
