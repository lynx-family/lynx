// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/semantic_analysis.h"

#include "base/include/sorted_for_each.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/guard.h"

namespace lynx {
namespace lepus {

enum ExprType {
  ExprType_Unknown,  // unknown type at semantic analysis phase
  ExprType_Nil,
  ExprType_Bool,
  ExprType_Number,
  ExprType_String,
  ExprType_Table,
};

struct ExprData {
  ExprType expr_type_;
  LexicalOp lex_po_;
  ExprData() : expr_type_(ExprType_Unknown), lex_po_(LexicalOp_None) {}
};
SemanticAnalysis::SemanticAnalysis() : sdk_version_("null") {}
void SemanticAnalysis::EnterFunction() {
  LexicalFunction* function = new LexicalFunction;
  function->parent_ = current_function_;
  current_function_.reset(function);
  size_t function_number =
      current_function_->SetFunctionNumber(GenerateFunctionNumber());
  function_map_[function_number] = current_function_;
}

void SemanticAnalysis::LeaveFunction() {
  current_function_ = current_function_->parent_;
}

void LexicalBlock::AddUpvalue(const base::String& upvalue_name,
                              uint64_t block_id) {
  if (upvalue_array_.find({upvalue_name, block_id}) == upvalue_array_.end()) {
    upvalue_array_[{upvalue_name, block_id}] = upvalue_array_max_index_++;
  }
}

int64_t LexicalBlock::SetBlockNumber(int64_t block_number) {
  block_number_ = block_number;
  return block_number_;
}

void LexicalBlock::SetParentBlockID(
    std::shared_ptr<LexicalBlock>& parent_block,
    std::shared_ptr<LexicalFunction>& parent_function) {
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
    parent_block_ids_.push_back(parent_block->GetBlockID());
  }
}

void LexicalFunction::AddUpvalue(const base::String& upvalue_name,
                                 uint64_t block_id) {
  if (upvalue_array_.find({upvalue_name, block_id}) == upvalue_array_.end()) {
    upvalue_array_[{upvalue_name, block_id}] = ++upvalue_array_max_index_;
  }
}

void SemanticAnalysis::EnterBlock() {
  LexicalBlock* block = new LexicalBlock;
  block->SetBlockID(block_id_increase_++);
  block->parent_ = std::move(current_function_->current_block_);
  block->SetParentBlockID(block->parent_, current_function_->parent_);
  current_function_->current_block_.reset(block);
  auto current_block = current_function_->current_block_;
  int64_t block_number = current_block->SetBlockNumber(GenerateBlockNumber());
  block_map_[block_number] = current_block;
}

void SemanticAnalysis::LeaveBlock() {
  current_function_->current_block_ =
      current_function_->current_block_->parent_;
}

bool SemanticAnalysis::InsertName(const base::String& name) {
  LexicalBlock* block = current_function_->current_block_.get();
  if (block->names_.find(name) != block->names_.end()) {
    return false;
  }
  block->names_.insert(name);
  return true;
}

LexicalScoping SemanticAnalysis::SearchName(const base::String& name) {
  LexicalFunction* function = current_function_.get();
  while (function) {
    LexicalBlock* block = function->current_block_.get();
    while (block) {
      auto iter = block->names_.find(name);
      if (iter != block->names_.end()) {
        if (function == current_function_.get()) {
          return LexicalScoping_Local;
        } else if (function->parent_.get() ||
                   (closure_fix_ && block->parent_.get())) {
          // other upvalue
          // {name, block id this variable belongs}
          function->AddUpvalue(name, block->GetBlockID());
          block->AddUpvalue(name, block->GetBlockID());
          return LexicalScoping_Upvalue_New;
        } else {
          // top level upvalue
          return LexicalScoping_Upvalue;
        }
      }
      block = block->parent_.get();
    }
    function = function->parent_.get();
  }
  return LexicalScoping_Global;
}

void SemanticAnalysis::Visit(ChunkAST* ast, void* data) {
  Guard<SemanticAnalysis> g(this, &SemanticAnalysis::EnterFunction,
                            &SemanticAnalysis::LeaveFunction);
  ast->block()->Accept(this, nullptr);
}

void SemanticAnalysis::Visit(CatchBlockAST* ast, void* data) {
  Guard<SemanticAnalysis> g(this, &SemanticAnalysis::EnterBlock,
                            &SemanticAnalysis::LeaveBlock);
  if (ast->catch_identifier()) {
    ast->catch_identifier()->Accept(this, nullptr);
  }
  ast->block()->Accept(this, nullptr);
}

void SemanticAnalysis::Visit(BlockAST* ast, void* data) {
  Guard<SemanticAnalysis> g(this, &SemanticAnalysis::EnterBlock,
                            &SemanticAnalysis::LeaveBlock);
  for (ASTreeVector::iterator iter = ast->statements().begin();
       iter != ast->statements().end(); ++iter) {
    (*iter)->Accept(this, nullptr);
  }
  if (ast->return_statement()) {
    ast->return_statement()->Accept(this, nullptr);
  }
}

void SemanticAnalysis::Visit(BlockAST* ast, void* data, bool gen) {
  SemanticAnalysis::Visit(ast, data);
}

void SemanticAnalysis::Visit(ReturnStatementAST* ast, void* data) {
  ExprData expr_data;
  expr_data.lex_po_ = LexicalOp_Read;
  if (ast->expression()) ast->expression()->Accept(this, &expr_data);
}

void SemanticAnalysis::Visit(LiteralAST* ast, void* data) {
  ExprData* expr_data = static_cast<ExprData*>(data);
  switch (ast->token().token_) {
    case Token_Nil:
      if (expr_data) expr_data->expr_type_ = ExprType_Nil;
      break;
    case Token_Id:
      if (expr_data) expr_data->expr_type_ = ExprType_Unknown;
      break;
    case Token_Number:
      if (expr_data) expr_data->expr_type_ = ExprType_Number;
      break;
    case Token_String:
      if (expr_data) expr_data->expr_type_ = ExprType_String;
      break;
    case Token_False:
    case Token_True:
      if (expr_data) expr_data->expr_type_ = ExprType_Bool;
      break;
    default:
      break;
  }
  if (ast->token().token_ == Token_Id) {
    ast->scope() = SearchName(ast->token().str_);
  }
  ast->lex_op() = expr_data == nullptr ? LexicalOp_Read : expr_data->lex_po_;
}

void SemanticAnalysis::Visit(ThrowStatementAST* ast, void* data) {
  if (ast->throw_identifier()) {
    ast->throw_identifier()->Accept(this, nullptr);
  }
}

void SemanticAnalysis::Visit(NamesAST* ast, void* data) {
  for (std::vector<Token>::iterator iter = ast->names().begin();
       iter != ast->names().end(); ++iter) {
    if (!InsertName((*iter).str_)) {
      throw CompileException((*iter).str_.c_str(), " is already existed",
                             (*iter), GetPartStr((*iter)));
    }
  }
}

void SemanticAnalysis::Visit(BinaryExprAST* ast, void* data) {
  ExprData l_expr_data, r_expr_data;
  l_expr_data.lex_po_ = LexicalOp_Read;
  r_expr_data.lex_po_ = LexicalOp_Read;
  if (!ast->left()) {
    throw CompileException("something wrong with operator", ast->op_token(),
                           GetPartStr(ast->op_token()));
  }
  ast->left()->Accept(this, &l_expr_data);
  if (!ast->right()) {
    throw CompileException("something wrong with operator", ast->op_token(),
                           GetPartStr(ast->op_token()));
  }
  ast->right()->Accept(this, &r_expr_data);

  ExprData* expr_data = static_cast<ExprData*>(data);
  if (lynx::tasm::Config::IsHigherOrEqual(sdk_version_,
                                          FEATURE_CONTROL_VERSION_2) == false) {
    int ast_token_ = ast->op_token().token_;
    if (ast_token_ == '&' || ast_token_ == '|' || ast_token_ == '^') {
      throw CompileException("The current Sdk Version is less than 1.4,",
                             "something wrong with operator", ast->op_token(),
                             GetPartStr(ast->op_token()));
    }
  }
  switch (ast->op_token().token_) {
    case '+':
      if (!(l_expr_data.expr_type_ == ExprType_Number ||
            l_expr_data.expr_type_ == ExprType_String) &&
          l_expr_data.expr_type_ != ExprType_Unknown) {
        throw CompileException("left expression is not number or string",
                               ast->op_token(), GetPartStr(ast->op_token()));
      }
      if (!(r_expr_data.expr_type_ == ExprType_Number ||
            r_expr_data.expr_type_ == ExprType_String) &&
          r_expr_data.expr_type_ != ExprType_Unknown) {
        throw CompileException("right expression is not number or string",
                               ast->op_token(), GetPartStr(ast->op_token()));
      }
      if (r_expr_data.expr_type_ == ExprType_String ||
          l_expr_data.expr_type_ == ExprType_String) {
        expr_data->expr_type_ = ExprType_String;
      } else {
        expr_data->expr_type_ = ExprType_Number;
      }
      break;
    case '-':
    case '*':
    case '/':
    case '%':
    case '|':
    case '&':
    case '^':
    case Token_Pow:
      if (!(l_expr_data.expr_type_ == ExprType_Number ||
            l_expr_data.expr_type_ == ExprType_Unknown)) {
        throw CompileException("left expression is not number", ast->op_token(),
                               GetPartStr(ast->op_token()));
      }

      if (!(r_expr_data.expr_type_ == ExprType_Number ||
            r_expr_data.expr_type_ == ExprType_Unknown)) {
        throw CompileException("right expression is not number",
                               ast->op_token(), GetPartStr(ast->op_token()));
      }

      if (expr_data) expr_data->expr_type_ = ExprType_Number;
      break;
    case '<':
    case '>':
    case Token_LessEqual:
    case Token_GreaterEqual:
      if (l_expr_data.expr_type_ != r_expr_data.expr_type_ &&
          !(l_expr_data.expr_type_ == ExprType_Unknown ||
            r_expr_data.expr_type_ == ExprType_Unknown)) {
        throw CompileException("compare different expression type",
                               ast->op_token(), GetPartStr(ast->op_token()));
      }
      if (expr_data) expr_data->expr_type_ = ExprType_Bool;
      break;
    case Token_NotEqual:
    case Token_Equal:
    case Token_AbsNotEqual:
    case Token_AbsEqual:
      if (expr_data) expr_data->expr_type_ = ExprType_Bool;
      break;
    case Token_ASSIGN_BIT_OR:
    case Token_ASSIGN_BIT_XOR:
    case Token_ASSIGN_BIT_AND:
      if (!(l_expr_data.expr_type_ == ExprType_Number ||
            l_expr_data.expr_type_ == ExprType_Unknown)) {
        throw CompileException("left expression is not number", ast->op_token(),
                               GetPartStr(ast->op_token()));
      }
      if (!(r_expr_data.expr_type_ == ExprType_Number ||
            r_expr_data.expr_type_ == ExprType_Unknown)) {
        throw CompileException("right expression is not number",
                               ast->op_token(), GetPartStr(ast->op_token()));
      }
      break;
    default:
      break;
  }
}

void SemanticAnalysis::Visit(UnaryExpression* ast, void* data) {
  ExprData unary_expr_data;
  unary_expr_data.lex_po_ = LexicalOp_Read;
  ast->expression()->Accept(this, &unary_expr_data);
  if (unary_expr_data.expr_type_ != ExprType_Unknown) {
    switch (ast->op_token().token_) {
      case '-':
      case '+':
        if (unary_expr_data.expr_type_ != ExprType_Number &&
            unary_expr_data.expr_type_ != ExprType_String) {
          throw CompileException("operand is not number or string",
                                 ast->op_token(), GetPartStr(ast->op_token()));
        }
        break;
      case '~':
        if (unary_expr_data.expr_type_ != ExprType_Number) {
          throw CompileException("operand is not number", ast->op_token(),
                                 GetPartStr(ast->op_token()));
        }
        break;
      case '!':
        if (unary_expr_data.expr_type_ != ExprType_Bool &&
            unary_expr_data.expr_type_ != ExprType_Number) {
          throw CompileException("operand is not number or bool",
                                 ast->op_token(), GetPartStr(ast->op_token()));
        }
        break;
      default:
        break;
    }
  }

  ExprData* expr_data = static_cast<ExprData*>(data);
  if (expr_data == nullptr) return;
  if (ast->op_token().token_ == '-' || ast->op_token().token_ == '+' ||
      ast->op_token().token_ == '~') {
    expr_data->expr_type_ = ExprType_Number;
  } else if (ast->op_token().token_ == '!') {
    expr_data->expr_type_ = ExprType_Bool;
  } else if (ast->op_token().token_ == Token_Typeof) {
    expr_data->expr_type_ = ExprType_String;
  }
}

void SemanticAnalysis::Visit(ExpressionListAST* ast, void* data) {
  for (ASTreeVector::iterator iter = ast->expressions().begin();
       iter != ast->expressions().end(); ++iter) {
    if (*iter == NULL) continue;
    ExprData expr_data;
    expr_data.lex_po_ = LexicalOp_Read;
    (*iter)->Accept(this, &expr_data);
  }
}

void SemanticAnalysis::Visit(VariableAST* ast, void* data) {
  ExprData expr_data;
  expr_data.lex_po_ = LexicalOp_Read;
  if (ast->expression()) {
    ast->expression()->Accept(this, &expr_data);
  }

  if (ast->identifier().token_ != Token_Id) {
    throw CompileException(ast->identifier().str_.c_str(), "is invalid",
                           ast->identifier(), GetPartStr(ast->identifier()));
  }
  if (!InsertName(ast->identifier().str_)) {
    throw CompileException(ast->identifier().str_.c_str(),
                           " is already existed", ast->identifier(),
                           GetPartStr(ast->identifier()));
  }
}

void SemanticAnalysis::Visit(CatchVariableAST* ast, void* data) {
  ExprData expr_data;
  expr_data.lex_po_ = LexicalOp_Read;
  if (ast->expression()) {
    ast->expression()->Accept(this, &expr_data);
  }

  if (ast->identifier().token_ != Token_Id) {
    throw CompileException(ast->identifier().str_.c_str(), "is invalid",
                           ast->identifier(), GetPartStr(ast->identifier()));
  }
  if (!InsertName(ast->identifier().str_)) {
    throw CompileException(ast->identifier().str_.c_str(),
                           " is already existed", ast->identifier(),
                           GetPartStr(ast->identifier()));
  }
}

void SemanticAnalysis::Visit(VariableListAST* ast, void* data) {
  for (VariableASTVector::iterator iter = ast->variable_list().begin();
       iter != ast->variable_list().end(); ++iter) {
    (*iter)->Accept(this, nullptr);
  }
}

void SemanticAnalysis::Visit(FunctionStatementAST* ast, void* data) {
  if (ast->function_name().token_ != Token_EOF &&
      !InsertName(ast->function_name().str_)) {
    throw CompileException(ast->function_name().str_.c_str(),
                           " is already existed", ast->function_name(),
                           GetPartStr(ast->function_name()));
  }

  Guard<SemanticAnalysis> g(this, &SemanticAnalysis::EnterFunction,
                            &SemanticAnalysis::LeaveFunction);
  {
    Guard<SemanticAnalysis> g(this, &SemanticAnalysis::EnterBlock,
                              &SemanticAnalysis::LeaveBlock);
    ast->params()->Accept(this, nullptr);
    ast->body()->Accept(this, nullptr);
  }
}

void SemanticAnalysis::Visit(ForStatementAST* ast, void* data) {
  Guard<SemanticAnalysis> g(this, &SemanticAnalysis::EnterBlock,
                            &SemanticAnalysis::LeaveBlock);
  if (ast->statement1()) {
    ast->statement1()->Accept(this, nullptr);
  }
  ExprData expr_data;
  expr_data.lex_po_ = LexicalOp_Read;
  ast->statement2()->Accept(this, &expr_data);
  for (ASTreeVector::iterator iter = ast->statement3().begin();
       iter != ast->statement3().end(); ++iter) {
    // TODO check statements
    if (*iter) {
      (*iter)->Accept(this, nullptr);
    }
  }
  if (ast->block()) {
    ast->block()->Accept(this, nullptr);
  }
}

void SemanticAnalysis::Visit(TryCatchFinallyStatementAST* ast, void* data) {
  Guard<SemanticAnalysis> g(this, &SemanticAnalysis::EnterBlock,
                            &SemanticAnalysis::LeaveBlock);
  ast->try_block()->Accept(this, nullptr);
  if (ast->catch_block()) {
    ast->catch_block()->Accept(this, nullptr);
  }
  if (ast->finally_block()) {
    ast->finally_block()->Accept(this, nullptr);
  }
}

void SemanticAnalysis::Visit(DoWhileStatementAST* ast, void* data) {
  ExprData expr_data;
  expr_data.lex_po_ = LexicalOp_Read;
  ast->condition()->Accept(this, &expr_data);
  {
    Guard<SemanticAnalysis> g(this, &SemanticAnalysis::EnterBlock,
                              &SemanticAnalysis::LeaveBlock);
    ast->block()->Accept(this, nullptr);
  }
}

void SemanticAnalysis::Visit(BreakStatementAST* ast, void* data) {}

void SemanticAnalysis::Visit(ContinueStatementAST* ast, void* data) {}

void SemanticAnalysis::Visit(WhileStatementAST* ast, void* data) {
  ExprData expr_data;
  expr_data.lex_po_ = LexicalOp_Read;
  ast->condition()->Accept(this, &expr_data);
  {
    Guard<SemanticAnalysis> g(this, &SemanticAnalysis::EnterBlock,
                              &SemanticAnalysis::LeaveBlock);
    ast->block()->Accept(this, nullptr);
  }
}

void SemanticAnalysis::Visit(IfStatementAST* ast, void* data) {
  ExprData expr_data;
  expr_data.lex_po_ = LexicalOp_Read;
  ast->condition()->Accept(this, &expr_data);
  {
    Guard<SemanticAnalysis> g(this, &SemanticAnalysis::EnterBlock,
                              &SemanticAnalysis::LeaveBlock);
    ast->true_branch()->Accept(this, nullptr);
  }

  if (ast->false_branch()) {
    ast->false_branch()->Accept(this, nullptr);
  }
}

void SemanticAnalysis::Visit(ElseStatementAST* ast, void* data) {
  Guard<SemanticAnalysis> g(this, &SemanticAnalysis::EnterBlock,
                            &SemanticAnalysis::LeaveBlock);
  ast->block()->Accept(this, nullptr);
}

void SemanticAnalysis::Visit(CaseStatementAST* ast, void* data) {
  ast->block()->Accept(this, nullptr);
}

void SemanticAnalysis::Visit(AssignStatement* ast, void* data) {
  if (lynx::tasm::Config::IsHigherOrEqual(sdk_version_,
                                          FEATURE_CONTROL_VERSION_2) == false) {
    int ast_token_ = ast->assignment().token_;
    if (ast_token_ == Token_ASSIGN_BIT_AND ||
        ast_token_ == Token_ASSIGN_BIT_OR ||
        ast_token_ == Token_ASSIGN_BIT_XOR) {
      throw CompileException("The current Sdk Version is less than 1.4,",
                             "something wrong with operator", ast->assignment(),
                             GetPartStr(ast->assignment()));
    }
  }
  ExprData expr_data;
  switch (ast->assignment().token_) {
    case '=':
      expr_data.lex_po_ = LexicalOp_Write;
      break;
    case Token_ASSIGN_ADD:
      expr_data.lex_po_ = LexicalOp_ASSIGN_ADD;
      break;
    case Token_ASSIGN_SUB:
      expr_data.lex_po_ = LexicalOp_ASSIGN_SUB;
      break;
    case Token_ASSIGN_MUL:
      expr_data.lex_po_ = LexicalOp_ASSIGN_MUL;
      break;
    case Token_ASSIGN_DIV:
      expr_data.lex_po_ = LexicalOp_ASSIGN_DIV;
      break;
    case Token_ASSIGN_MOD:
      expr_data.lex_po_ = LexicalOp_ASSIGN_MOD;
      break;
    case Token_ASSIGN_BIT_OR:
      expr_data.lex_po_ = LexicalOp_ASSIGN_BIT_OR;
      break;
    case Token_ASSIGN_BIT_AND:
      expr_data.lex_po_ = LexicalOp_ASSIGN_BIT_AND;
      break;
    case Token_ASSIGN_BIT_XOR:
      expr_data.lex_po_ = LexicalOp_ASSIGN_BIT_XOR;
      break;
    case Token_ASSIGN_Pow:
      expr_data.lex_po_ = LexicalOp_ASSIGN_POW;
      break;
    default:
      break;
  }
  ast->lex_op() = expr_data.lex_po_;
  ast->variable()->Accept(this, &expr_data);
  expr_data.lex_po_ = LexicalOp_Read;
  ast->expression()->Accept(this, &expr_data);
}

void SemanticAnalysis::Visit(MemberAccessorAST* ast, void* data) {
  ExprData expr_data;
  expr_data.lex_po_ = LexicalOp_Read;
  ast->table()->Accept(this, &expr_data);
  ast->member()->Accept(this, &expr_data);
}

void SemanticAnalysis::Visit(FunctionCallAST* ast, void* data) {
  ExprData expr_data;
  expr_data.lex_po_ = LexicalOp_Read;
  ast->caller()->Accept(this, &expr_data);
  {
    // need a new scope for args
    Guard<SemanticAnalysis> g(this, &SemanticAnalysis::EnterBlock,
                              &SemanticAnalysis::LeaveBlock);
    ast->args()->Accept(this, nullptr);
  }
}
void SemanticAnalysis::Visit(TernaryStatementAST* ast, void* data) {
  ExprData expr_data;
  expr_data.lex_po_ = LexicalOp_Read;
  ast->condition()->Accept(this, &expr_data);
  ast->true_branch()->Accept(this, &expr_data);
  ast->false_branch()->Accept(this, &expr_data);
}

void SemanticAnalysis::Visit(ObjectLiteralAST* ast, void* data) {
  base::SortedForEach(ast->property(), [this](const auto& it) {
    ExprData expr_data;
    expr_data.lex_po_ = LexicalOp_Read;
    it.second->Accept(this, &expr_data);
  });
}

void SemanticAnalysis::Visit(ArrayLiteralAST* ast, void* data) {
  if (ast->values()) {
    ast->values()->Accept(this, nullptr);
  }
}
void SemanticAnalysis::SetSdkVersion(const std::string& sdk_version) {
  sdk_version_ = sdk_version;
}
}  // namespace lepus
}  // namespace lynx
