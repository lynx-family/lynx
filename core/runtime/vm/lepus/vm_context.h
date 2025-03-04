// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_VM_CONTEXT_H_
#define CORE_RUNTIME_VM_LEPUS_VM_CONTEXT_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/value/base_string.h"
#include "base/include/vector.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/function.h"
#include "core/runtime/vm/lepus/heap.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
class TemplateBinaryReader;
class TemplateEntry;
}  // namespace tasm

namespace lepus {
class OutputStream;
class VMContextBundle;
class VMContext : public Context {
 public:
  VMContext()
      : Context(VMContextType),
        current_frame_(nullptr),
        enable_strict_check_(false),
        enable_top_var_strict_mode_(true),
        closures_(),
        block_context_() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "VMContext::VMContext");
  }
  ~VMContext() override;
  virtual void Initialize() override;
  virtual bool Execute(Value* ret = nullptr) override;

  virtual bool UpdateTopLevelVariableByPath(base::Vector<std::string>& path,
                                            const Value& value) override;
  virtual bool CheckTableShadowUpdatedWithTopLevelVariable(
      const lepus::Value& update) override;

  virtual void ResetTopLevelVariable() override;
  virtual void ResetTopLevelVariableByVal(const Value& val) override;

  virtual std::unique_ptr<lepus::Value> GetTopLevelVariable(
      bool ignore_callable = false) override;
  virtual bool GetTopLevelVariableByName(const base::String& name,
                                         lepus::Value* ret) override;

  LEPUS_INLINE long GetParamsSize() override final {
    return heap().top_ - current_frame_->register_;
  }

  LEPUS_INLINE Value* GetParam(long index) override final {
    return current_frame_->register_ + index;
  }

  void CleanClosuresInCycleReference() override;
  int32_t CallFunction(Value* function, size_t argc, Value* ret);

  BASE_EXPORT_FOR_DEVTOOL Frame* GetCurrentFrame();
  BASE_EXPORT_FOR_DEVTOOL fml::RefPtr<Function> GetRootFunction();
  // for deserialize
  void SetRootFunction(fml::RefPtr<Function> func) {
    root_function_ = std::move(func);
  }

#ifdef LEPUS_TEST
  void Dump();
#endif

  void PushCurrentContext(long current_context) {
    current_context_.push(current_context);
  }

  long PopCurrentContextReg() {
    DCHECK(!current_context_.empty());
    long last_context = current_context_.top();
    current_context_.pop();
    return last_context;
  }

  long GetCurrentContextReg() {
    if (current_context_.empty()) {
      return -1;
    }
    return current_context_.top();
  }

  void SetArrayPrototype(Value proto) { array_prototype_ = proto; }

  void SetDatePrototype(Value proto) { date_prototype_ = proto; }

  void SetStringPrototype(Value proto) { string_prototype_ = proto; }

  void SetRegexpPrototype(Value proto) { regexp_prototype_ = proto; }

  void SetNumberPrototype(Value proto) { number_prototype_ = proto; }

  void SetEnableStrictCheck(bool val) { enable_strict_check_ = val; }

  void SetEnableTopVarStrictMode(bool val) {
    enable_top_var_strict_mode_ = val;
  }

  void SetNullPropAsUndef(bool val) { enable_null_prop_as_undef_ = val; }

  void SetClosureFix(bool val) { closure_fix_ = val; }
  bool GetClosureFix() { return closure_fix_; }

  inline Global* global() { return &global_; }
  inline Global* builtin() { return &builtin_; }
  void SetGlobalData(const base::String& name, Value value) override;
  lepus::Value GetGlobalData(const base::String& name) override;

  void SetBuiltinData(const base::String& name, Value value) {
    builtin_.Add(name, std::move(value));
  }
  Value* SearchGlobalData(const base::String& name) {
    return global_.Find(name);
  }

  static VMContext* Cast(Context* context) {
    DCHECK(context->IsVMContext());
    return static_cast<VMContext*>(context);
  }

  virtual void RegisterMethodToLynx() override;

  virtual void RegisterLepusVerion() override;

  bool DeSerialize(const ContextBundle& bundle, bool, Value* ret,
                   const char* file_name = nullptr) override;
  bool MoveContextBundle(VMContextBundle& bundle);
  void RegisterCtxBuiltin(const tasm::ArchOption&) override;
  void ApplyConfig(const std::shared_ptr<tasm::PageConfig>&,
                   const tasm::CompileOptions&) override;

  lepus::Value ReportFatalError(const std::string& error_message, bool exit,
                                int32_t code) override;

  virtual lepus::Value GetCurrentThis(lepus::Value* argv,
                                      int32_t offset) override;

  class DebugDelegate {
   public:
    virtual ~DebugDelegate() = default;
    virtual void UpdateCurrentPC(int32_t current_pc) = 0;
    virtual int32_t GenerateDebuggerFrameId() { return 0; }
  };
  void SetDebugDelegate(const std::shared_ptr<DebugDelegate>& debug_delegate) {
    is_debug_enabled_ = true;
    debug_delegate_ = debug_delegate;
  }

 private:
  // used to control closure context
  class ContextScope {
   public:
    ContextScope(VMContext* ctx, const fml::RefPtr<lepus::Closure>& closure)
        : ctx_(ctx) {
      last_closure_context_ = ctx->PrepareClosureContext(closure);
    }
    ~ContextScope() { ctx_->closure_context_ = last_closure_context_; }

   private:
    VMContext* ctx_;
    Value last_closure_context_;
  };

  class ClosureManager {
   public:
    void AddClosure(fml::RefPtr<lepus::Closure>& closure,
                    bool context_executed);
    ~ClosureManager();
    ClosureManager() : itr_(0){};
    void CleanUpClosuresCreatedAfterExecuted();

   private:
    void ClearClosure();
    std::vector<fml::RefPtr<lepus::Closure>> all_closures_before_executed_;
    std::vector<fml::RefPtr<lepus::Closure>> all_closures_after_executed_;
    std::vector<fml::RefPtr<lepus::Closure>>::size_type itr_;
  };

  Value* CallPrologue(const base::String& name);
  Value CallEpilogue(Value* function, size_t arg_count);

  virtual Value CallArgs(const base::String& name, const Value* args[],
                         size_t args_count,
                         bool pause_suppression_mode) override;
  virtual Value CallClosureArgs(const Value& closure, const Value* args[],
                                size_t args_count) override;

  void RunFrame();
  void GenerateClosure(Value* value, long index);
  Value PrepareClosureContext(const fml::RefPtr<lepus::Closure>& clo);
  void ReportException(const std::string& exception_info, int& pc,
                       int& instruction_length,
                       fml::RefPtr<Closure>& current_frame_closure,
                       Function*& current_frame_function,
                       const Instruction*& current_frame_base,
                       Value*& current_frame_regs, bool report_logbox,
                       int32_t err_code = error::E_MTS_RUNTIME_ERROR);
  void ReportLogBox(const std::string& exception_info, int& pc);

  std::string BuildBackTrace(const base::Vector<int>& pcs,
                             Frame* exception_frame_);
  Heap heap_;
  Frame* current_frame_;
  base::Stack<long> current_context_;  // Used by code generator only.

  // for debug
  std::weak_ptr<DebugDelegate> debug_delegate_;
  bool is_debug_enabled_{false};

 protected:
  friend class CodeGenerator;
  friend class ContextBinaryWriter;
  friend class LexicalFunction;
  friend class ContextScope;

  friend class tasm::TemplateEntry;
  friend class tasm::TemplateBinaryReader;
  Heap& heap() { return heap_; }

  Global global_;
  Global builtin_;

  std::unordered_map<base::String, long> top_level_variables_;
  fml::RefPtr<Function> root_function_;
  base::InlineStack<Value, 32> context_;
  Value closure_context_;
  std::string exception_info_;
  bool enable_strict_check_;
  bool enable_top_var_strict_mode_;
  bool enable_null_prop_as_undef_ = false;
  bool closure_fix_ = false;

  Value array_prototype_;
  Value date_prototype_;
  Value string_prototype_;
  Value regexp_prototype_;
  Value number_prototype_;

  bool executed_ = false;

  ClosureManager closures_;
  base::InlineStack<Value, 16> block_context_;

  std::optional<std::string> current_exception_{};
  int32_t err_code_ = error::E_MTS_RUNTIME_ERROR;

 private:
  // To reduce arguments need to be passed.
  struct RunFrameContext {
    Value*& a;
    Value*& b;
    Value*& c;
    Value*& regs;
    Instruction i;
  };

  // Extract unlike paths of RunFrame for PGO to reduce binary size expansion.
  void RunFrame_Op_Neg_UnlikelyPath(Value*& a);
  void RunFrame_Op_Pos(Value*& a);
  void RunFrame_Op_Add_UnlikelyPath_B_Number(Value*& a, Value*& b, Value*& c);
  void RunFrame_Op_Add_UnlikelyPath_C_Number(Value*& a, Value*& b, Value*& c);
  void RunFrame_Op_Mod(RunFrameContext& ctx);
  void RunFrame_Op_Pow(RunFrameContext& ctx);
  void RunFrame_Op_BitOr(RunFrameContext& ctx);
  void RunFrame_Op_BitAnd(RunFrameContext& ctx);
  void RunFrame_Op_BitXor(RunFrameContext& ctx);
  void RunFrame_Op_GetTable_UnlikelyPath_String(Value*& a, Value*& b,
                                                Value*& c);
  void RunFrame_Op_CreateBlockContext(RunFrameContext& ctx);
  void RunFrame_Label_EnterBlock(fml::RefPtr<Closure>& closure);
  void RunFrame_Label_LeaveBlock();
};

class VMContextBundle final : public ContextBundle {
 public:
  VMContextBundle() = default;
  virtual ~VMContextBundle() override = default;
  virtual bool IsLepusNG() const override;

  std::unordered_map<base::String, lepus::Value>& lepus_root_global() {
    return lepus_root_global_;
  }
  std::unordered_map<base::String, long>& lepus_top_variables() {
    return lepus_top_variables_;
  }
  fml::RefPtr<lepus::Function>& lepus_root_function() {
    return lepus_root_function_;
  }

 private:
  std::unordered_map<base::String, lepus::Value> lepus_root_global_{};
  std::unordered_map<base::String, long> lepus_top_variables_{};
  fml::RefPtr<lepus::Function> lepus_root_function_{Function::Create()};

  friend class VMContextDecoder;
  friend class VMContext;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_VM_CONTEXT_H_
