// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JS_BINDINGS_GLOBAL_H_
#define CORE_RUNTIME_JS_BINDINGS_GLOBAL_H_

#include <memory>

#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace runtime {
namespace js {
class ConsoleMessagePostMan;

class Global : public HostGlobal {
 public:
  Global() = default;
  ~Global() override;

  void Init(Runtime& runtime, std::shared_ptr<ConsoleMessagePostMan>& post_man,
            const tasm::PageOptions& page_options,
            bool install_shared_host_objects) override;
  void EnsureConsole(Runtime& runtime,
                     std::shared_ptr<ConsoleMessagePostMan>& post_man,
                     const tasm::PageOptions& page_options);
};

}  // namespace js

}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_BINDINGS_GLOBAL_H_
