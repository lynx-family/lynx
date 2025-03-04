// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/syntax_tree.h"

#include "core/runtime/vm/lepus/visitor.h"

namespace lynx {
namespace lepus {

void ChunkAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void BlockAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void BlockAST::Accept(Visitor* visitor, void* data, bool gen) {
  visitor->Visit(this, data, gen);
}

void CatchBlockAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void ReturnStatementAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void LiteralAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void ThrowStatementAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void NamesAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void BinaryExprAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void UnaryExpression::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void ExpressionListAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void VariableAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void CatchVariableAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void VariableListAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void FunctionStatementAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void DoWhileStatementAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void ForStatementAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void TryCatchFinallyStatementAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void BreakStatementAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void ContinueStatementAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void WhileStatementAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void IfStatementAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void ElseStatementAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void CaseStatementAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void AssignStatement::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void MemberAccessorAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void FunctionCallAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void TernaryStatementAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void ObjectLiteralAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}

void ArrayLiteralAST::Accept(Visitor* visitor, void* data) {
  visitor->Visit(this, data);
}
}  // namespace lepus
}  // namespace lynx
