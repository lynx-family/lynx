// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/code_generator.h"

#include <stack>

#include "base/include/sorted_for_each.h"
#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/function.h"
#include "core/runtime/vm/lepus/guard.h"
#include "core/runtime/vm/lepus/op_code.h"
#include "core/runtime/vm/lepus/switch.h"
#define INT_LOGIC_VERSION "1.3"

namespace lynx {
namespace lepus {

class AstLineScope {
 public:
  AstLineScope(Function* func, ASTree* tree) : func_(func) {
    old_line_col_ = func->CurrentLineCol();
    if (old_line_col_ != -1 && tree->LineCol() == -1) {
      return;
    } else {
      func->SetCurrentLineCol(tree->LineCol());
    }
  }

  ~AstLineScope() { func_->SetCurrentLineCol(old_line_col_); }

 private:
  Function* func_;
  int64_t old_line_col_;
};

#define NOOP function->AddInstruction(Instruction::ACode(TypeOp_Noop, 0));

#define Load(type, reg, index)                                          \
  do {                                                                  \
    if (reg >= 0)                                                       \
      function->AddInstruction(Instruction::ABxCode(type, reg, index)); \
  } while (0);

#define Ret(reg)                                                     \
  do {                                                               \
    if (reg >= 0)                                                    \
      function->AddInstruction(Instruction::ACode(TypeOp_Ret, reg)); \
  } while (0);

#define MOVE(dst, src)                                                      \
  do {                                                                      \
    if (src >= 0 && dst >= 0)                                               \
      function->AddInstruction(Instruction::ABCode(TypeOp_Move, dst, src)); \
  } while (0);

#define Call(caller, argc, result)                                  \
  do {                                                              \
    if (caller >= 0 && result >= 0 && argc >= 0)                    \
      function->AddInstruction(                                     \
          Instruction::ABCCode(TypeOp_Call, caller, argc, result)); \
  } while (0);

#define Switch(jmp_index, reg, index)                       \
  long jmp_index = -1;                                      \
  do {                                                      \
    if (reg >= 0 && index >= 0)                             \
      jmp_index = function->AddInstruction(                 \
          Instruction::ABxCode(TypeOp_Switch, reg, index)); \
  } while (0);

#define BinaryOperator(type, dst, src1, src2)                                \
  do {                                                                       \
    if (src1 >= 0 && src2 >= 0 && dst >= 0)                                  \
      function->AddInstruction(Instruction::ABCCode(type, dst, src1, src2)); \
  } while (0);

#define UpvalueOperator(type, reg, index)                              \
  do {                                                                 \
    if (reg >= 0 && index >= 0)                                        \
      function->AddInstruction(Instruction::ABCode(type, reg, index)); \
  } while (0);

#define AutomaticOperator(type, reg)                                       \
  do {                                                                     \
    if (reg >= 0) function->AddInstruction(Instruction::ACode(type, reg)); \
  } while (0);

CodeGenerator::CodeGenerator(VMContext* context) : context_(context) {
  support_closure_ = tasm::Config::IsHigherOrEqual(context_->sdk_version_,
                                                   FEATURE_CONTROL_VERSION_2);
  support_block_closure_ =
      tasm::Config::IsHigherOrEqual(context_->sdk_version_, LYNX_VERSION_2_6);
}

CodeGenerator::CodeGenerator(VMContext* context,
                             SemanticAnalysis* semanticAnalysis)
    : context_(context),
      semanticAnalysis_(semanticAnalysis),
      function_number_(-1),
      block_number_(-1) {
  support_closure_ = lynx::tasm::Config::IsHigherOrEqual(
      context_->sdk_version_, FEATURE_CONTROL_VERSION_2);
  support_block_closure_ = lynx::tasm::Config::IsHigherOrEqual(
      context_->sdk_version_, LYNX_VERSION_2_6);
}

void CodeGenerator::EnterFunction() {
  FunctionGenerate* function = new FunctionGenerate;
  function_generators_.push_back(std::unique_ptr<FunctionGenerate>(function));
  if (current_function_) {
    current_function_->childs_.push_back(function);
  }
  function->parent_ = current_function_;
  current_function_ = function;
  function->function_ = Function::Create();
  FunctionGenerate* parent = function->parent_;
  if (IsTopLevelFunction()) {
    function->function_->SetFunctionName("<anonymous>");
  } else {
    function->function_->set_index(
        parent->function_->AddChildFunction(function->function_));
    function->function_->SetFunctionName(current_function_name_.str());
  }

  // generate a unique function id
  function->function_->SetFunctionId(GenerateFunctionId());

  if (support_closure_) {
    size_t function_number =
        function->SetFunctionNumber(GenerateFunctionNumber());
    std::shared_ptr<LexicalFunction> lexical_function =
        semanticAnalysis_->GetFunctionFromFunctionNumber(function_number);
    if (lexical_function) {
      function->function_->SetUpvalueArray(lexical_function->upvalue_array_);
    }
    if (!GetParentUpArray().empty()) {
      context_stack_.push_back(GetParentUpArray());
    }
    upvalue_array_ = current_function_->function_->GetUpvalueArray();
    if (IsTopLevelFunction() && !upvalue_array_.empty()) {
      CreateAndPushContext();
    }
  }
}

void CodeGenerator::LeaveFunction() {
  if (support_closure_) {
    if (!GetParentUpArray().empty()) {
      context_stack_.pop_back();
    }
    if (IsTopLevelFunction() && !upvalue_array_.empty()) {
      PopContext();
      context_->PopCurrentContextReg();
    }
    upvalue_array_ = GetParentUpArray();
  }

  current_function_ = std::move(current_function_->parent_);
  current_function_name_ = "";
}

void CodeGenerator::EnterBlock() {
  BlockGenerate* block = new BlockGenerate;
  block->SetBlockID(block_id_increase_++);
  block->SetStartEndLine(start_line_, end_line_);
  block_generators_.push_back(std::unique_ptr<BlockGenerate>(block));
  if (current_function_->current_block_) {
    current_function_->current_block_->childs_.push_back(block);
  }
  block->parent_ = current_function_->current_block_;
  current_function_->current_block_ = block;
  current_function_->current_block_->SetParentBlockID(
      block->parent_, current_function_->parent_);

  int64_t block_number = block->SetBlockNumber(GenerateBlockNumber());
  auto lexical_block = semanticAnalysis_->GetBlockFromBlockNumber(block_number);
  if (lexical_block) {
    block->SetUpvalueArray(lexical_block->upvalue_array_);
  }

  while (block->parent_) {
    block = block->parent_;
  }
  current_function_->blocks_.insert(block);
}

void CodeGenerator::LeaveBlock() {
  BlockGenerate* block = current_function_->current_block_;
  current_function_->current_block_ = block->parent_;
}

void CodeGenerator::EnterLoop() {
  LoopGenerate* loop = new LoopGenerate;
  loop->loop_start_index_ = current_function_->function_->OpCodeSize();
  loop->parent_ = std::move(current_function_->current_loop_);
  current_function_->current_loop_.reset(loop);
}

void CodeGenerator::LeaveLoop() {
  size_t end_loop_index = current_function_->function_->OpCodeSize();
  LoopGenerate* current_loop = current_function_->current_loop_.get();
  for (std::size_t i = 0; i < current_loop->loop_infos_.size(); ++i) {
    LoopInfo* info = &current_loop->loop_infos_[i];
    long op_index = info->op_index_;
    if (info->type_ == LoopJmp_Tail) {
      current_function_->function_->GetInstruction(op_index)->RefillsBx(
          end_loop_index - op_index);
    } else if (info->type_ == LoopJmp_Head) {
      current_function_->function_->GetInstruction(op_index)->RefillsBx(
          current_loop->loop_start_index_ - op_index);
    } else if (info->type_ == LoopJmp_Continue) {
      current_function_->function_->GetInstruction(op_index)->RefillsBx(
          current_loop->loop_continue_index_ - op_index);
    }
  }

  current_function_->current_loop_ = std::move(current_loop->parent_);
}

void CodeGenerator::EnterTryCatch() {
  TryCatchGenerate* trycatch = new TryCatchGenerate;
  trycatch->parent_ = std::move(current_function_->current_tryCatch_);
  current_function_->current_tryCatch_.reset(trycatch);
}

void CodeGenerator::LeaveTryCatch() {
  TryCatchGenerate* current_tryCatch =
      current_function_->current_tryCatch_.get();
  for (std::size_t i = 0; i < current_tryCatch->trycatch_infos_.size(); ++i) {
    TryCatchInfo* info = &current_tryCatch->trycatch_infos_[i];
    long op_index = info->op_index_;
    if (info->type_ == TryCatchJmp_finally) {
      current_function_->function_->GetInstruction(op_index)->RefillsBx(
          current_tryCatch->finally_index_ - op_index);
    }
  }

  current_function_->current_tryCatch_ = std::move(current_tryCatch->parent_);
}

void CodeGenerator::InsertVariable(const base::String& name, long register_id) {
  current_function_->current_block_->variables_map_[name] = register_id;
  if ((!context_->closure_fix_ ||
       !current_function_->current_block_->parent_) &&
      !current_function_->parent_) {
    context_->top_level_variables_.insert(std::make_pair(name, register_id));
  }
}

long CodeGenerator::SearchVariable(const base::String& name) {
  return SearchVariable(name, current_function_);
}

long CodeGenerator::SearchVariable(const base::String& name,
                                   FunctionGenerate* current) {
  BlockGenerate* block = current->current_block_;
  while (block) {
    auto iter = block->variables_map_.find(name);
    if (iter != block->variables_map_.end()) {
      return iter->second;
    }
    block = block->parent_;
  }
  return -1;
}

long CodeGenerator::SearchGlobal(const base::String& name) {
  return context_->global()->Search(name);
}

long CodeGenerator::SearchBuiltin(const base::String& name) {
  return context_->builtin()->Search(name);
}

long CodeGenerator::ManageUpvalues(const base::String& name) {
  fml::RefPtr<Function> function = current_function_->function_;
  long index = function->SearchUpvalue(name);

  if (index > 0) return index;

  std::stack<FunctionGenerate*> parents;
  parents.push(current_function_->parent_);

  long register_index = -1;
  bool in_parent_vars = false;

  while (!parents.empty()) {
    FunctionGenerate* current = parents.top();
    if (register_index >= 0) {
      long index =
          current->function_->AddUpvalue(name, register_index, in_parent_vars);
      in_parent_vars = false;
      register_index = index;
      parents.pop();
    } else {
      long register_id = SearchVariable(name, current);
      if (register_id >= 0) {
        register_index = register_id;
        in_parent_vars = true;
        parents.pop();
      } else {
        long index = current->function_->SearchUpvalue(name);
        if (index >= 0) {
          register_index = index;
          in_parent_vars = false;
          parents.pop();
        } else {
          parents.push(current->parent_);
        }
      }
    }
  }

  index = function->AddUpvalue(name, register_index, in_parent_vars);
  return index;
}

std::pair<long, long> CodeGenerator::ManageUpvaluesNew(
    const base::String& name) {
  fml::RefPtr<Function> function = current_function_->function_;
  long offset = 0;
  long array_index = -1;
  for (int i = static_cast<int>(context_stack_.size() - 1); i >= 0; i--) {
    auto upvalue_array = context_stack_[i];
    // find the {name, nearest parent block id} in upvalue_array
    bool find_result = false;
    uint64_t id = 0;
    std::vector<uint64_t> parent_ids =
        current_function_->current_block_->GetParentBlockID();
    for (int32_t j = static_cast<int32_t>(parent_ids.size()) - 1; j >= 0; j--) {
      id = parent_ids[j];
      auto iter = upvalue_array.find({name, id});
      if (iter != upvalue_array.end()) {
        find_result = true;
        array_index = iter->second;
        break;
      }
    }

    if (find_result == true) {
      break;
    } else {
      offset++;
    }
  }
  current_function_->current_block_->closure_variables_.insert(
      {name, {array_index, offset}});
  // adaptor to old template
  return {array_index, offset};
}

void CodeGenerator::GenerateScopes(FunctionGenerate* root) {
  Value scope;
  root->GenerateScope(scope);
}

void CodeGenerator::Visit(ChunkAST* ast, void* data) {
  LineScope lsp(this, ast->block().get());
  Guard<CodeGenerator> g(this, &CodeGenerator::EnterFunction,
                         &CodeGenerator::LeaveFunction);
  AstLineScope sop(current_function_->function_.get(), ast);
  ast->block()->Accept(this, nullptr);

  CompleteOptionalChainInfos();

  context_->root_function_ = current_function_->function_;
  GenerateScopes(current_function_);
}

void CodeGenerator::Visit(CatchBlockAST* ast, void* data) {
  Guard<CodeGenerator> g(this, &CodeGenerator::EnterBlock,
                         &CodeGenerator::LeaveBlock);
  RegisterBoundary rebs(current_function_);
  if (ast->catch_identifier()) {
    ast->catch_identifier()->Accept(this, nullptr);
  }
  ast->block()->Accept(this, nullptr);
}

void CodeGenerator::GenLeaveBlockScopeIns() {
  auto function = current_function_->function_;
  auto block = current_function_->current_block_;
  auto bs_stack = function->block_scope_stack();

  while (block) {
    if (!bs_stack.empty() && bs_stack.top() == block->block_id_) {
      function->AddInstruction(Instruction::Code(TypeLabel_LeaveBlock));
      break;
    }
    if (block->block_id_ == function->GetLoopBlockStack()) {
      break;
    }
    block = block->parent_;
  }
}

void CodeGenerator::Visit(BlockAST* ast, void* data, bool gen_scope) {
  LineScope lsp(this, ast);
  Guard<CodeGenerator> g(this, &CodeGenerator::EnterBlock,
                         &CodeGenerator::LeaveBlock);
  BlockScope block_scope(this);
  RegisterBoundary rebs(current_function_);
  AstLineScope sop(current_function_->function_.get(), ast);

  for (ASTreeVector::iterator iter = ast->statements().begin();
       iter != ast->statements().end(); ++iter) {
    (*iter)->Accept(this, nullptr);
  }

  if (ast->return_statement()) {
    ast->return_statement()->Accept(this, nullptr);
  }
}

void CodeGenerator::Visit(BlockAST* ast, void* data) {
  LineScope lsp(this, ast);
  Guard<CodeGenerator> g(this, &CodeGenerator::EnterBlock,
                         &CodeGenerator::LeaveBlock);

  RegisterBoundary rebs(current_function_);
  AstLineScope sop(current_function_->function_.get(), ast);

  for (ASTreeVector::iterator iter = ast->statements().begin();
       iter != ast->statements().end(); ++iter) {
    (*iter)->Accept(this, nullptr);
  }
  if (ast->return_statement()) {
    ast->return_statement()->Accept(this, nullptr);
  }
}

void CodeGenerator::Visit(ReturnStatementAST* ast, void* data) {
  RegisterBoundary regs(current_function_);
  AstLineScope sop(current_function_->function_.get(), ast);
  fml::RefPtr<Function> function = current_function_->function_;
  long register_id = GenerateRegisterId();
  if (ast->expression())
    ast->expression()->Accept(this, &register_id);
  else
    Load(TypeOp_LoadNil, register_id, 0);

  // Leave block scope first before OP_Ret
  auto bs_stack = function->block_scope_stack();
  size_t bs_stack_size = bs_stack.size();
  while (bs_stack_size--) {
    function->AddInstruction(Instruction::Code(TypeLabel_LeaveBlock));
  }

  if (support_closure_) {
    if (upvalue_array_.size()) {
      PopContext();
    }
  }

  Ret(register_id);
}

void CodeGenerator::WriteLocalValue(LexicalOp op, long dst, long src) {
  fml::RefPtr<Function> function = current_function_->function_;
  Instruction instruction;
  switch (op) {
    case LexicalOp_Write:
      MOVE(dst, src);
      break;
    case LexicalOp_ASSIGN_ADD:
      BinaryOperator(TypeOp_Add, dst, dst, src);
      break;
    case LexicalOp_ASSIGN_SUB:
      BinaryOperator(TypeOp_Sub, dst, dst, src);
      break;
    case LexicalOp_ASSIGN_MUL:
      BinaryOperator(TypeOp_Mul, dst, dst, src);
      break;
    case LexicalOp_ASSIGN_DIV:
      BinaryOperator(TypeOp_Div, dst, dst, src);
      break;
    case LexicalOp_ASSIGN_MOD:
      BinaryOperator(TypeOp_Mod, dst, dst, src);
      break;
    case LexicalOp_ASSIGN_BIT_OR:
      BinaryOperator(TypeOp_BitOr, dst, dst, src);
      break;
    case LexicalOp_ASSIGN_BIT_AND:
      BinaryOperator(TypeOp_BitAnd, dst, dst, src);
      break;
    case LexicalOp_ASSIGN_BIT_XOR:
      BinaryOperator(TypeOp_BitXor, dst, dst, src);
      break;
    case LexicalOp_ASSIGN_POW:
      BinaryOperator(TypeOp_Pow, dst, dst, src);
      break;
    default:
      break;
  }
}

void CodeGenerator::WriteUpValue(LexicalOp op, long dst, long src) {
  RegisterBoundary regs(current_function_);
  fml::RefPtr<Function> function = current_function_->function_;
  Instruction instruction;
  long register_id = GenerateRegisterId();
  switch (op) {
    case LexicalOp_ASSIGN_ADD:
      UpvalueOperator(TypeOp_GetUpvalue, register_id, dst);
      BinaryOperator(TypeOp_Add, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_SUB:
      UpvalueOperator(TypeOp_GetUpvalue, register_id, dst);
      BinaryOperator(TypeOp_Sub, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_MUL:
      UpvalueOperator(TypeOp_GetUpvalue, register_id, dst);
      BinaryOperator(TypeOp_Mul, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_DIV:
      UpvalueOperator(TypeOp_GetUpvalue, register_id, dst);
      BinaryOperator(TypeOp_Div, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_MOD:
      UpvalueOperator(TypeOp_GetUpvalue, register_id, dst);
      BinaryOperator(TypeOp_Mod, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_BIT_OR:
      UpvalueOperator(TypeOp_GetUpvalue, register_id, dst);
      BinaryOperator(TypeOp_BitOr, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_BIT_AND:
      UpvalueOperator(TypeOp_GetUpvalue, register_id, dst);
      BinaryOperator(TypeOp_BitAnd, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_BIT_XOR:
      UpvalueOperator(TypeOp_GetUpvalue, register_id, dst);
      BinaryOperator(TypeOp_BitXor, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_POW:
      UpvalueOperator(TypeOp_GetUpvalue, register_id, dst);
      BinaryOperator(TypeOp_Pow, src, register_id, src);
      break;
    default:
      break;
  }
  UpvalueOperator(TypeOp_SetUpvalue, src, dst);
}

void CodeGenerator::WriteUpValueNew(LexicalOp op, std::pair<long, long> data,
                                    long src) {
  RegisterBoundary regs(current_function_);
  fml::RefPtr<Function> function = current_function_->function_;
  Instruction instruction;
  long register_id = GenerateRegisterId();
  switch (op) {
    case LexicalOp_ASSIGN_ADD:
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_GetContextSlot, register_id, data.first, data.second));
      BinaryOperator(TypeOp_Add, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_SUB:
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_GetContextSlot, register_id, data.first, data.second));
      BinaryOperator(TypeOp_Sub, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_MUL:
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_GetContextSlot, register_id, data.first, data.second));
      BinaryOperator(TypeOp_Mul, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_DIV:
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_GetContextSlot, register_id, data.first, data.second));
      BinaryOperator(TypeOp_Div, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_MOD:
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_GetContextSlot, register_id, data.first, data.second));
      BinaryOperator(TypeOp_Mod, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_BIT_OR:
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_GetContextSlot, register_id, data.first, data.second));
      BinaryOperator(TypeOp_BitOr, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_BIT_AND:
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_GetContextSlot, register_id, data.first, data.second));
      BinaryOperator(TypeOp_BitAnd, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_BIT_XOR:
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_GetContextSlot, register_id, data.first, data.second));
      BinaryOperator(TypeOp_BitXor, src, register_id, src);
      break;
    case LexicalOp_ASSIGN_POW:
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_GetContextSlot, register_id, data.first, data.second));
      BinaryOperator(TypeOp_Pow, src, register_id, src);
      break;
    default:
      break;
  }
  function->AddInstruction(Instruction::ABCCode(TypeOp_SetContextSlot, src,
                                                data.first, data.second));
}

void CodeGenerator::AutomaticLocalValue(AutomaticType type, long dst,
                                        long src) {
  fml::RefPtr<Function> function = current_function_->function_;
  switch (type) {
    case Automatic_None:
      MOVE(dst, src);
      break;
    case Automatic_Inc_Before:
      function->AddInstruction(Instruction::ACode(TypeOp_Inc, src));
      MOVE(dst, src);
      break;
    case Automatic_Inc_After:
      MOVE(dst, src);
      function->AddInstruction(Instruction::ACode(TypeOp_Inc, src));
      break;
    case Automatic_Dec_Before:
      function->AddInstruction(Instruction::ACode(TypeOp_Dec, src));
      MOVE(dst, src);
      break;
    case Automatic_Dec_After:
      MOVE(dst, src);
      function->AddInstruction(Instruction::ACode(TypeOp_Dec, src));
      break;
    default:
      break;
  }
}

void CodeGenerator::AutomaticUpValue(AutomaticType type, long dst, long src) {
  RegisterBoundary regs(current_function_);
  fml::RefPtr<Function> function = current_function_->function_;
  long register_id = GenerateRegisterId();
  switch (type) {
    case Automatic_None:
      UpvalueOperator(TypeOp_GetUpvalue, dst, src);
      break;
    case Automatic_Inc_Before:
      UpvalueOperator(TypeOp_GetUpvalue, register_id, src);
      AutomaticOperator(TypeOp_Inc, register_id);
      MOVE(dst, register_id);
      UpvalueOperator(TypeOp_SetUpvalue, register_id, src);
      break;
    case Automatic_Inc_After:
      UpvalueOperator(TypeOp_GetUpvalue, register_id, src);
      MOVE(dst, register_id);
      AutomaticOperator(TypeOp_Inc, register_id);
      UpvalueOperator(TypeOp_SetUpvalue, register_id, src);
      break;
    case Automatic_Dec_Before:
      UpvalueOperator(TypeOp_GetUpvalue, register_id, src);
      AutomaticOperator(TypeOp_Dec, register_id);
      MOVE(dst, register_id);
      UpvalueOperator(TypeOp_SetUpvalue, register_id, src);
      break;
    case Automatic_Dec_After:
      UpvalueOperator(TypeOp_GetUpvalue, register_id, src);
      MOVE(dst, register_id);
      AutomaticOperator(TypeOp_Dec, register_id);
      UpvalueOperator(TypeOp_SetUpvalue, register_id, src);
      break;
    default:
      break;
  }
}

void CodeGenerator::AutomaticUpValueNew(AutomaticType type, long dst,
                                        std::pair<long, long> data) {
  RegisterBoundary regs(current_function_);
  fml::RefPtr<Function> function = current_function_->function_;
  long register_id = GenerateRegisterId();
  switch (type) {
    case Automatic_None:
      function->AddInstruction(Instruction::ABCCode(TypeOp_GetContextSlot, dst,
                                                    data.first, data.second));
      break;
    case Automatic_Inc_Before:
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_GetContextSlot, register_id, data.first, data.second));
      AutomaticOperator(TypeOp_Inc, register_id);
      MOVE(dst, register_id);
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_SetContextSlot, register_id, data.first, data.second));
      break;
    case Automatic_Inc_After:
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_GetContextSlot, register_id, data.first, data.second));
      MOVE(dst, register_id);
      AutomaticOperator(TypeOp_Inc, register_id);
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_SetContextSlot, register_id, data.first, data.second));
      break;
    case Automatic_Dec_Before:
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_GetContextSlot, register_id, data.first, data.second));
      AutomaticOperator(TypeOp_Dec, register_id);
      MOVE(dst, register_id);
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_SetContextSlot, register_id, data.first, data.second));
      break;
    case Automatic_Dec_After:
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_GetContextSlot, register_id, data.first, data.second));
      MOVE(dst, register_id);
      AutomaticOperator(TypeOp_Dec, register_id);
      function->AddInstruction(Instruction::ABCCode(
          TypeOp_SetContextSlot, register_id, data.first, data.second));
      break;
    default:
      break;
  }
}

void CodeGenerator::Visit(ThrowStatementAST* ast, void* data) {
  fml::RefPtr<Function> function = current_function_->function_;
  AstLineScope sop(current_function_->function_.get(), ast);
  if (ast->throw_identifier()) {
    long source_id = GenerateRegisterId();
    ast->throw_identifier()->Accept(this, &source_id);

    Instruction instruction = Instruction::ACode(TypeLabel_Throw, source_id);
    function->AddInstruction(instruction);
  }
}

uint64_t CodeGenerator::GetCurrentBlockId() {
  return current_function_->current_block_->GetBlockID();
}

long CodeGenerator::GetUpvalueArrayIndex(const lynx::base::String& name) {
  uint64_t value_block_id = 0;
  // find the block where this value belongs
  BlockGenerate* block = current_function_->current_block_;
  while (block) {
    auto iter = block->variables_map_.find(name);
    if (iter != block->variables_map_.end()) {
      value_block_id = block->GetBlockID();
      break;
    }
    block = block->parent_;
  }

  // iterate current block and parent blocks, find if {name, block_id} is in
  // upvalue_array_
  block = current_function_->current_block_;
  while (block) {
    uint64_t block_id = block->GetBlockID();
    auto iter = upvalue_array_.find({name, block_id});
    if (iter != upvalue_array_.end()) {
      if (value_block_id == block_id) {
        return iter->second;
      } else {
        return -1;
      }
    }
    block = block->parent_;
  }
  return -1;
}

void CodeGenerator::Visit(LiteralAST* ast, void* data) {
  fml::RefPtr<Function> function = current_function_->function_;
  AstLineScope sop(current_function_->function_.get(), ast);
  long register_id = data == nullptr ? -1 : *static_cast<long*>(data);
  if (ast->lex_op() >= LexicalOp_Write) {
    if (ast->scope() == LexicalScoping_Local) {
      base::String name = ast->token().str_;
      long var_reg_id = SearchVariable(name);
      if (var_reg_id < 0) {
        throw CompileException(name.c_str(), " is not defined", ast->token(),
                               GetPartStr(ast->token()));
      }
      WriteLocalValue(ast->lex_op(), var_reg_id, register_id);
      if (support_closure_) {
        long array_index = GetUpvalueArrayIndex(name);
        if (array_index != -1) {
          auto instruction = Instruction::ABCCode(
              TypeOp_SetContextSlotMove, context_->GetCurrentContextReg(),
              array_index, var_reg_id);
          current_function_->function_->AddInstruction(instruction);
          current_function_->current_block_->closure_variables_outside_.insert(
              {name, {array_index, context_->GetCurrentContextReg()}});
        }
      }
    } else if (ast->scope() == LexicalScoping_Global) {
    } else {
      if (support_closure_) {
        if (ast->scope() == LexicalScoping_Upvalue_New) {
          std::pair<long, long> index_data =
              ManageUpvaluesNew(ast->token().str_);
          WriteUpValueNew(ast->lex_op(), index_data, register_id);
        } else if (ast->scope() == LexicalScoping_Upvalue) {
          long index = ManageUpvalues(ast->token().str_);
          WriteUpValue(ast->lex_op(), index, register_id);
        }
      } else {
        if (ast->scope() == LexicalScoping_Upvalue) {
          long index = ManageUpvalues(ast->token().str_);
          WriteUpValue(ast->lex_op(), index, register_id);
        }
      }
    }
  } else if (ast->lex_op() == LexicalOp_Read) {
    if (ast->token().token_ == Token_Number) {
      long index = function->AddConstNumber(ast->token().number_);
      auto instruction =
          Instruction::ABxCode(TypeOp_LoadConst, register_id, index);
      function->AddInstruction(instruction);
    } else if (ast->token().token_ == Token_String) {
      long index = function->AddConstString(ast->token().str_);
      auto instruction =
          Instruction::ABxCode(TypeOp_LoadConst, register_id, index);
      function->AddInstruction(instruction);
    } else if (ast->token().token_ == Token_RegExp) {
      long index = function->AddConstRegExp(
          RegExp::Create(ast->token().pattern_, ast->token().flags_));
      auto instruction =
          Instruction::ABxCode(TypeOp_LoadConst, register_id, index);
      function->AddInstruction(instruction);
    } else if (ast->token().token_ == Token_True ||
               ast->token().token_ == Token_False) {
      long index = function->AddConstBoolean(
          ast->token().token_ == Token_True ? true : false);
      auto instruction =
          Instruction::ABxCode(TypeOp_LoadConst, register_id, index);
      function->AddInstruction(instruction);
    } else if (ast->token().token_ == Token_Id) {
      if (ast->scope() == LexicalScoping_Local) {
        base::String name = ast->token().str_.c_str();
        long var_reg_id = SearchVariable(ast->token().str_);
        if (var_reg_id < 0) {
          throw CompileException(name.c_str(), " is  not defined 1",
                                 ast->token(), GetPartStr(ast->token()));
        }
        if (!support_closure_) {
          AutomaticLocalValue(ast->auto_type(), register_id, var_reg_id);
        } else {
          long array_index = GetUpvalueArrayIndex(name);
          if (array_index != -1) {
            auto instruction = Instruction::ABCCode(
                TypeOp_GetContextSlotMove, var_reg_id, array_index,
                context_->GetCurrentContextReg());
            current_function_->function_->AddInstruction(instruction);
            AutomaticLocalValue(ast->auto_type(), register_id, var_reg_id);
            auto instruction1 = Instruction::ABCCode(
                TypeOp_SetContextSlotMove, context_->GetCurrentContextReg(),
                array_index, var_reg_id);
            current_function_->function_->AddInstruction(instruction1);
            current_function_->current_block_->closure_variables_outside_
                .insert({ast->token().str_,
                         {array_index, context_->GetCurrentContextReg()}});
          } else {
            AutomaticLocalValue(ast->auto_type(), register_id, var_reg_id);
          }
        }
      } else if (ast->scope() == LexicalScoping_Global) {
        long global_index = SearchGlobal(ast->token().str_);
        if (global_index >= 0) {
          auto instruction =
              Instruction::ABxCode(TypeOp_GetGlobal, register_id, global_index);
          function->AddInstruction(instruction);
        } else if (ast->token().str_ == "globalThis") {
          auto instruction =
              Instruction::ABCode(TypeOp_LoadNil, register_id, 2);
          function->AddInstruction(instruction);
        } else if (lynx::tasm::Config::IsHigherOrEqual(
                       context_->sdk_version_, FEATURE_CONTEXT_GLOBAL_DATA) &&
                   ast->token().str_ == "lynx") {
          // If engine version >= 2.8 and ast == "lynx", generate
          // Instruction::ABCode(TypeOp_LoadNil, register_id, 3).
          auto instruction =
              Instruction::ABCode(TypeOp_LoadNil, register_id, 3);
          function->AddInstruction(instruction);
        } else if (lynx::tasm::Config::IsHigherOrEqual(
                       context_->sdk_version_, FEATURE_CONTROL_VERSION)) {
          long builtin_index = SearchBuiltin(ast->token().str_);
          if (builtin_index >= 0) {
            auto instruction = Instruction::ABxCode(TypeOp_GetBuiltin,
                                                    register_id, builtin_index);
            function->AddInstruction(instruction);
          } else {
            throw CompileException(ast->token().str_.c_str(),
                                   " is not defined 2", ast->token(),
                                   GetPartStr(ast->token()));
          }
        } else {
          throw CompileException(ast->token().str_.c_str(), " is not defined 2",
                                 ast->token(), GetPartStr(ast->token()));
        }
      } else {
        if (support_closure_) {
          if (ast->scope() == LexicalScoping_Upvalue_New) {
            std::pair<long, long> index_data =
                ManageUpvaluesNew(ast->token().str_);
            AutomaticUpValueNew(ast->auto_type(), register_id, index_data);
          } else if (ast->scope() == LexicalScoping_Upvalue) {
            long index = ManageUpvalues(ast->token().str_);
            AutomaticUpValue(ast->auto_type(), register_id, index);
          }
        } else {
          if (ast->scope() == LexicalScoping_Upvalue) {
            long index = ManageUpvalues(ast->token().str_);
            AutomaticUpValue(ast->auto_type(), register_id, index);
          } else if (ast->scope() == LexicalScoping_Upvalue_New) {
            throw CompileException(
                "The current Sdk Version is less than 1.4, closure variable "
                "not support: ",
                ast->token().str_.c_str(), ast->token(),
                GetPartStr(ast->token()));
          }
        }
      }
    } else if (ast->token().token_ == Token_Nil) {
      // Instead of adding more instructions, it's recommended to clear the
      // memory.
      auto instruction = Instruction::ACode(TypeOp_LoadNil, register_id);
      function->AddInstruction(instruction);
    } else if (ast->token().token_ == Token_Undefined) {
      // use c register to show it need load undefined
      auto instruction = Instruction::ABCode(TypeOp_LoadNil, register_id, 1);
      function->AddInstruction(instruction);
    }
  }
}

void CodeGenerator::Visit(NamesAST* ast, void* data) {
  std::vector<Token>::iterator iter_name = ast->names().begin();
  AstLineScope sop(current_function_->function_.get(), ast);
  for (; iter_name != ast->names().end(); ++iter_name) {
    InsertVariable(iter_name->str_, GenerateRegisterId());
  }
}

void CodeGenerator::Visit(BinaryExprAST* ast, void* data) {
  RegisterBoundary regs(current_function_);
  AstLineScope sop(current_function_->function_.get(), ast);
  int register_id = data == nullptr ? -1 : *static_cast<int*>(data);
  fml::RefPtr<Function> function = current_function_->function_;
  long left_register_id = GenerateRegisterId();
  ast->left()->Accept(this, &left_register_id);
  if (ast->op_token().token_ == Token_And ||
      ast->op_token().token_ == Token_Or ||
      ast->op_token().token_ == Token_Nullish_Coalescing) {
    if (support_closure_) {
      Instruction instruction;
      long jmp_index = 0;
      long undef_jmp_index = 0;
      if (ast->op_token().token_ == Token_And) {
        instruction = Instruction::ABxCode(TypeOp_JmpTrue, left_register_id, 0);
        jmp_index = function->AddInstruction(instruction);
      } else if (ast->op_token().token_ == Token_Or) {
        instruction =
            Instruction::ABxCode(TypeOp_JmpFalse, left_register_id, 0);
        jmp_index = function->AddInstruction(instruction);
      } else {
        long null_id = GenerateRegisterId();
        function->AddInstruction(
            Instruction::ABCode(TypeOp_LoadNil, null_id, 0));
        BinaryOperator(TypeOp_AbsEqual, null_id, left_register_id, null_id);
        instruction = Instruction::ABxCode(TypeOp_JmpTrue, null_id, 0);
        jmp_index = function->AddInstruction(instruction);
        function->AddInstruction(
            Instruction::ABCode(TypeOp_LoadNil, null_id, 1));
        BinaryOperator(TypeOp_AbsEqual, null_id, left_register_id, null_id);
        undef_jmp_index = function->AddInstruction(
            Instruction::ABxCode(TypeOp_JmpTrue, null_id, 0));
      }

      MOVE(register_id, left_register_id);

      instruction = Instruction::ABxCode(TypeOp_Jmp, 0, 0);
      long true_jmp_index = function->AddInstruction(instruction);

      long index = function->OpCodeSize();
      function->GetInstruction(jmp_index)->RefillsBx(index - jmp_index);
      if (ast->op_token().token_ == Token_Nullish_Coalescing) {
        function->GetInstruction(jmp_index)->RefillsBx(index - jmp_index);
        function->GetInstruction(undef_jmp_index)
            ->RefillsBx(index - undef_jmp_index);
      }

      long right_register_id = GenerateRegisterId();
      ast->right()->Accept(this, &right_register_id);
      MOVE(register_id, right_register_id);
      long end_index = function->OpCodeSize();
      function->GetInstruction(true_jmp_index)
          ->RefillsBx(end_index - true_jmp_index);
    } else {
      long right_register_id = GenerateRegisterId();
      ast->right()->Accept(this, &right_register_id);
      if (ast->op_token().token_ == Token_And) {
        BinaryOperator(TypeOp_And, register_id, left_register_id,
                       right_register_id);
      } else {
        BinaryOperator(TypeOp_Or, register_id, left_register_id,
                       right_register_id);
      }
    }
  } else {
    long right_register_id = GenerateRegisterId();
    ast->right()->Accept(this, &right_register_id);
    if (!support_closure_) {
      int token = ast->op_token().token_;
      if (token == '&' || token == '|' || token == '^' || token == Token_Pow) {
        throw CompileException("engine version is less 1.4",
                               "'&' '+' '^' '**' is not supported",
                               ast->op_token(), GetPartStr(ast->op_token()));
      }
    }
    switch (ast->op_token().token_) {
      case '+':
        BinaryOperator(TypeOp_Add, register_id, left_register_id,
                       right_register_id);
        break;
      case '-':
        BinaryOperator(TypeOp_Sub, register_id, left_register_id,
                       right_register_id);
        break;
      case '*':
        BinaryOperator(TypeOp_Mul, register_id, left_register_id,
                       right_register_id);
        break;
      case '/':
        BinaryOperator(TypeOp_Div, register_id, left_register_id,
                       right_register_id);
        break;
      case Token_Pow:
        BinaryOperator(TypeOp_Pow, register_id, left_register_id,
                       right_register_id);
        break;
      case '%':
        BinaryOperator(TypeOp_Mod, register_id, left_register_id,
                       right_register_id);
        break;
      case '<':
        BinaryOperator(TypeOp_Less, register_id, left_register_id,
                       right_register_id);
        break;
      case '>':
        BinaryOperator(TypeOp_Greater, register_id, left_register_id,
                       right_register_id);
        break;
      case '|':
        BinaryOperator(TypeOp_BitOr, register_id, left_register_id,
                       right_register_id);
        break;
      case '&':
        BinaryOperator(TypeOp_BitAnd, register_id, left_register_id,
                       right_register_id);
        break;
      case '^':
        BinaryOperator(TypeOp_BitXor, register_id, left_register_id,
                       right_register_id);
        break;
      case Token_Equal:
        BinaryOperator(TypeOp_Equal, register_id, left_register_id,
                       right_register_id);
        break;
      case Token_AbsEqual:
        if (support_closure_) {
          BinaryOperator(TypeOp_AbsEqual, register_id, left_register_id,
                         right_register_id);
        } else {
          BinaryOperator(TypeOp_Equal, register_id, left_register_id,
                         right_register_id);
        }
        break;
      case Token_NotEqual:
        BinaryOperator(TypeOp_UnEqual, register_id, left_register_id,
                       right_register_id);
        break;
      case Token_AbsNotEqual:
        if (support_closure_) {
          BinaryOperator(TypeOp_AbsUnEqual, register_id, left_register_id,
                         right_register_id);
        } else {
          BinaryOperator(TypeOp_UnEqual, register_id, left_register_id,
                         right_register_id);
        }
        break;
      case Token_LessEqual:
        BinaryOperator(TypeOp_LessEqual, register_id, left_register_id,
                       right_register_id);
        break;
      case Token_GreaterEqual:
        BinaryOperator(TypeOp_GreaterEqual, register_id, left_register_id,
                       right_register_id);
        break;
      default:
        MOVE(register_id, right_register_id);
        break;
    }
  }
}

void CodeGenerator::Visit(UnaryExpression* ast, void* data) {
  int register_id = *static_cast<int*>(data);
  AstLineScope sop(current_function_->function_.get(), ast);
  fml::RefPtr<Function> function = current_function_->function_;
  ast->expression()->Accept(this, data);
  if (!support_closure_) {
    if (ast->op_token().token_ == '~' || ast->op_token().token_ == '+') {
      throw CompileException("The current Sdk Version is less than 1.4,",
                             "+ and ~ is not supported", ast->op_token(),
                             GetPartStr(ast->op_token()));
    }
  }
  switch (ast->op_token().token_) {
    case '-':
      AutomaticOperator(TypeOp_Neg, register_id);
      break;
    case '!':
      AutomaticOperator(TypeOp_Not, register_id);
      break;
    case '~':
      AutomaticOperator(TypeOp_BitNot, register_id);
      break;
    case '+':
      AutomaticOperator(TypeOp_Pos, register_id);
      break;
    case Token_Typeof:
      AutomaticOperator(TypeOp_Typeof, register_id);
      break;
    default:
      NOOP;
      break;
  }
}

void CodeGenerator::Visit(ExpressionListAST* ast, void* data) {
  bool gen_new_id = (!data) || (*static_cast<int*>(data) + 1 >=
                                current_function_->register_id_);

  for (ASTreeVector::iterator iter = ast->expressions().begin();
       iter != ast->expressions().end(); ++iter) {
    if (*iter) {
      long register_id =
          gen_new_id ? GenerateRegisterId() : (*static_cast<int*>(data) + 1);
      gen_new_id = true;
      (*iter)->Accept(this, &register_id);
    }
  }
}

void CodeGenerator::Visit(VariableAST* ast, void* data) {
  long register_id = GenerateRegisterId();
  AstLineScope sop(current_function_->function_.get(), ast);
  InsertVariable(ast->identifier().str_, register_id);
  long array_index = -1;
  if (support_closure_) {
    auto iter =
        upvalue_array_.find({ast->identifier().str_, GetCurrentBlockId()});
    if (iter != upvalue_array_.end()) {
      array_index = iter->second;
    }
  }

  if (!ast->expression()) {
    // use register b to show it may as LoadUndefined, it is decided in runtime
    Instruction instruction =
        Instruction::ABCode(TypeOp_LoadNil, register_id, 1);
    current_function_->function_->AddInstruction(instruction);
  } else {
    ast->expression()->Accept(this, &register_id);
  }

  if (support_closure_) {
    if (array_index != -1) {
      auto instruction = Instruction::ABCCode(TypeOp_SetContextSlotMove,
                                              context_->GetCurrentContextReg(),
                                              array_index, register_id);
      current_function_->function_->AddInstruction(instruction);
      current_function_->current_block_->closure_variables_outside_.insert(
          {ast->identifier().str_,
           {array_index, context_->GetCurrentContextReg()}});
    }
  }
}

void CodeGenerator::Visit(CatchVariableAST* ast, void* data) {
  long register_id = GenerateRegisterId();
  AstLineScope sop(current_function_->function_.get(), ast);
  InsertVariable(ast->identifier().str_, register_id);
  if (!ast->expression()) {
    Instruction instruction =
        Instruction::ACode(TypeOp_SetCatchId, register_id);
    current_function_->function_->AddInstruction(instruction);
  }
}

void CodeGenerator::Visit(VariableListAST* ast, void* data) {
  for (VariableASTVector::iterator iter = ast->variable_list().begin();
       iter != ast->variable_list().end(); ++iter) {
    (*iter)->Accept(this, nullptr);
  }
}

void CodeGenerator::Visit(FunctionStatementAST* ast, void* data) {
  long child_index = 0;
  long register_id =
      data != nullptr ? *static_cast<long*>(data) : GenerateRegisterId();
  AstLineScope sop(current_function_->function_.get(), ast);
  if (ast->function_name().token_ != Token_EOF) {
    InsertVariable(ast->function_name().str_, register_id);
  }

  {
    current_function_name_ = ast->function_name().str_;
    LineScope lsp(this, ast);
    Guard<CodeGenerator> g(this, &CodeGenerator::EnterFunction,
                           &CodeGenerator::LeaveFunction);
    {
      Guard<CodeGenerator> g(this, &CodeGenerator::EnterBlock,
                             &CodeGenerator::LeaveBlock);

      child_index = current_function_->function_->index();
      if (ast->params()) {
        auto cast_result = static_cast<NamesAST*>(ast->params().get());
        if (cast_result) {
          current_function_->function_->SetParamsSize(
              static_cast<int32_t>(cast_result->names().size()));
        }
        if (support_closure_) {
          // put params into funtion_parms
          if (cast_result) {
            for (auto iter_name : cast_result->names()) {
              current_function_->function_params_.push_back(iter_name.str_);
            }
          }
        }
        ast->params()->Accept(this, nullptr);
      }
      AstLineScope sop1(current_function_->function_.get(), ast);

      // we muse put ContextScope here after ast->parms visit
      // because params register ids must after function's register id
      ContextScope context_scope(this);
      if (support_closure_) {
        auto params = current_function_->function_params_;
        for (auto iter : params) {
          long var_reg_id = SearchVariable(iter);
          long array_index = GetUpvalueArrayIndex(iter);
          if (array_index != -1) {
            auto instruction = Instruction::ABCCode(
                TypeOp_SetContextSlotMove, context_->GetCurrentContextReg(),
                array_index, var_reg_id);
            current_function_->function_->AddInstruction(instruction);
            current_function_->current_block_->closure_variables_outside_
                .insert(
                    {iter, {array_index, context_->GetCurrentContextReg()}});
          }
        }
      }
      ast->body()->Accept(this, nullptr);
    }
  }

  Instruction i =
      Instruction::ABxCode(TypeOp_Closure, register_id, child_index);
  current_function_->function_->AddInstruction(i);

  if (support_closure_) {
    const auto& function_name = ast->function_name().str_;
    long array_index = GetUpvalueArrayIndex(function_name);
    if (array_index != -1 && context_->GetCurrentContextReg() != -1) {
      auto instruction = Instruction::ABCCode(TypeOp_SetContextSlotMove,
                                              context_->GetCurrentContextReg(),
                                              array_index, register_id);
      current_function_->function_->AddInstruction(instruction);
      current_function_->current_block_->closure_variables_outside_.insert(
          {function_name, {array_index, context_->GetCurrentContextReg()}});
    }
  }
}

void CodeGenerator::Visit(ForStatementAST* ast, void* data) {
  fml::RefPtr<Function> function = current_function_->function_;
  RegisterBoundary regs(current_function_);
  LineScope lsp(this, ast);
  AstLineScope sop(current_function_->function_.get(), ast);
  Guard<CodeGenerator> g(this, &CodeGenerator::EnterBlock,
                         &CodeGenerator::LeaveBlock);
  function->PushLoopBlockStack(GetCurrentBlockId());

  if (ast->statement1()) {
    ast->statement1()->Accept(this, nullptr);
  }

  Guard<CodeGenerator> l(this, &CodeGenerator::EnterLoop,
                         &CodeGenerator::LeaveLoop);

  long jmp_index = -1;
  if (ast->statement2()) {
    long register_id = GenerateRegisterId();
    ast->statement2()->Accept(this, &register_id);
    Instruction instruction =
        Instruction::ABxCode(TypeOp_JmpFalse, register_id, 0);
    jmp_index = function->AddInstruction(instruction);
  }
  if (ast->block()) {
    ast->block()->Accept(this, nullptr, true);
  }
  current_function_->current_loop_->loop_continue_index_ =
      function->OpCodeSize();

  for (ASTreeVector::iterator iter = ast->statement3().begin();
       iter != ast->statement3().end(); ++iter) {
    if (*iter) {
      (*iter)->Accept(this, nullptr);
    }
  }

  Instruction instruction = Instruction::ABxCode(TypeOp_Jmp, 0, 0);
  long loop_head_index = function->AddInstruction(instruction);
  LoopGenerate* current_loop = current_function_->current_loop_.get();
  if (current_loop) {
    current_loop->loop_infos_.push_back(
        LoopInfo(LoopJmp_Head, loop_head_index));
  }

  long end_index = function->OpCodeSize();
  if (jmp_index != -1) {
    function->GetInstruction(jmp_index)->RefillsBx(end_index - jmp_index);
  }
  function->PopLoopBlockStack();
}

void CodeGenerator::Visit(TryCatchFinallyStatementAST* ast, void* data) {
  fml::RefPtr<Function> function = current_function_->function_;
  AstLineScope sop(current_function_->function_.get(), ast);
  RegisterBoundary regs(current_function_);
  LineScope lsp(this, ast);
  Guard<CodeGenerator> g(this, &CodeGenerator::EnterBlock,
                         &CodeGenerator::LeaveBlock);

  if (ast->try_block()) {
    Guard<CodeGenerator> g_trycatch(this, &CodeGenerator::EnterTryCatch,
                                    &CodeGenerator::LeaveTryCatch);
    {
      ast->try_block()->Accept(this, nullptr);
      Instruction instruction = Instruction::ABxCode(TypeOp_Jmp, 0, 0);
      long finally_index = function->AddInstruction(instruction);
      TryCatchGenerate* current_trycatch =
          current_function_->current_tryCatch_.get();
      if (current_trycatch) {
        current_trycatch->trycatch_infos_.emplace_back(
            TryCatchInfo(TryCatchJmp_finally, finally_index));
      }
    }

    if (ast->catch_block()) {
      Instruction instruction = Instruction::Code(TypeLabel_Catch);
      function->AddInstruction(instruction);
      ast->catch_block()->Accept(this, nullptr);
    }

    if (ast->finally_block()) {
      current_function_->current_tryCatch_->finally_index_ =
          function->OpCodeSize();
      ast->finally_block()->Accept(this, nullptr);
    } else {
      current_function_->current_tryCatch_->finally_index_ =
          function->OpCodeSize();
    }
  }
}

void CodeGenerator::Visit(DoWhileStatementAST* ast, void* data) {
  fml::RefPtr<Function> function = current_function_->function_;
  RegisterBoundary regs(current_function_);
  AstLineScope sop(current_function_->function_.get(), ast);
  {
    Guard<CodeGenerator> g(this, &CodeGenerator::EnterLoop,
                           &CodeGenerator::LeaveLoop);
    {
      LineScope lsp(this, ast);
      Guard<CodeGenerator> g(this, &CodeGenerator::EnterBlock,
                             &CodeGenerator::LeaveBlock);
      function->PushLoopBlockStack(GetCurrentBlockId());
      ast->block()->Accept(this, nullptr, true);

      current_function_->current_loop_->loop_continue_index_ =
          function->OpCodeSize();

      long register_id = GenerateRegisterId();
      ast->condition()->Accept(this, &register_id);
      Instruction jmp_false_instruction =
          Instruction::ABxCode(TypeOp_JmpFalse, register_id, 2);
      function->AddInstruction(jmp_false_instruction);

      Instruction instruction = Instruction::ABxCode(TypeOp_Jmp, 0, 0);
      long loop_head_index = function->AddInstruction(instruction);
      LoopGenerate* current_loop = current_function_->current_loop_.get();
      if (current_loop) {
        current_loop->loop_infos_.push_back(
            LoopInfo(LoopJmp_Head, loop_head_index));
      }
      function->PopLoopBlockStack();
    }
  }
}

void CodeGenerator::Visit(BreakStatementAST* ast, void* data) {
  GenLeaveBlockScopeIns();
  fml::RefPtr<Function> function = current_function_->function_;
  AstLineScope sop(current_function_->function_.get(), ast);
  Instruction instruction = Instruction::ABxCode(TypeOp_Jmp, 0, 0);
  long break_index = function->AddInstruction(instruction);
  LoopGenerate* current_loop = current_function_->current_loop_.get();
  if (current_loop) {
    current_loop->loop_infos_.push_back(LoopInfo(LoopJmp_Tail, break_index));
  }
}

void CodeGenerator::Visit(ContinueStatementAST* ast, void* data) {
  GenLeaveBlockScopeIns();
  fml::RefPtr<Function> function = current_function_->function_;
  AstLineScope sop(current_function_->function_.get(), ast);
  Instruction instruction = Instruction::ABxCode(TypeOp_Jmp, 0, 0);
  long continue_index = function->AddInstruction(instruction);
  LoopGenerate* current_loop = current_function_->current_loop_.get();
  if (current_loop) {
    current_loop->loop_infos_.push_back(
        LoopInfo(LoopJmp_Continue, continue_index));
  }
}

void CodeGenerator::Visit(WhileStatementAST* ast, void* data) {
  fml::RefPtr<Function> function = current_function_->function_;
  AstLineScope sop(current_function_->function_.get(), ast);
  RegisterBoundary regs(current_function_);
  {
    Guard<CodeGenerator> g(this, &CodeGenerator::EnterLoop,
                           &CodeGenerator::LeaveLoop);

    long register_id = GenerateRegisterId();
    ast->condition()->Accept(this, &register_id);
    Instruction instruction =
        Instruction::ABxCode(TypeOp_JmpFalse, register_id, 0);
    long jmp_index = function->AddInstruction(instruction);
    {
      LineScope lsp(this, ast);
      Guard<CodeGenerator> g(this, &CodeGenerator::EnterBlock,
                             &CodeGenerator::LeaveBlock);
      function->PushLoopBlockStack(GetCurrentBlockId());
      ast->block()->Accept(this, nullptr, true);

      current_function_->current_loop_->loop_continue_index_ =
          function->OpCodeSize();

      Instruction instruction = Instruction::ABxCode(TypeOp_Jmp, 0, 0);
      long loop_head_index = function->AddInstruction(instruction);
      LoopGenerate* current_loop = current_function_->current_loop_.get();
      if (current_loop) {
        current_loop->loop_infos_.push_back(
            LoopInfo(LoopJmp_Head, loop_head_index));
      }
      function->PopLoopBlockStack();
    }

    long end_index = function->OpCodeSize();
    function->GetInstruction(jmp_index)->RefillsBx(end_index - jmp_index);
  }
}

void CodeGenerator::Visit(IfStatementAST* ast, void* data) {
  fml::RefPtr<Function> function = current_function_->function_;
  AstLineScope sop(current_function_->function_.get(), ast);
  RegisterBoundary regs(current_function_);
  {
    long register_id = GenerateRegisterId();
    ast->condition()->Accept(this, &register_id);
    Instruction instruction =
        Instruction::ABxCode(TypeOp_JmpFalse, register_id, 0);
    long jmp_index = function->AddInstruction(instruction);

    LineScope lsp(this, ast);
    Guard<CodeGenerator> g(this, &CodeGenerator::EnterBlock,
                           &CodeGenerator::LeaveBlock);

    ast->true_branch()->Accept(this, nullptr);

    instruction = Instruction::ABxCode(TypeOp_Jmp, 0, 0);
    long true_jmp_index = function->AddInstruction(instruction);

    long index = function->OpCodeSize();
    function->GetInstruction(jmp_index)->RefillsBx(index - jmp_index);
    if (ast->false_branch()) {
      ast->false_branch()->Accept(this, nullptr);
    }
    long end_index = function->OpCodeSize();
    function->GetInstruction(true_jmp_index)
        ->RefillsBx(end_index - true_jmp_index);
  }
}

void CodeGenerator::Visit(ElseStatementAST* ast, void* data) {
  AstLineScope sop(current_function_->function_.get(), ast);
  Guard<CodeGenerator> g(this, &CodeGenerator::EnterBlock,
                         &CodeGenerator::LeaveBlock);
  ast->block()->Accept(this, nullptr);
}

void CodeGenerator::Visit(CaseStatementAST* ast, void* data) {
  AstLineScope sop(current_function_->function_.get(), ast);
  ast->block()->Accept(this, nullptr);
}

void CodeGenerator::Visit(AssignStatement* ast, void* data) {
  RegisterBoundary regs(current_function_);
  fml::RefPtr<Function> function = current_function_->function_;
  AstLineScope sop(current_function_->function_.get(), ast);

  if (ast->variable()->type() == ASTType_MemberAccessor) {
    MemberAccessorAST* property =
        reinterpret_cast<MemberAccessorAST*>(ast->variable().get());

    long table_reg = GenerateRegisterId();
    property->table()->Accept(this, &table_reg);

    long member_reg_id = GenerateRegisterId();
    property->member()->Accept(this, &member_reg_id);

    long accumulate = GenerateRegisterId();
    if (ast->lex_op() == LexicalOp_Write) {
      ast->expression()->Accept(this, &accumulate);
      function->AddInstruction(Instruction::ABCCode(TypeOp_SetTable, table_reg,
                                                    member_reg_id, accumulate));
    } else {
      TypeOpCode op;
      switch (ast->lex_op()) {
        case LexicalOp_ASSIGN_ADD:
          op = TypeOp_Add;
          break;
        case LexicalOp_ASSIGN_SUB:
          op = TypeOp_Sub;
          break;
        case LexicalOp_ASSIGN_MUL:
          op = TypeOp_Mul;
          break;
        case LexicalOp_ASSIGN_DIV:
          op = TypeOp_Div;
          break;
        case LexicalOp_ASSIGN_MOD:
          op = TypeOp_Mod;
          break;
        case LexicalOp_ASSIGN_BIT_OR:
          op = TypeOp_BitOr;
          break;
        case LexicalOp_ASSIGN_BIT_AND:
          op = TypeOp_BitAnd;
          break;
        case LexicalOp_ASSIGN_BIT_XOR:
          op = TypeOp_BitXor;
          break;
        case LexicalOp_ASSIGN_POW:
          op = TypeOp_Pow;
          break;
        default:
          return;
      }

      function->AddInstruction(Instruction::ABCCode(TypeOp_GetTable, accumulate,
                                                    table_reg, member_reg_id));
      long src = GenerateRegisterId();
      ast->expression()->Accept(this, &src);
      BinaryOperator(op, accumulate, accumulate, src);
      function->AddInstruction(Instruction::ABCCode(TypeOp_SetTable, table_reg,
                                                    member_reg_id, accumulate));
    }

    if (data) {
      MOVE((*static_cast<int*>(data)), accumulate);
    }
    return;
  }

  long source_id = GenerateRegisterId();
  ast->expression()->Accept(this, &source_id);
  ast->variable()->Accept(this, &source_id);
  if (data != nullptr) {
    long register_id = *static_cast<int*>(data);
    MOVE(register_id, source_id);
  }
}

void CodeGenerator::AddOptionalChainInfo(ASTree* ast, Function* function,
                                         long result_reg_id) {
  bool is_optional = false;
  if ((ast->type() == ASTType_MemberAccessor &&
       static_cast<MemberAccessorAST*>(ast)->is_optional()) ||
      static_cast<FunctionCallAST*>(ast)->is_optional()) {
    is_optional = true;
  }

  OptionalChainInfo op_info(function, is_optional);
  if (is_optional) {
    // generate instructions which compare result and null/undefined
    // and jmp instructions if result equals to null or undefined
    long reg_id = GenerateRegisterId();
    function->AddInstruction(Instruction::ABCode(TypeOp_LoadNil, reg_id, 0));
    BinaryOperator(TypeOp_AbsEqual, reg_id, result_reg_id, reg_id);
    op_info.move_indexes_.push_back(function->AddInstruction(
        Instruction::ABCode(TypeOp_LoadNil, 0, 1)));  // need refill register id
    op_info.jmp_indexes_.push_back(
        function->AddInstruction(Instruction::ABxCode(
            TypeOp_JmpTrue, reg_id, 0)));  // need refill jmp_index
    function->AddInstruction(Instruction::ABCode(TypeOp_LoadNil, reg_id, 1));
    BinaryOperator(TypeOp_AbsEqual, reg_id, result_reg_id, reg_id);
    op_info.move_indexes_.push_back(function->AddInstruction(
        Instruction::ABCode(TypeOp_LoadNil, 0, 1)));  // need refill register id
    op_info.jmp_indexes_.push_back(
        function->AddInstruction(Instruction::ABxCode(
            TypeOp_JmpTrue, reg_id, 0)));  // need refill jmp_index
  }
  op_info.current_jmp_index_ =
      function->OpCodeSize();  // current ast's end instruction pc
  optional_chain_infos_.insert(std::make_pair(ast, op_info));
}

void CodeGenerator::Visit(MemberAccessorAST* ast, void* data) {
  RegisterBoundary regs(current_function_);
  fml::RefPtr<Function> function = current_function_->function_;
  AstLineScope sop(function.get(), ast);

  long reg = GenerateRegisterId();
  ast->table()->Accept(this, &reg);

  AddOptionalChainInfo(ast, function.get(), reg);

  long member_reg_id = GenerateRegisterId();
  ast->member()->Accept(this, &member_reg_id);

  long result_id =
      data != nullptr ? *static_cast<int*>(data) : GenerateRegisterId();
  function->AddInstruction(
      Instruction::ABCCode(TypeOp_GetTable, result_id, reg, member_reg_id));
  optional_chain_infos_.at(ast).current_jmp_index_ = function->OpCodeSize();
  optional_chain_infos_.at(ast).current_result_id_ = result_id;
}

void CodeGenerator::CompleteOptionalChainInfos() {
  std::unordered_map<ASTree*, ASTree*> ast2primary_ast;
  // collect optional chaining ast's parent node
  for (auto& ast_info : optional_chain_infos_) {
    if (ast_info.first->type() ==
        ASTType_MemberAccessor) {  // MemberAccessorAST
      ast2primary_ast
          [static_cast<MemberAccessorAST*>(ast_info.first)->table().get()] =
              ast_info.first;
    } else {  // FunctionCall
      ast2primary_ast
          [static_cast<FunctionCallAST*>(ast_info.first)->caller().get()] =
              ast_info.first;
    }
  }

  // find the top ast node of the optional chaining ast
  // like disjoint-set's union operation
  while (true) {
    bool not_change = true;
    for (auto& ast_pair : ast2primary_ast) {
      if (ast2primary_ast.find(ast_pair.second) != ast2primary_ast.end()) {
        ast_pair.second = ast2primary_ast[ast_pair.second];
        not_change = false;
      }
    }
    if (not_change) break;
  }

  // use top ast node's OptionalChainInfo to refill result register_id and jmp
  // addr
  for (auto& ast_info : optional_chain_infos_) {
    if (ast_info.second.is_optional_) {
      ASTree* primary_ast = ast_info.first;
      if (ast2primary_ast.find(ast_info.first) != ast2primary_ast.end())
        primary_ast = ast2primary_ast[ast_info.first];

      ast_info.second.RefillMoveIndex(
          optional_chain_infos_.at(primary_ast).current_result_id_);
      ast_info.second.RefillJmpIndex(
          optional_chain_infos_.at(primary_ast).current_jmp_index_);
    }
  }
}

bool ShouldInsertThisParam(FunctionCallAST* ast) {
  if (ast->caller()->type() == ASTType_MemberAccessor) {
    MemberAccessorAST* property =
        static_cast<MemberAccessorAST*>(ast->caller().get());
    if (property->table()->type() == ASTType_Literal) {
      LiteralAST* ast = static_cast<LiteralAST*>(property->table().get());
      // for Global builtins like: base::String/Math should not pass [[this]]
      if (ast->token().token_ == Token_Id &&
          ast->scope() == LexicalScoping_Global) {
        return false;
      }
    }
    return true;
  }
  return false;
}

void CodeGenerator::Visit(FunctionCallAST* ast, void* data) {
  RegisterBoundary regs(current_function_);
  fml::RefPtr<Function> function = current_function_->function_;
  AstLineScope sop(function.get(), ast);
  long return_register_id =
      data == nullptr ? GenerateRegisterId() : *static_cast<int*>(data);
  long resolve_this_register = GenerateRegisterId();
  long caller_register_id = GenerateRegisterId();
  if (ShouldInsertThisParam(ast)) {
    RegisterBoundary regs(current_function_);
    MemberAccessorAST* property =
        static_cast<MemberAccessorAST*>(ast->caller().get());
    property->table()->Accept(this, &resolve_this_register);

    AddOptionalChainInfo(property, function.get(), resolve_this_register);

    long member_reg_id = GenerateRegisterId();
    property->member()->Accept(this, &member_reg_id);

    function->AddInstruction(
        Instruction::ABCCode(TypeOp_GetTable, caller_register_id,
                             resolve_this_register, member_reg_id));

    optional_chain_infos_.at(property).current_jmp_index_ =
        function->OpCodeSize();
    optional_chain_infos_.at(property).current_result_id_ = caller_register_id;
  } else {
    ast->caller()->Accept(this, &caller_register_id);
  }

  AddOptionalChainInfo(ast, function.get(), caller_register_id);
  if (ast->is_optional()) DestoryRegisterId();

  {
    // need to have the same block structure with semantic_analysis(for closure
    // implement)
    LineScope lsp(this, ast);
    Guard<CodeGenerator> g(this, &CodeGenerator::EnterBlock,
                           &CodeGenerator::LeaveBlock);
    long argc = ast->args() ? static_cast<ExpressionListAST*>(ast->args().get())
                                  ->expressions()
                                  .size()
                            : 0;

    ast->args()->Accept(this, nullptr);
    int base_arg = 0;
    // 1. function call like array.push should pass [[this]] as the first arg
    // 2. for Global builtins like: base::String/Math should not pass [[this]]
    if (ShouldInsertThisParam(ast)) {
      long this_reg = GenerateRegisterId();
      MOVE(this_reg, resolve_this_register);
      base_arg = 1;
    }

    Call(caller_register_id, argc + base_arg, return_register_id);

    optional_chain_infos_.at(ast).current_jmp_index_ = function->OpCodeSize();
    optional_chain_infos_.at(ast).current_result_id_ = return_register_id;
  }
}
void CodeGenerator::Visit(TernaryStatementAST* ast, void* data) {
  RegisterBoundary regs(current_function_);
  fml::RefPtr<Function> function = current_function_->function_;
  AstLineScope sop(function.get(), ast);
  long condition_register_id = GenerateRegisterId();

  ast->condition()->Accept(this, &condition_register_id);
  Instruction instruction =
      Instruction::ABxCode(TypeOp_JmpFalse, condition_register_id, 0);
  long jmp_index = function->AddInstruction(instruction);
  DestoryRegisterId();
  ast->true_branch()->Accept(this, data);
  instruction = Instruction::ABxCode(TypeOp_Jmp, 0, 0);
  long true_jmp_index = function->AddInstruction(instruction);
  long index = function->OpCodeSize();
  function->GetInstruction(jmp_index)->RefillsBx(index - jmp_index);

  ast->false_branch()->Accept(this, data);
  long end_index = function->OpCodeSize();
  function->GetInstruction(true_jmp_index)
      ->RefillsBx(end_index - true_jmp_index);
}

void CodeGenerator::Visit(ObjectLiteralAST* ast, void* data) {
  RegisterBoundary regs(current_function_);
  long register_id =
      data == nullptr ? GenerateRegisterId() : *static_cast<long*>(data);
  fml::RefPtr<Function> function = current_function_->function_;
  AstLineScope sop(function.get(), ast);
  auto instruction = Instruction::ACode(TypeOp_NewTable, register_id);
  function->AddInstruction(instruction);

  long key_reg = GenerateRegisterId();
  long value_reg = GenerateRegisterId();
  base::SortedForEach(ast->property(), [this, &function, &key_reg, &value_reg,
                                        &register_id](const auto& it) {
    long key_index = function->AddConstString(it.first);
    function->AddInstruction(
        Instruction::ABxCode(TypeOp_LoadConst, key_reg, key_index));
    it.second->Accept(this, &value_reg);
    function->AddInstruction(
        Instruction::ABCCode(TypeOp_SetTable, register_id, key_reg, value_reg));
  });
}

void CodeGenerator::Visit(ArrayLiteralAST* ast, void* data) {
  RegisterBoundary regs(current_function_);
  fml::RefPtr<Function> function = current_function_->function_;
  AstLineScope sop(function.get(), ast);
  long register_id =
      data == nullptr ? GenerateRegisterId() : *static_cast<int*>(data);
  long argc =
      ast->values() != nullptr ? ast->values()->expressions().size() : 0;
  ast->values()->Accept(this, &register_id);
  auto instruction = Instruction::ABCode(TypeOp_NewArray, register_id, argc);
  function->AddInstruction(instruction);
}

void CodeGenerator::CreateAndPushContext(bool is_block) {
  long array_register_id = GenerateRegisterId();
  context_->PushCurrentContext(array_register_id);
  if (is_block) {
    Instruction instruction = Instruction::ABCode(
        TypeOp_CreateBlockContext, array_register_id, upvalue_array_.size());
    current_function_->function_->AddInstruction(instruction);
  } else {
    Instruction instruction = Instruction::ABCode(
        TypeOp_CreateContext, array_register_id, upvalue_array_.size());
    current_function_->function_->AddInstruction(instruction);
    Instruction instruction1 =
        Instruction::ACode(TypeOp_PushContext, array_register_id);
    current_function_->function_->AddInstruction(instruction1);
  }
}

void CodeGenerator::PopContext() {
  Instruction instruction = Instruction::Code(TypeOp_PopContext);
  current_function_->function_->AddInstruction(instruction);
}

long CodeGenerator::GenerateRegisterId() {
  long register_id = current_function_->register_id_++;

  if (register_id >= 256) {
    Token fake_token;
    throw CompileException(
        "Register ID is overflow! The max register id is 256!", fake_token, "");
  }
  return register_id;
}

// get all the parent block id of current block(for closure implement)
void BlockGenerate::SetParentBlockID(BlockGenerate* parent_block,
                                     FunctionGenerate* parent_function) {
  if (!parent_block) {
    if (parent_function) {
      parent_block_ids_ = parent_function->current_block_->parent_block_ids_;
      parent_block_ids_.emplace_back(
          parent_function->current_block_->GetBlockID());
    } else {
      parent_block_ids_.clear();
    }
  } else {
    parent_block_ids_ = parent_block->parent_block_ids_;
    parent_block_ids_.emplace_back(parent_block->GetBlockID());
  }
}

void BlockGenerate::Dump(int32_t tab_size) {
  int32_t start_line = 0;
  int32_t start_col = 0;
  int32_t end_line = 0;
  int32_t end_col = 0;

  Function::DecodeLineCol(start_line_col_, start_line, start_col);
  Function::DecodeLineCol(end_line_col_, end_line, end_col);

  for (size_t i = 0; i < static_cast<size_t>(tab_size); i++) {
    std::cout << "-";
  }
  std::cout << "{ " << std::endl;

  tab_size++;
  for (size_t i = 0; i < static_cast<size_t>(tab_size); i++) {
    std::cout << "-";
  }
  std::cout << "start: " << start_line << " : " << start_col << " => "
            << end_line << " : " << end_col << std::endl;

  for (size_t i = 0; i < static_cast<size_t>(tab_size); i++) {
    std::cout << "-";
  }
  std::cout << "Normal: " << std::endl;
  for (const auto& element : variables_map_) {
    if (closure_variables_.find(element.first) != closure_variables_.end()) {
      continue;
    }
    if (closure_variables_outside_.find(element.first) !=
        closure_variables_outside_.end()) {
      continue;
    }
    for (size_t i = 0; i < static_cast<size_t>(tab_size); i++) {
      std::cout << "-";
    }
    std::cout << element.first.c_str() << " : " << element.second << std::endl;
  }

  for (const auto& element : closure_variables_outside_) {
    for (size_t i = 0; i < static_cast<size_t>(tab_size); i++) {
      std::cout << "-";
    }
    std::cout << element.first.c_str() << " : (" << element.second.first << ", "
              << element.second.second << " )" << std::endl;
  }

  for (size_t i = 0; i < static_cast<size_t>(tab_size); i++) {
    std::cout << "-";
  }
  std::cout << "Closure: " << std::endl;
  for (const auto& element : closure_variables_) {
    for (size_t i = 0; i < static_cast<size_t>(tab_size); i++) {
      std::cout << "-";
    }
    std::cout << element.first.c_str() << " : (" << element.second.first << ", "
              << element.second.second << std::endl;
  }

  tab_size++;
  for (const auto& element : childs_) {
    element->Dump(tab_size);
  }

  for (size_t i = 0; i < static_cast<size_t>(tab_size) - 2; i++) {
    std::cout << "-";
  }
  std::cout << "}" << std::endl;
}

Value BlockGenerate::GenerateScope(const Value& prev) {
  // for normal variables
  Value scope = Value::CreateObject(nullptr);
  Value childs = Value(CArray::Create());
  scope.SetProperty(Function::kLepusPrev, Value());
  scope.SetProperty(Function::kStartLine, Value(start_line_col_));
  scope.SetProperty(Function::kEndLine, Value(end_line_col_));

  base::SortedForEach(variables_map_, [this, &scope](const auto& element) {
    if (closure_variables_.find(element.first) != closure_variables_.end()) {
      return;
    }
    if (closure_variables_outside_.find(element.first) !=
        closure_variables_outside_.end()) {
      return;
    }
    scope.SetProperty(element.first,
                      Value(Function::EncodeVariableInfo(
                          0, static_cast<int>(element.second), 0, 0)));
  });

  base::SortedForEach(
      closure_variables_outside_, [&scope](const auto& element) {
        scope.SetProperty(element.first,
                          Value(Function::EncodeVariableInfo(
                              2, 0, static_cast<int>(element.second.first),
                              static_cast<int>(element.second.second))));
      });

  base::SortedForEach(closure_variables_, [&scope](const auto& element) {
    scope.SetProperty(element.first,
                      Value(Function::EncodeVariableInfo(
                          1, 0, static_cast<int>(element.second.first),
                          static_cast<int>(element.second.second))));
  });

  for (const auto& element : childs_) {
    childs.Array()->push_back(element->GenerateScope(scope));
  }
  scope.SetProperty(Function::kChilds, childs);
  return scope;
}

void FunctionGenerate::Dump(int32_t tab_size) {
  std::cout << "Function... " << std::endl;
  for (const auto& element : blocks_) {
    element->Dump(tab_size);
  }

  for (const auto& element : childs_) {
    element->Dump(++tab_size);
  }
}

void FunctionGenerate::GenerateScope(const Value& firstScope) {
  DCHECK(blocks_.size() == 1);
  BlockGenerate* block = *blocks_.begin();
  Value scope = block->GenerateScope(firstScope);
  function_->SetScope(scope);
  for (const auto& element : childs_) {
    element->GenerateScope(scope);
  }
}

}  // namespace lepus
}  // namespace lynx
