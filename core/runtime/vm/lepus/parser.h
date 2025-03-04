// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_PARSER_H_
#define CORE_RUNTIME_VM_LEPUS_PARSER_H_

#include <string>

#include "core/runtime/vm/lepus/scanner.h"
namespace {  // NOLINT
enum ExprType {
  ExprType_Normal,
  ExprType_Var,
  ExprType_FunctionCall,
  ExprType_OptionalChaining
};

enum OperatorPriority {
  OperatorPriority_Pow = 82,           // **
  OperatorPriority_LogicalNot = 81,    // ~
  OperatorPriority_BitNot = 81,        // !
  OperatorPriority_Mul = 80,           // *
  OperatorPriority_Div = 80,           // /
  OperatorPriority_Rem = 80,           // %
  OperatorPriority_Add = 70,           // +
  OperatorPriority_Sub = 70,           // -
  OperatorPriority_Greater = 51,       // >
  OperatorPriority_Less = 51,          // <
  OperatorPriority_GreaterEqual = 51,  // >=
  OperatorPriority_LessEqual = 51,     // <=
  OperatorPriority_NotEqual = 50,      // !=
  OperatorPriority_Equal = 50,         // ==
  OperatorPriority_AbsNotEqual = 50,   // !==
  OperatorPriority_AbsEqual = 50,      // ===
  OperatorPriority_BitAnd = 43,        // &
  OperatorPriority_BitXor = 42,        // ^
  OperatorPriority_BitOr = 41,         // |
  OperatorPriority_And = 40,           // &&
  OperatorPriority_Or = 30,            // ||
  OperatorPriority_NullCoal,           // ??
  OperatorPriority_QuestionMark = 13,  // ?
};

}  // namespace
namespace lynx {
namespace lepus {
class ASTree;
class Parser {
 public:
  class LoopScope {
   public:
    LoopScope(Parser* parser) : parser_(parser) { parser->AddLoopCount(); }
    ~LoopScope() { parser_->DecreaseLoopCount(); }

   private:
    Parser* parser_;
  };

  class BlockScope {
   public:
    BlockScope(ASTree* block, Parser* parser);
    ~BlockScope();

   private:
    ASTree* block_;
    Parser* parser_;
  };

  Parser(Scanner* scanner) : scanner_(scanner), loop_count_(0) {}
  ASTree* Parse();
  ASTree* ParseChunk();
  ASTree* ParseBlock();
  ASTree* ParseCatchBlock();
  ASTree* ParseBlockSingleLine();
  ASTree* ParseReturnStatement();

  ASTree* ParseStatement();
  ASTree* ParseBreakStatement();
  ASTree* ParseContinueStatement();
  ASTree* ParseForStatement();
  ASTree* ParseDoWhileStatement();
  ASTree* ParseWhileStatement();
  ASTree* ParseTryCatchFinallyStatement();
  ASTree* ParseIfStatement();
  ASTree* ParseElseStatement();
  ASTree* ParseElseIfStatement();
  ASTree* ParseCaseStatement();
  ASTree* ParseBlockStatement();
  ASTree* ParseThrowStatement();
  ASTree* ParseOtherStatement();

  ASTree* ParseFunctionStatement();
  ASTree* ParseVarStatement();
  ASTree* ParseCatchIdentifier();
  ASTree* ParseExpressionList(const int& token);
  ASTree* ParseExpression(ASTree* left = nullptr, int left_priority = 0,
                          Token token = Token());
  ASTree* ParseTableLiteral();
  ASTree* ParseArrayLiteral();
  ASTree* ParsePrimaryExpr();
  ASTree* ParsePrefixExpr(ExprType* type = nullptr);
  ASTree* ParsePrefixExprEnd(ASTree* expr, ExprType* type);
  ASTree* ParseVar(ASTree* table, bool is_optional = false);
  ASTree* ParseNames();
  ASTree* ParseFunctionCall(ASTree* caller, bool is_optional = false);
  ASTree* ParseTernaryOperation(ASTree* condition);
  ASTree* ParseOptionalChaining(ASTree* ast, ExprType* type = nullptr);
  ASTree* ParseImportStatement();
  ASTree* ParseExportStatement();

  int GetIsInLoop() { return loop_count_; }

  void AddLoopCount() { loop_count_++; }
  void DecreaseLoopCount() { loop_count_--; }

  // this shift should be equal to Function::kLineBitsShift
  int64_t GetLineCol() {
    if (lynx::tasm::Config::IsHigherOrEqual(sdk_version_, LYNX_VERSION_2_1)) {
      // new version: 30 bits for line number and 34 bits for column number
      return static_cast<uint64_t>(scanner_->line()) << 30 |
             static_cast<uint64_t>(scanner_->column());
    } else {
      // old version: 16 bits for line number and 16 bits for column number
      return static_cast<uint64_t>(scanner_->line()) << 16 |
             static_cast<uint64_t>(scanner_->column());
    }
  }

  void SetSdkVersion(const std::string& sdk_version);

 private:
  Token& NextToken() {
    if (next_token_.token_ != Token_EOF) {
      current_token_ = next_token_;
      next_token_ = next_next_token_;
      if (next_next_token_.token_ != Token_EOF) {
        next_next_token_.token_ = Token_EOF;
      }
    } else {
      Token current_tmp = current_token_;
      scanner_->NextToken(current_token_, current_tmp);
    }
    return current_token_;
  }

  Token& LookAhead() {
    if (next_token_.token_ == Token_EOF) {
      scanner_->NextToken(next_token_, current_token_);
    }
    return next_token_;
  }

  Token& LookAhead2() {
    LookAhead();
    if (next_next_token_.token_ == Token_EOF) {
      scanner_->NextToken(next_next_token_, next_token_);
    }
    return next_next_token_;
  }

  int Priority(int token) {
    switch (token) {
      case Token_Pow:
        return OperatorPriority_Pow;
      case '~':
        return OperatorPriority_LogicalNot;
      case '!':
        return OperatorPriority_BitNot;
      case '*':
        return OperatorPriority_Mul;
      case '/':
        return OperatorPriority_Div;
      case '%':
        return OperatorPriority_Rem;
      case '+':
        return OperatorPriority_Add;
      case '-':
        return OperatorPriority_Sub;
      case '>':
        return OperatorPriority_Greater;
      case '<':
        return OperatorPriority_Less;
      case Token_GreaterEqual:
        return OperatorPriority_GreaterEqual;
      case Token_LessEqual:
        return OperatorPriority_LessEqual;
      case Token_NotEqual:
        return OperatorPriority_NotEqual;
      case Token_Equal:
        return OperatorPriority_Equal;
      case Token_AbsNotEqual:
        return OperatorPriority_AbsNotEqual;
      case Token_AbsEqual:
        return OperatorPriority_AbsEqual;
      case '&':
        return OperatorPriority_BitAnd;
      case '^':
        return OperatorPriority_BitXor;
      case '|':
        return OperatorPriority_BitOr;
      case Token_And:
        return OperatorPriority_And;
      case Token_Or:
        return OperatorPriority_Or;
      case Token_Nullish_Coalescing:
        return OperatorPriority_NullCoal;
      case '?':
        return OperatorPriority_QuestionMark;
      default:
        return 0;
    }
  }
  std::string GetPartStr(Token& token) { return scanner_->GetPartStr(token); }

  Token current_token_;
  Token next_token_;
  Token next_next_token_;
  Scanner* scanner_;
  int loop_count_;

  std::string sdk_version_;
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_PARSER_H_
