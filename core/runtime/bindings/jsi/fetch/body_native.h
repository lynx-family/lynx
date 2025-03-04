// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_FETCH_BODY_NATIVE_H_
#define CORE_RUNTIME_BINDINGS_JSI_FETCH_BODY_NATIVE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {

class BodyNative : public HostObject {
 public:
  explicit BodyNative(const std::string& data)
      : data_(data.begin(), data.end()) {}
  explicit BodyNative(std::vector<uint8_t> data) : data_(std::move(data)) {}

  Value get(Runtime* rt, const PropNameID& name) override;

  Value SafeUseBody(Runtime& rt, base::MoveOnlyClosure<Value, Runtime&> use);

  static void RegisterBodyNative(Runtime& rt);

 private:
  bool body_used_{false};
  std::vector<uint8_t> data_{};
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_FETCH_BODY_NATIVE_H_
