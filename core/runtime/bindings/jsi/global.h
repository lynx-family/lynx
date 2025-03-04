// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_BINDINGS_JSI_GLOBAL_H_
#define CORE_RUNTIME_BINDINGS_JSI_GLOBAL_H_

#include <memory>
#include <mutex>
#include <string>

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {

class ConsoleMessagePostMan;

class Global : public HostGlobal {
 public:
  Global() = default;
  ~Global() override;

  void Init(std::shared_ptr<Runtime>& js_runtime_,
            std::shared_ptr<piper::ConsoleMessagePostMan>& post_man) override;
  void Release() override;
  void EnsureConsole(std::shared_ptr<piper::ConsoleMessagePostMan>& post_man);

 private:
  virtual void SetJSRuntime(std::shared_ptr<Runtime> js_runtime_) = 0;
  virtual std::shared_ptr<Runtime> GetJSRuntime() = 0;
};

class SharedContextGlobal : public Global {
 public:
  SharedContextGlobal() = default;
  ~SharedContextGlobal() override = default;

  void Release() override;

 private:
  virtual void SetJSRuntime(std::shared_ptr<Runtime> js_runtime_) override;
  virtual std::shared_ptr<Runtime> GetJSRuntime() override;
  std::shared_ptr<Runtime> js_runtime_;
};

class SingleGlobal : public Global {
 public:
  SingleGlobal() {}
  virtual ~SingleGlobal();

  void Release() override;

 private:
  virtual void SetJSRuntime(std::shared_ptr<Runtime> js_runtime_) override;
  virtual std::shared_ptr<Runtime> GetJSRuntime() override;
  std::weak_ptr<Runtime> js_runtime_;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_GLOBAL_H_
