// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_VISITOR_H_
#define CORE_RUNTIME_VM_LEPUS_VISITOR_H_

#include "core/runtime/vm/lepus/syntax_tree.h"

namespace lynx {
namespace lepus {
class Visitor {
 public:
  virtual ~Visitor() {}
  virtual void Visit(ChunkAST* ast, void* data) = 0;
  virtual void Visit(BlockAST* ast, void* data) = 0;
  virtual void Visit(BlockAST* ast, void* data, bool) = 0;
  virtual void Visit(CatchBlockAST* ast, void* data) = 0;
  virtual void Visit(ReturnStatementAST* ast, void* data) = 0;
  virtual void Visit(LiteralAST* ast, void* data) = 0;
  virtual void Visit(ThrowStatementAST* ast, void* data) = 0;
  virtual void Visit(NamesAST* ast, void* data) = 0;
  virtual void Visit(BinaryExprAST* ast, void* data) = 0;
  virtual void Visit(UnaryExpression* ast, void* data) = 0;
  virtual void Visit(ExpressionListAST* ast, void* data) = 0;
  virtual void Visit(VariableAST* ast, void* data) = 0;
  virtual void Visit(CatchVariableAST* ast, void* data) = 0;
  virtual void Visit(VariableListAST* ast, void* data) = 0;
  virtual void Visit(FunctionStatementAST* ast, void* data) = 0;
  virtual void Visit(ForStatementAST* ast, void* data) = 0;
  virtual void Visit(TryCatchFinallyStatementAST* ast, void* data) = 0;
  virtual void Visit(DoWhileStatementAST* ast, void* data) = 0;
  virtual void Visit(BreakStatementAST* ast, void* data) = 0;
  virtual void Visit(ContinueStatementAST* ast, void* data) = 0;
  virtual void Visit(WhileStatementAST* ast, void* data) = 0;
  virtual void Visit(IfStatementAST* ast, void* data) = 0;
  virtual void Visit(ElseStatementAST* ast, void* data) = 0;
  virtual void Visit(CaseStatementAST* ast, void* data) = 0;
  virtual void Visit(AssignStatement* ast, void* data) = 0;
  virtual void Visit(MemberAccessorAST* ast, void* data) = 0;
  virtual void Visit(FunctionCallAST* ast, void* data) = 0;
  virtual void Visit(TernaryStatementAST* ast, void* data) = 0;
  virtual void Visit(ObjectLiteralAST* ast, void* data) = 0;
  virtual void Visit(ArrayLiteralAST* ast, void* data) = 0;
};
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_VM_LEPUS_VISITOR_H_
