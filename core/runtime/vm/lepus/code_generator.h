// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_CODE_GENERATOR_H_
#define CORE_RUNTIME_VM_LEPUS_CODE_GENERATOR_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/renderer/tasm/config.h"
#include "core/runtime/vm/lepus/function.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/op_code.h"
#include "core/runtime/vm/lepus/semantic_analysis.h"
#include "core/runtime/vm/lepus/visitor.h"
#include "core/runtime/vm/lepus/vm_context.h"

namespace lynx {
namespace lepus {

struct VariableNameInfo {
  int register_id_;
  int block_begin_pc_;

  explicit VariableNameInfo(int register_id = 0, int begin_pc = 0)
      : register_id_(register_id), block_begin_pc_(begin_pc) {}
};

enum LoopJmpType {
  LoopJmp_Head,
  LoopJmp_Tail,
  LoopJmp_Continue,
};

enum TryCatchJumpType {
  TryCatchJmp_finally,
};

struct LoopInfo {
  LoopJmpType type_;
  long op_index_;
  LoopInfo(LoopJmpType type, long index) : type_(type), op_index_(index) {}
};

struct TryCatchInfo {
  TryCatchJumpType type_;
  long op_index_;
  TryCatchInfo(TryCatchJumpType type, long index)
      : type_(type), op_index_(index) {}
};

struct TryCatchGenerate {
  fml::RefPtr<Function> function_;
  std::unique_ptr<TryCatchGenerate> parent_;
  std::vector<TryCatchInfo> trycatch_infos_;
  // op index in function
  size_t finally_index_;
  size_t catch_index_;
  size_t exception_finally_index_;

  TryCatchGenerate()
      : function_(nullptr),
        parent_(),
        trycatch_infos_(),
        finally_index_(0),
        catch_index_(0),
        exception_finally_index_(0) {}
};

struct LoopGenerate {
  fml::RefPtr<Function> function_;
  std::unique_ptr<LoopGenerate> parent_;
  std::vector<LoopInfo> loop_infos_;  // jmp head or jmp tail, loop controller
                                      // op index in function
  size_t loop_start_index_;
  size_t loop_continue_index_;

  LoopGenerate()
      : function_(nullptr),
        parent_(),
        loop_infos_(),
        loop_start_index_(0),
        loop_continue_index_(0) {}
};

// uesd to refill optional chaining related instructions' jmp address and result
// register id All MemberAccessorAST and FunctionCallAST should generate the
// following info whether optional or not
struct OptionalChainInfo {
  Function* function_;  // function of the optional chain's related ast
  bool is_optional_;
  std::vector<long>
      jmp_indexes_;  // optional chaining's jmp instructions' index
  std::vector<long>
      move_indexes_;        // optional chaining's LoadNil instructions' index
  long current_jmp_index_;  // current ast's jmp address when evaluated null or
                            // undefined
  long current_result_id_;  // current ast's result register id

  OptionalChainInfo(Function* function, bool is_optional)
      : function_(function), is_optional_(is_optional) {}
  void RefillJmpIndex(long jmp_addr) {
    for (auto& i : jmp_indexes_)
      function_->GetInstruction(i)->RefillsBx(jmp_addr - i);
  }
  void RefillMoveIndex(long reg_id) {
    for (auto& i : move_indexes_)
      function_->GetInstruction(i)->RefillsA(reg_id);
  }
};

struct FunctionGenerate;
struct BlockGenerate {
  fml::RefPtr<Function> function_;
  std::unordered_map<base::String, long> variables_map_;
  BlockGenerate* parent_;
  std::vector<BlockGenerate*> childs_;
  std::unordered_map<base::String, std::pair<int64_t, int64_t>>
      closure_variables_outside_;
  std::unordered_map<base::String, std::pair<int64_t, int64_t>>
      closure_variables_;
  std::vector<uint64_t> parent_block_ids_;
  int register_id_;
  uint64_t block_id_;
  int64_t block_number_ = -1;
  BlockGenerate()
      : function_(nullptr), variables_map_(), parent_(), register_id_(0) {}
  void SetBlockID(uint64_t id) { block_id_ = id; }
  void SetParentBlockID(BlockGenerate* parent_block,
                        FunctionGenerate* parent_function);
  std::vector<uint64_t> GetParentBlockID() { return parent_block_ids_; }
  uint64_t GetBlockID() { return block_id_; }
  void SetStartEndLine(int64_t start, int64_t end) {
    start_line_col_ = start;
    end_line_col_ = end;
  }

  int64_t SetBlockNumber(int64_t block_number) {
    block_number_ = block_number;
    return block_number_;
  }

  void SetUpvalueArray(const UpvalueArrayMap& upvalue_array) {
    upvalue_array_ = upvalue_array;
  }

  const UpvalueArrayMap& GetUpvalueArray() { return upvalue_array_; }

  void Dump(int32_t tab_size);
  Value GenerateScope(const Value& prev);
  int64_t start_line_col_ = -1;
  int64_t end_line_col_ = -1;
  UpvalueArrayMap upvalue_array_;
};

struct FunctionGenerate {
  FunctionGenerate* parent_;
  std::vector<FunctionGenerate*> childs_;
  BlockGenerate* current_block_ = nullptr;
  std::unordered_set<BlockGenerate*> blocks_;
  std::unique_ptr<LoopGenerate> current_loop_;
  std::unique_ptr<TryCatchGenerate> current_tryCatch_;
  fml::RefPtr<Function> function_;
  long register_id_ = 0;
  long function_number_ = -1;
  std::vector<base::String> function_params_;
  void GenerateScope(const Value& firstScope);
  FunctionGenerate() = default;

  size_t SetFunctionNumber(size_t function_number) {
    function_number_ = function_number;
    return function_number_;
  }
  void Dump(int tab_size);
  void GenerateScope();
};

// Help to save & restore the register position so that manage register count
// easily.
//
// When use RegisterBoundary, you can generate any register id for using and
// those register will be recycle when you leave the boundary.
struct RegisterBoundary {
  FunctionGenerate* func_gen_;
  long saved_rid_;

  RegisterBoundary(FunctionGenerate* func_gen) : func_gen_(func_gen) {
    // save
    saved_rid_ = func_gen->register_id_;
  }

  ~RegisterBoundary() {
    // restore
    func_gen_->register_id_ = saved_rid_;
  }
};

class CodeGenerator : public Visitor {
 public:
  explicit CodeGenerator(VMContext* context);
  CodeGenerator(VMContext* context, SemanticAnalysis* semanticAnalysis);
  virtual ~CodeGenerator() {}
  virtual void Visit(ChunkAST* ast, void* data);
  virtual void Visit(BlockAST* ast, void* data);
  virtual void Visit(BlockAST* ast, void* data, bool);
  virtual void Visit(CatchBlockAST* ast, void* data);
  virtual void Visit(ReturnStatementAST* ast, void* data);
  virtual void Visit(LiteralAST* ast, void* data);
  virtual void Visit(ThrowStatementAST* ast, void* data);
  virtual void Visit(NamesAST* ast, void* data);
  virtual void Visit(BinaryExprAST* ast, void* data);
  virtual void Visit(UnaryExpression* ast, void* data);
  virtual void Visit(ExpressionListAST* ast, void* data);
  virtual void Visit(VariableAST* ast, void* data);
  virtual void Visit(CatchVariableAST* ast, void* data);
  virtual void Visit(VariableListAST* ast, void* data);
  virtual void Visit(FunctionStatementAST* ast, void* data);
  virtual void Visit(ForStatementAST* ast, void* data);
  virtual void Visit(TryCatchFinallyStatementAST* ast, void* data);
  virtual void Visit(DoWhileStatementAST* ast, void* data);
  virtual void Visit(BreakStatementAST* ast, void* data);
  virtual void Visit(ContinueStatementAST* ast, void* data);
  virtual void Visit(WhileStatementAST* ast, void* data);
  virtual void Visit(IfStatementAST* ast, void* data);
  virtual void Visit(ElseStatementAST* ast, void* data);
  virtual void Visit(CaseStatementAST* ast, void* data);
  virtual void Visit(AssignStatement* ast, void* data);
  virtual void Visit(MemberAccessorAST* ast, void* data);
  virtual void Visit(FunctionCallAST* ast, void* data);
  virtual void Visit(TernaryStatementAST* ast, void* data);
  virtual void Visit(ObjectLiteralAST* ast, void* data);
  virtual void Visit(ArrayLiteralAST* ast, void* data);

 private:
  class BlockScope {
   public:
    BlockScope(CodeGenerator* code_gen) : code_gen_(code_gen) {
      if (code_gen_->support_block_closure_ && NeedGenerateBlockScope()) {
        auto function = code_gen_->current_function_->function_;
        function->AddInstruction(Instruction::Code(TypeLabel_EnterBlock));
        code_gen_->CreateAndPushContext(true);
        code_gen_->context_stack_.push_back(code_gen_->upvalue_array_);
        function->PushBSStack(code_gen_->GetCurrentBlockId());
      }
    }

    ~BlockScope() {
      if (code_gen_->support_block_closure_ && NeedGenerateBlockScope()) {
        code_gen_->context_->PopCurrentContextReg();
        code_gen_->context_stack_.pop_back();
        auto function = code_gen_->current_function_->function_;
        function->AddInstruction(Instruction::Code(TypeLabel_LeaveBlock));
        function->PopBSStack();
      }
    }

   private:
    bool NeedGenerateBlockScope() {
      return code_gen_->support_closure_ &&
             !code_gen_->upvalue_array_.empty() &&
             !code_gen_->current_function_->current_block_->GetUpvalueArray()
                  .empty();
    }
    CodeGenerator* code_gen_;
  };

  class ContextScope {
   public:
    ContextScope(CodeGenerator* code_gen) : code_gen_(code_gen) {
      if (code_gen_->support_closure_ && !code_gen_->upvalue_array_.empty()) {
        code_gen_->CreateAndPushContext();
      }
    }

    ~ContextScope() {
      if (code_gen_->support_closure_ && !code_gen_->upvalue_array_.empty()) {
        // Context will be poped in ret instruction
        code_gen_->PopContext();
        code_gen_->context_->PopCurrentContextReg();
      }
    }

   private:
    CodeGenerator* code_gen_;
  };

  class LineScope {
   public:
    LineScope(CodeGenerator* code_gen, ASTree* tree) : code_gen_(code_gen) {
      old_start_line_ = code_gen->start_line_;
      old_end_line_ = code_gen->end_line_;

      code_gen->start_line_ = tree->LineCol();
      code_gen->end_line_ = tree->EndLineCol();
    }

    ~LineScope() {
      code_gen_->start_line_ = old_start_line_;
      code_gen_->end_line_ = old_end_line_;
    }

    LineScope(const LineScope&) = delete;
    LineScope& operator=(const LineScope&) = delete;

   private:
    CodeGenerator* code_gen_;
    int64_t old_start_line_;
    int64_t old_end_line_;
  };

  void GenerateScopes(FunctionGenerate* root);
  void EnterFunction();
  void LeaveFunction();
  void EnterBlock();
  void LeaveBlock();
  void EnterLoop();
  void LeaveLoop();
  void EnterTryCatch();
  void LeaveTryCatch();
  void GenLeaveBlockScopeIns();

  void InsertVariable(const base::String& name, long register_id);
  long SearchVariable(const base::String& name);
  long SearchVariable(const base::String& name, FunctionGenerate* current);
  long SearchGlobal(const base::String& name);
  long SearchBuiltin(const base::String& name);
  long ManageUpvalues(const base::String& name);
  std::pair<long, long> ManageUpvaluesNew(const base::String& name);

  void WriteLocalValue(LexicalOp op, long dst, long src);
  void WriteUpValueNew(LexicalOp op, std::pair<long, long> dst, long src);
  void WriteUpValue(LexicalOp op, long dst, long src);

  void AutomaticLocalValue(AutomaticType type, long dst, long src);
  void AutomaticUpValueNew(AutomaticType type, long dst,
                           std::pair<long, long> src);
  void AutomaticUpValue(AutomaticType type, long dst, long src);
  long GetUpvalueArrayIndex(const base::String& name);
  long GenerateRegisterId();
  uint64_t GetCurrentBlockId();
  void DestoryRegisterId() {
    if (current_function_->register_id_ > 0) {
      current_function_->register_id_--;
    }
  }
  long GenerateFunctionNumber() {
    function_number_++;
    return function_number_;
  }

  int64_t GenerateBlockNumber() { return ++block_number_; }

  int64_t GenerateFunctionId() {
    function_id_++;
    return function_id_;
  }

  std::string GetPartStr(Token& token) {
    return semanticAnalysis_->GetPartStr(token);
  }

  void CreateAndPushContext(bool is_block = false);
  void PopContext();
  bool IsTopLevelFunction() { return !current_function_->parent_; }

  const std::unordered_map<std::pair<lynx::base::String, uint64_t>, long,
                           pair_hash>&
  GetParentUpArray() {
    if (IsTopLevelFunction()) {
      return current_function_->function_->GetUpvalueArray();
    } else {
      return current_function_->parent_->function_->GetUpvalueArray();
    }
  }
  void AddOptionalChainInfo(ASTree* ast, Function* function,
                            long result_reg_id);
  void CompleteOptionalChainInfos();

  bool support_closure_;
  bool support_block_closure_;
  VMContext* context_;
  base::String current_function_name_;
  FunctionGenerate* current_function_ = nullptr;
  SemanticAnalysis* semanticAnalysis_ = nullptr;
  long function_number_;
  int64_t block_number_;
  std::unordered_map<std::pair<base::String, uint64_t>, long, pair_hash>
      upvalue_array_;
  std::vector<
      std::unordered_map<std::pair<base::String, uint64_t>, long, pair_hash>>
      context_stack_;

  // we use function-id and pc-index to generate sourceMap, then sourceMap treat
  // function-id as line number
  // and treat pc-index as column number, but sourceMap assume tha line number
  // is start from 1, so the function_id will start from 1, other than 0
  int64_t function_id_ = 0;

  // used to hold function_generators and block_generators
  // they will be freed after CodeGenerator free
  std::vector<std::unique_ptr<FunctionGenerate>> function_generators_;
  std::vector<std::unique_ptr<BlockGenerate>> block_generators_;
  std::unordered_map<ASTree*, OptionalChainInfo> optional_chain_infos_;
  int64_t start_line_ = -1;
  int64_t end_line_ = -1;
  uint64_t block_id_increase_ = 0;
  friend ContextScope;
  friend BlockScope;
  friend LineScope;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_CODE_GENERATOR_H_
