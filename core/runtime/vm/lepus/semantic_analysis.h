// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_SEMANTIC_ANALYSIS_H_
#define CORE_RUNTIME_VM_LEPUS_SEMANTIC_ANALYSIS_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/value/base_string.h"
#include "core/runtime/vm/lepus/scanner.h"
#include "core/runtime/vm/lepus/syntax_tree.h"
#include "core/runtime/vm/lepus/visitor.h"
#include "core/runtime/vm/lepus/vm_context.h"

namespace lynx {
namespace lepus {

using UpvalueArrayMap =
    std::unordered_map<std::pair<base::String, uint64_t>, int64_t, pair_hash>;

class LexicalFunction;
struct LexicalBlock {
  std::shared_ptr<LexicalBlock> parent_;
  uint64_t block_id_;
  std::unordered_set<lynx::base::String> names_;
  std::vector<uint64_t> parent_block_ids_;
  int64_t block_number_ = -1;
  void SetBlockID(uint64_t id) { block_id_ = id; }
  void SetParentBlockID(std::shared_ptr<LexicalBlock>& parent_block,
                        std::shared_ptr<LexicalFunction>& parent_function);
  uint64_t GetBlockID() { return block_id_; }
  LexicalBlock() : parent_(nullptr) {}
  void AddUpvalue(const base::String& upvalue_name, uint64_t block_id);
  int64_t SetBlockNumber(int64_t block_number);
  // name, block_id,  array_index
  UpvalueArrayMap upvalue_array_;
  int64_t upvalue_array_max_index_ = 0;
};

class LexicalFunction {
 public:
  std::shared_ptr<LexicalFunction> parent_;
  std::shared_ptr<LexicalBlock> current_block_;
  std::string function_name_;
  // name, block_id,  array_index
  std::unordered_map<std::pair<lynx::base::String, uint64_t>, long, pair_hash>
      upvalue_array_;
  long upvalue_array_max_index_ = 0;
  size_t function_num_ = -1;

  LexicalFunction() = default;

  void SetFunctionName(const std::string& function_name) {
    function_name_ = function_name;
  }

  size_t SetFunctionNumber(size_t function_number) {
    function_num_ = function_number;
    return function_num_;
  }

  void AddUpvalue(const base::String& upvalue_name, uint64_t block_id);
};

class SemanticAnalysis : public Visitor {
 public:
  explicit SemanticAnalysis();
  virtual ~SemanticAnalysis() {}
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

  const std::shared_ptr<LexicalBlock> GetBlockFromBlockNumber(
      int64_t block_number) {
    return block_map_[block_number];
  }

  const std::shared_ptr<LexicalFunction> GetFunctionFromFunctionNumber(
      size_t function_number) {
    return function_map_[function_number];
  }
  void SetSdkVersion(const std::string& sdk_version);
  void SetClosureFix(bool val) { closure_fix_ = val; }
  void SetInput(Scanner* input_) { input = input_; }
  std::string GetPartStr(Token& token) { return input->GetPartStr(token); }

 private:
  void EnterFunction();
  void LeaveFunction();
  void EnterBlock();
  void LeaveBlock();

  bool InsertName(const base::String& name);
  size_t GenerateFunctionNumber() {
    function_number_++;
    return function_number_;
  }
  int64_t GenerateBlockNumber() { return ++block_number_; }
  LexicalScoping SearchName(const base::String& name);
  std::shared_ptr<LexicalFunction> current_function_;
  size_t function_number_ = -1;
  int64_t block_number_ = -1;
  std::unordered_map<size_t, std::shared_ptr<LexicalFunction>> function_map_;
  std::unordered_map<int64_t, std::shared_ptr<LexicalBlock>> block_map_;

  std::string sdk_version_;
  Scanner* input;
  bool closure_fix_ = false;
  uint64_t block_id_increase_ = 0;
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_SEMANTIC_ANALYSIS_H_
