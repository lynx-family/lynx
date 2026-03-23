// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <fstream>
#include <sstream>

#include "core/runtime/lepus/ir/pass_manager/pass_manager.h"

namespace lynx {
namespace lepus {
namespace ir {

static std::shared_ptr<std::ofstream> GetOutStream(std::string& path) {
  auto out = std::make_shared<std::ofstream>();
  out->open(path, std::ofstream::out | std::ofstream::binary);
  if (LEPUS_UNLIKELY(!out->is_open())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: failed to open dump file for writing");
  }
  return out;
}

bool CFGRawViewer::RunOnModule(ModuleOp* mod) {
  bool changed = false;

  // create directory if need
  if (!std::filesystem::exists(target_dir_))
    std::filesystem::create_directories(target_dir_);

  llvh::for_each(*mod, [&](FuncOp* func) {
    std::string func_path = target_dir_ + "/" + func->GetName();
    auto out = GetOutStream(func_path);
    std::ostringstream os;
    func->Dump(mode_, os);
    *out << os.str();
    out->close();
  });

  return changed;
}

Pass* CreateCFGRawViewer(IRContext* ir_ctx) { return new CFGRawViewer(ir_ctx); }

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
