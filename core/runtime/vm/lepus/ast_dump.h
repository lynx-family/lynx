// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_AST_DUMP_H_
#define CORE_RUNTIME_VM_LEPUS_AST_DUMP_H_

#include <string>
#include <unordered_set>

#include "core/runtime/vm/lepus/syntax_tree.h"
#include "core/runtime/vm/lepus/visitor.h"

namespace lynx {
namespace lepus {
class ASTDump : public Visitor {
 public:
  explicit ASTDump() = default;
  virtual ~ASTDump() {}
  virtual void Visit(ChunkAST* ast, void* data);
  virtual void Visit(BlockAST* ast, void* data);
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
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_AST_DUMP_H_
