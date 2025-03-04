// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_LYNX_H_
#define CORE_RUNTIME_BINDINGS_JSI_LYNX_H_

#include <memory>
#include <vector>

#include "core/runtime/bindings/jsi/js_app.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
class LynxProxy : public HostObject {
 public:
  LynxProxy(std::weak_ptr<Runtime> rt, std::weak_ptr<App> app)
      : rt_(rt), native_app_(app){};
  ~LynxProxy() = default;

  virtual Value get(Runtime*, const PropNameID& name) override;
  virtual void set(Runtime*, const PropNameID& name,
                   const Value& value) override;
  virtual std::vector<PropNameID> getPropertyNames(Runtime& rt) override;

 private:
  std::weak_ptr<Runtime> rt_;
  std::weak_ptr<App> native_app_;

  piper::Value GetCustomSectionSync(Runtime& rt, const char* prop_name);
};
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_LYNX_H_
