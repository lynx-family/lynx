// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_SYNTAX_TREE_H_
#define CORE_RUNTIME_VM_LEPUS_SYNTAX_TREE_H_

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/runtime/vm/lepus/token.h"

namespace lynx {
namespace lepus {

class Visitor;

enum LexicalOp {
  LexicalOp_None,
  LexicalOp_Read,
  LexicalOp_Write,
  LexicalOp_ASSIGN_BIT_OR,   // |=
  LexicalOp_ASSIGN_BIT_XOR,  // ^=
  LexicalOp_ASSIGN_BIT_AND,  // &=
  LexicalOp_ASSIGN_SHL,      // <<=
  LexicalOp_ASSIGN_SAR,      // >>>=
  LexicalOp_ASSIGN_SHR,      // >>>=
  LexicalOp_ASSIGN_ADD,      // +=
  LexicalOp_ASSIGN_SUB,      // -=
  LexicalOp_ASSIGN_MUL,      // *=
  LexicalOp_ASSIGN_DIV,      // /=
  LexicalOp_ASSIGN_MOD,      // %
  LexicalOp_ASSIGN_POW
};

enum LexicalScoping {
  LexicalScoping_Unknow,
  LexicalScoping_Global,
  LexicalScoping_Upvalue,
  LexicalScoping_Local,
  LexicalScoping_Upvalue_New,
};

enum ASTType {
  ASTType_Unknow,
  ASTType_Chunk,
  ASTType_Block,
  ASTType_Return,
  ASTType_Literal,
  ASTType_Names,
  ASTType_BinaryExpr,
  ASTType_UnaryExpr,
  ASTType_ExpressionList,
  ASTType_MemberAccessor,
};

enum AutomaticType {
  Automatic_None,
  Automatic_Inc_Before,
  Automatic_Inc_After,
  Automatic_Dec_Before,
  Automatic_Dec_After,
};

#define AST_ACCEPT_VISITOR virtual void Accept(Visitor* visitor, void* data);

class ASTree {
 public:
  virtual ~ASTree() {}
  virtual ASTType type() { return ASTType_Unknow; }
  virtual void Accept(Visitor* visitor, void* data) {}
  virtual void Accept(Visitor* visitor, void* data, bool) {}
  void SetLineCol(int64_t line_col) { line_col_ = line_col; }
  int64_t LineCol() { return line_col_; }

  void SetEndLineCol(int64_t line_col) { end_line_col_ = line_col; }
  int64_t EndLineCol() { return end_line_col_; }

 private:
  int64_t line_col_ = -1;
  int64_t end_line_col_ = -1;
};

using ASTreeVector = std::vector<std::unique_ptr<ASTree>>;

class ChunkAST : public ASTree {
 public:
  ChunkAST(ASTree* block) : block_(block) {}

  std::unique_ptr<ASTree>& block() { return block_; }
  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> block_;
  ;
};

class BlockAST : public ASTree {
 public:
  ASTreeVector& statements() { return statements_; }
  std::unique_ptr<ASTree>& return_statement() { return return_statement_; }

  AST_ACCEPT_VISITOR
  void Accept(Visitor* visitor, void* data, bool);

 private:
  ASTreeVector statements_;
  std::unique_ptr<ASTree> return_statement_;
};

class CatchBlockAST : public ASTree {
 public:
  std::unique_ptr<ASTree>& block() { return block_; }
  std::unique_ptr<ASTree>& catch_identifier() { return catch_identifier_; }

  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> catch_identifier_;
  std::unique_ptr<ASTree> block_;
};

class ReturnStatementAST : public ASTree {
 public:
  std::unique_ptr<ASTree>& expression() { return expression_; }
  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> expression_;
};

class ThrowStatementAST : public ASTree {
 public:
  std::unique_ptr<ASTree>& throw_identifier() { return throw_identifier_; }
  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> throw_identifier_;
};

class LiteralAST : public ASTree {
 public:
  LiteralAST(const Token& token)
      : token_(token),
        scope_(LexicalScoping_Unknow),
        lex_op_(LexicalOp_None),
        auto_type_(Automatic_None) {}

  Token& token() { return token_; }
  LexicalScoping& scope() { return scope_; }
  LexicalOp& lex_op() { return lex_op_; }
  AutomaticType& auto_type() { return auto_type_; }
  virtual ASTType type() { return ASTType_Literal; }

  AST_ACCEPT_VISITOR
 private:
  Token token_;
  LexicalScoping scope_;
  LexicalOp lex_op_;
  AutomaticType auto_type_;
};

class NamesAST : public ASTree {
 public:
  std::vector<Token>& names() { return names_; }
  AST_ACCEPT_VISITOR
 private:
  std::vector<Token> names_;
};

class BinaryExprAST : public ASTree {
 public:
  BinaryExprAST() {}
  BinaryExprAST(ASTree* left, ASTree* right, const Token& op)
      : left_(left), right_(right), op_token_(op) {}

  std::unique_ptr<ASTree>& left() { return left_; }

  std::unique_ptr<ASTree>& right() { return right_; }

  Token& op_token() { return op_token_; }
  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> left_;
  std::unique_ptr<ASTree> right_;
  Token op_token_;
};

class UnaryExpression : public ASTree {
 public:
  UnaryExpression() {}
  UnaryExpression(ASTree* expression, const Token& op)
      : expression_(expression), op_token_(op) {}

  std::unique_ptr<ASTree>& expression() { return expression_; }

  Token& op_token() { return op_token_; }
  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> expression_;
  Token op_token_;
};

class ExpressionListAST : public ASTree {
 public:
  ASTreeVector& expressions() { return expressions_; }

  AST_ACCEPT_VISITOR
 private:
  ASTreeVector expressions_;
};

class VariableAST : public ASTree {
 public:
  VariableAST() {}
  VariableAST(const Token& identifier, ASTree* expression)
      : identifier_(identifier), expression_(expression) {}

  Token& identifier() { return identifier_; }
  std::unique_ptr<ASTree>& expression() { return expression_; }
  AST_ACCEPT_VISITOR
 private:
  Token identifier_;
  std::unique_ptr<ASTree> expression_;
};

class CatchVariableAST : public ASTree {
 public:
  CatchVariableAST() {}
  CatchVariableAST(const Token& identifier, ASTree* expression)
      : identifier_(identifier), expression_(expression) {}

  Token& identifier() { return identifier_; }
  std::unique_ptr<ASTree>& expression() { return expression_; }
  AST_ACCEPT_VISITOR
 private:
  Token identifier_;
  std::unique_ptr<ASTree> expression_;
};

using VariableASTVector = std::vector<std::unique_ptr<VariableAST>>;

class VariableListAST : public ASTree {
 public:
  VariableListAST() {}
  VariableASTVector& variable_list() { return variable_list_; }
  AST_ACCEPT_VISITOR
 private:
  VariableASTVector variable_list_;
};

class FunctionStatementAST : public ASTree {
 public:
  FunctionStatementAST() {}
  FunctionStatementAST(const Token& name) : function_name_(name) {}

  inline void set_function_name(const Token& name) { function_name_ = name; }

  std::unique_ptr<ASTree>& params() { return params_; }

  std::unique_ptr<ASTree>& body() { return body_; }

  Token& function_name() { return function_name_; }
  AST_ACCEPT_VISITOR
 private:
  Token function_name_;
  std::unique_ptr<ASTree> params_;
  std::unique_ptr<ASTree> body_;
};

class BreakStatementAST : public ASTree {
 public:
  explicit BreakStatementAST(const Token& token) : break_(token) {}
  AST_ACCEPT_VISITOR
 private:
  Token break_;
};

class ContinueStatementAST : public ASTree {
 public:
  explicit ContinueStatementAST(const Token& token) : continue_(token) {}
  AST_ACCEPT_VISITOR
 private:
  Token continue_;
};

class ForStatementAST : public ASTree {
 public:
  std::unique_ptr<ASTree>& statement1() { return statement1_; }

  std::unique_ptr<ASTree>& statement2() { return statement2_; }

  ASTreeVector& statement3() { return statement3_; }

  std::unique_ptr<ASTree>& block() { return block_; }
  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> statement1_;
  std::unique_ptr<ASTree> statement2_;
  ASTreeVector statement3_;
  std::unique_ptr<ASTree> block_;
};

class DoWhileStatementAST : public ASTree {
 public:
  explicit DoWhileStatementAST(ASTree* condition, ASTree* block)
      : condition_(condition), block_(block) {}

  const std::unique_ptr<ASTree>& condition() { return condition_; }

  const std::unique_ptr<ASTree>& block() { return block_; }

  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> condition_;
  std::unique_ptr<ASTree> block_;
};

class TryCatchFinallyStatementAST : public ASTree {
 public:
  std::unique_ptr<ASTree>& try_block() { return try_block_; }
  std::unique_ptr<ASTree>& catch_block() { return catch_block_; }
  std::unique_ptr<ASTree>& finally_block() { return finally_block_; }
  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> try_block_;
  std::unique_ptr<ASTree> catch_block_;
  std::unique_ptr<ASTree> finally_block_;
};

class WhileStatementAST : public ASTree {
 public:
  explicit WhileStatementAST(ASTree* condition, ASTree* block)
      : condition_(condition), block_(block) {}

  const std::unique_ptr<ASTree>& condition() { return condition_; }

  const std::unique_ptr<ASTree>& block() { return block_; }

  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> condition_;
  std::unique_ptr<ASTree> block_;
};

class IfStatementAST : public ASTree {
 public:
  explicit IfStatementAST(ASTree* condition, ASTree* true_branch,
                          ASTree* false_branch)
      : condition_(condition),
        true_branch_(true_branch),
        false_branch_(false_branch) {}

  const std::unique_ptr<ASTree>& condition() { return condition_; }

  const std::unique_ptr<ASTree>& true_branch() { return true_branch_; }

  const std::unique_ptr<ASTree>& false_branch() { return false_branch_; }

  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> condition_;
  std::unique_ptr<ASTree> true_branch_;
  std::unique_ptr<ASTree> false_branch_;
};

class TernaryStatementAST : public ASTree {
 public:
  explicit TernaryStatementAST(ASTree* condition, ASTree* true_branch,
                               ASTree* false_branch)
      : condition_(condition),
        true_branch_(true_branch),
        false_branch_(false_branch) {}

  const std::unique_ptr<ASTree>& condition() { return condition_; }

  const std::unique_ptr<ASTree>& true_branch() { return true_branch_; }

  const std::unique_ptr<ASTree>& false_branch() { return false_branch_; }

  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> condition_;
  std::unique_ptr<ASTree> true_branch_;
  std::unique_ptr<ASTree> false_branch_;
};

class ElseStatementAST : public ASTree {
 public:
  explicit ElseStatementAST(ASTree* block) : block_(block) {}

  const std::unique_ptr<ASTree>& block() { return block_; }

  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> block_;
};

class CaseStatementAST : public ASTree {
 public:
  explicit CaseStatementAST(bool is_default, Token& key, ASTree* block)
      : is_default_(is_default), key_(key), block_(block) {}

  bool is_default() { return is_default_; }

  Token& key() { return key_; }

  const std::unique_ptr<ASTree>& block() { return block_; }

  AST_ACCEPT_VISITOR
 private:
  bool is_default_;
  Token key_;
  std::unique_ptr<ASTree> block_;
};

class AssignStatement : public ASTree {
 public:
  AssignStatement(const Token& assignment, ASTree* variable, ASTree* expression)
      : assignment_(assignment),
        variable_(variable),
        expression_(expression),
        lex_op_(LexicalOp_None) {}

  Token& assignment() { return assignment_; }

  std::unique_ptr<ASTree>& variable() { return variable_; }

  std::unique_ptr<ASTree>& expression() { return expression_; }

  LexicalOp& lex_op() { return lex_op_; }

  AST_ACCEPT_VISITOR
 private:
  Token assignment_;
  std::unique_ptr<ASTree> variable_;
  std::unique_ptr<ASTree> expression_;
  LexicalOp lex_op_;
};

class MemberAccessorAST : public ASTree {
 public:
  MemberAccessorAST(ASTree* table, ASTree* member, bool is_optional = false)
      : table_(table), member_(member), is_optional_(is_optional) {}

  std::unique_ptr<ASTree>& table() { return table_; }

  std::unique_ptr<ASTree>& member() { return member_; }

  bool is_optional() { return is_optional_; }

  void set_is_optional(bool is_optional) { is_optional_ = is_optional; }

  virtual ASTType type() { return ASTType_MemberAccessor; }

  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> table_;
  std::unique_ptr<ASTree> member_;
  bool is_optional_;
};

class FunctionCallAST : public ASTree {
 public:
  FunctionCallAST(ASTree* caller, ASTree* args, bool is_optional = false)
      : caller_(caller), args_(args), is_optional_(is_optional) {}

  bool is_optional() { return is_optional_; }

  void set_is_optional(bool is_optional) { is_optional_ = is_optional; }

  std::unique_ptr<ASTree>& caller() { return caller_; }

  std::unique_ptr<ASTree>& args() { return args_; }

  AST_ACCEPT_VISITOR
 private:
  std::unique_ptr<ASTree> caller_;
  std::unique_ptr<ASTree> args_;
  bool is_optional_;
};

class ObjectLiteralAST : public ASTree {
 public:
  ObjectLiteralAST(
      std::unordered_map<base::String, std::unique_ptr<ASTree>> property)
      : property_(std::move(property)) {}
  ~ObjectLiteralAST() {}
  AST_ACCEPT_VISITOR

  const std::unordered_map<base::String, std::unique_ptr<ASTree>>& property() {
    return property_;
  }

 private:
  // Now only support string as key
  std::unordered_map<base::String, std::unique_ptr<ASTree>> property_;
};

class ArrayLiteralAST : public ASTree {
 public:
  ArrayLiteralAST(ExpressionListAST* values) : values_(values) {}
  ~ArrayLiteralAST() {}
  AST_ACCEPT_VISITOR

  inline ExpressionListAST* values() { return values_.get(); }

 private:
  std::unique_ptr<ExpressionListAST> values_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_SYNTAX_TREE_H_
