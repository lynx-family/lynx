// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_BIG_INT_JSBI_H_
#define CORE_RUNTIME_BINDINGS_JSI_BIG_INT_JSBI_H_

#include <string>
#include <vector>

#include "core/base/js_constants.h"
#include "core/runtime/bindings/jsi/big_int/big_integer.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
class Runtime;
class JSBI : public HostObject {
 public:
  virtual Value get(Runtime*, const PropNameID& name) override;
  virtual std::vector<PropNameID> getPropertyNames(Runtime& rt) override;

 private:
  base::expected<Value, JSINativeException> BigInt(
      Runtime* rt, const Value* args, size_t count,
      const std::string& func_name);

  base::expected<Value, JSINativeException> operate(
      Runtime* rt, const Value* args, size_t count,
      const std::string& func_name);
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_BIG_INT_JSBI_H_
