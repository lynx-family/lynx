// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_PASS_MANAGER_PASS_MANAGER_H_
#define CORE_RUNTIME_LEPUS_IR_PASS_MANAGER_PASS_MANAGER_H_

#include <memory>
#include <string>

#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"

namespace lynx {
namespace lepus {
namespace ir {

/// The pass manager is responsible for running the transformation passes on the
/// whole module and on the functions in the module. The pass manager determines
/// the order of the passes, the order of the functions to be processed and the
/// invalidation of analysis.
class PassManager {
  llvh::SmallVector<std::unique_ptr<Pass>, 16> pipeline_;

 public:
  PassManager(IRContext* ir_ctx) : ir_ctx_(ir_ctx) {}
  ~PassManager() = default;

/// Add a pass by appending its name.
#define PASS(ID, NAME, DESCRIPTION) \
  void Add##ID() { AddPass(Create##ID(ir_ctx_)); }
#include "core/runtime/lepus/ir/pass_manager/passes.def"

  /// Add a pass by name.
  bool AddPassForName(llvh::StringRef name) {
#define PASS(ID, NAME, DESCRIPTION) \
  if (name == NAME) {               \
    Add##ID();                      \
    return true;                    \
  }
#include "core/runtime/lepus/ir/pass_manager/passes.def"
    return false;
  }
  IRContext* GetIRCtx() { return ir_ctx_; }

  /// Add a pass by reference.
  void AddPass(Pass* p);
  void Run(FuncOp* f);
  void Run(ModuleOp* mod);
  void SetMode(StageMode mode) { mode_ = mode; }
  void SetIRDumpPath(const char* path) { ir_dump_path_ = path; }
  StageMode GetMode() const { return mode_; }

 private:
  IRContext* ir_ctx_;
  uint32_t idx_ = 0;
  StageMode mode_ = StageMode::SM_HIR;
  const char* ir_dump_path_ = nullptr;
};

#ifdef LEPUS_TEST
class CFGRawViewer : public ModulePass {
 public:
  explicit CFGRawViewer(IRContext* ir_ctx)
      : ModulePass(ir_ctx, "cfg-raw-viewer") {}
  ~CFGRawViewer() override = default;

  void SetTargetDir(std::string& target_dir) { target_dir_ = target_dir; }

  void SetMode(StageMode mode) { mode_ = mode; }

  void SetPipelineName(const std::string& pipeline_name) {
    pipeline_name_ = pipeline_name;
  }

  bool RunOnModule(ModuleOp* mod) override;

 private:
  std::string target_dir_;
  std::string pipeline_name_;
  StageMode mode_ = StageMode::SM_MIR;
};
#endif

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_PASS_MANAGER_PASS_MANAGER_H_
