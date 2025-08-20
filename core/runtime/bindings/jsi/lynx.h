// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_LYNX_H_
#define CORE_RUNTIME_BINDINGS_JSI_LYNX_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/runtime/bindings/jsi/js_app.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
class LynxProxy : public HostObject {
 public:
  LynxProxy(std::weak_ptr<App> app);
  ~LynxProxy() = default;

  virtual Value get(Runtime*, const PropNameID& name) override;
  virtual void set(Runtime*, const PropNameID& name,
                   const Value& value) override;
  virtual std::vector<PropNameID> getPropertyNames(Runtime& rt) override;

 private:
  std::weak_ptr<App> native_app_;
  std::unordered_map<std::string, base::MoveOnlyClosure<Value, Runtime*>>
      dispatch_map_;

  void Init();

  piper::Value GetCustomSectionSync(Runtime& rt, const char* prop_name);
  piper::Value LoadScript(Runtime& rt);

  piper::Value FetchBundle(Runtime& rt);
};
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_LYNX_H_
