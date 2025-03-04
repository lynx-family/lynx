// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/ast_dump.h"

#include <vector>

#include "core/runtime/vm/lepus/function.h"

namespace lynx {
namespace lepus {

#ifdef LEPUS_TEST
#define GET_INT(data) (*(static_cast<int*>(data)))

#define PRINT(data, char)                                      \
  do {                                                         \
    for (int i = 0; i < GET_INT(data); i++) std::cout << char; \
  } while (0);

#define PRINT_TAB(data) PRINT(data, ' ')

#define PRINT_AST_INFO(data, ast_name, line_col, end_line_col) \
  PRINT(data, '-')                                             \
  std::cout << #ast_name << std::endl;                         \
  PRINT_TAB(data)                                              \
  int32_t line, col;                                           \
  Function::DecodeLineCol(line_col, line, col);                \
  std::cout << "start : " << line << ", " << col << std::endl; \
  PRINT_TAB(data)                                              \
  Function::DecodeLineCol(end_line_col, line, col);            \
  std::cout << "end : " << line << ", " << col << std::endl;

#define PRINT_AST(data, ast_name) \
  PRINT_AST_INFO(data, ast_name, ast->LineCol(), ast->EndLineCol())

#define PRINT_TOKEN(data, token, newLine) \
  PRINT_TAB(data)                         \
  std::cout << #token;                    \
  if (newLine) std::cout << std::endl;

const char* LexicalOpNameArr[] = {
    "LexicalOp_None",           "LexicalOp_Read", "LexicalOp_Write",
    "LexicalOp_ASSIGN_BIT_OR",   // |=
    "LexicalOp_ASSIGN_BIT_XOR",  // ^=
    "LexicalOp_ASSIGN_BIT_AND",  // &=
    "LexicalOp_ASSIGN_SHL",      // <<=
    "LexicalOp_ASSIGN_SAR",      // >>>=
    "LexicalOp_ASSIGN_SHR",      // >>>=
    "LexicalOp_ASSIGN_ADD",      // +=
    "LexicalOp_ASSIGN_SUB",      // -=
    "LexicalOp_ASSIGN_MUL",      // *=
    "LexicalOp_ASSIGN_DIV",      // /=
    "LexicalOp_ASSIGN_MOD",      // %=
    "LexicalOp_ASSIGN_POW"       // **=
};

const char* LexicalScopingNameArr[] = {
    "LexicalScoping_Unknow", "LexicalScoping_Global", "LexicalScoping_Upvalue",
    "LexicalScoping_Local", "LexicalScoping_Upvalue_New"};

const char* AutomaticTypeNameArr[] = {
    "Automatic_None", "Automatic_Inc_Before", "Automatic_Inc_After",
    "Automatic_Dec_Before", "Automatic_Dec_After"};

void ASTDump::Visit(ChunkAST* ast, void* data) {
  int start_tab = 0;
  PRINT_AST(&start_tab, ChunkAST)
  int tab = start_tab + 1;
  ast->block()->Accept(this, &tab);
}

void ASTDump::Visit(BlockAST* ast, void* data) {
  PRINT_AST(data, BlockAST);
  for (ASTreeVector::iterator iter = ast->statements().begin();
       iter != ast->statements().end(); ++iter) {
    int tab = GET_INT(data) + 1;
    (*iter)->Accept(this, &tab);
  }
  if (ast->return_statement()) {
    int tab = GET_INT(data) + 1;
    ast->return_statement()->Accept(this, &tab);
  }
}

void ASTDump::Visit(CatchBlockAST* ast, void* data) {
  PRINT_AST(data, CatchBlockAST)
  if (ast->catch_identifier()) {
    int tab = GET_INT(data) + 1;
    ast->catch_identifier()->Accept(this, &tab);
  }
  int tab = GET_INT(data) + 1;
  ast->block()->Accept(this, &tab);
}

void ASTDump::Visit(ReturnStatementAST* ast, void* data) {
  PRINT_AST(data, ReturnStatementAST)
  if (ast->expression()) {
    int tab = GET_INT(data) + 1;
    ast->expression()->Accept(this, &tab);
  }
}

void ASTDump::Visit(ThrowStatementAST* ast, void* data) {
  PRINT_AST(data, ThrowStatementAST)
  if (ast->throw_identifier()) {
    int tab = GET_INT(data) + 1;
    ast->throw_identifier()->Accept(this, &tab);
  }
}

void ASTDump::Visit(LiteralAST* ast, void* data) {
  PRINT_AST(data, LiteralAST)
  switch (ast->token().token_) {
    case Token_Nil:
      PRINT_TOKEN(data, Token_Nil, true);
      break;
    case Token_Undefined:
      PRINT_TOKEN(data, Token_Undefined, true);
      break;
    case Token_Id:
      PRINT_TOKEN(data, Token_Id, false);
      std::cout << " " << ast->token().str_.c_str() << std::endl;
      break;
    case Token_Number:
      PRINT_TOKEN(data, Token_Number, false);
      std::cout << " " << ast->token().number_ << std::endl;
      break;
    case Token_String:
      PRINT_TOKEN(data, Token_String, false);
      std::cout << " " << ast->token().str_.c_str() << std::endl;
      break;
    case Token_False:
      PRINT_TOKEN(data, Token_False, true);
      break;
    case Token_True:
      PRINT_TOKEN(data, Token_True, true);
      break;
    default:
      break;
  }

  PRINT_TAB(data)
  std::cout << "LexicalScoping : " << LexicalScopingNameArr[ast->scope()]
            << std::endl;

  PRINT_TAB(data)
  std::cout << "LexicalOp : " << LexicalOpNameArr[ast->lex_op()] << std::endl;

  PRINT_TAB(data)
  std::cout << "AutomaticType : " << AutomaticTypeNameArr[ast->auto_type()]
            << std::endl;
}

void ASTDump::Visit(NamesAST* ast, void* data) {
  PRINT_AST(data, NamesAST)

  PRINT_TAB(data)
  std::cout << "function params" << std::endl;
  for (std::vector<Token>::iterator iter = ast->names().begin();
       iter != ast->names().end(); ++iter) {
    std::cout << " " << (*iter).str_.c_str();
  }
  std::cout << std::endl;
}

void ASTDump::Visit(BinaryExprAST* ast, void* data) {
  PRINT_AST(data, BinaryExprAST)
  std::string bin_op;
  switch (ast->op_token().token_) {
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '|':
    case '&':
    case '^':
    case '<':
    case '>':
      bin_op = (char)(ast->op_token().token_);
      break;
    case Token_Pow:
      bin_op = "**";
      break;
    case Token_LessEqual:
      bin_op = "<=";
      break;
    case Token_GreaterEqual:
      bin_op = ">=";
      break;
    case Token_NotEqual:
      bin_op = "!=";
      break;
    case Token_Equal:
      bin_op = "==";
      break;
    case Token_AbsNotEqual:
      bin_op = "!==";
      break;
    case Token_AbsEqual:
      bin_op = "===";
      break;
    case Token_ASSIGN_BIT_OR:
      bin_op = "|=";
      break;
    case Token_ASSIGN_BIT_XOR:
      bin_op = "^=";
      break;
    case Token_ASSIGN_BIT_AND:
      bin_op = "&=";
      break;
    case Token_ASSIGN_SHL:
      bin_op = "<<=";
      break;
    case Token_ASSIGN_SAR:
      bin_op = ">>>=";
      break;
    case Token_ASSIGN_SHR:
      bin_op = ">>=";
      break;
    case Token_ASSIGN_ADD:
      bin_op = "+=";
      break;
    case Token_ASSIGN_SUB:
      bin_op = "+=";
      break;
    case Token_ASSIGN_MUL:
      bin_op = "*=";
      break;
    case Token_ASSIGN_DIV:
      bin_op = "/=";
      break;
    case Token_ASSIGN_MOD:
      bin_op = "%=";
      break;
    case Token_ASSIGN_Pow:
      bin_op = "**=";
      break;
    default:
      break;
  }
  PRINT_TAB(data)
  std::cout << "Binary Operator : " << bin_op << std::endl;
  int leftTab, rightTab;
  leftTab = rightTab = GET_INT(data) + 1;
  ast->left()->Accept(this, &leftTab);
  ast->right()->Accept(this, &rightTab);
}

void ASTDump::Visit(UnaryExpression* ast, void* data) {
  PRINT_AST(data, UnaryExpression)
  switch (ast->op_token().token_) {
    case '-':
    case '!':
    case '~':
    case '+':
      PRINT_TAB(data)
      std::cout << "UnaryOperator : " << (char)(ast->op_token().token_)
                << std::endl;
      break;
    case Token_Typeof:
      PRINT_TAB(data)
      std::cout << "UnaryOperator : typeof" << std::endl;
      break;
  }
  int tab = GET_INT(data) + 1;
  ast->expression()->Accept(this, &tab);
}

void ASTDump::Visit(ExpressionListAST* ast, void* data) {
  PRINT_AST(data, ExpressionListAST)
  for (ASTreeVector::iterator iter = ast->expressions().begin();
       iter != ast->expressions().end(); ++iter) {
    int tab = GET_INT(data) + 1;
    if (*iter) (*iter)->Accept(this, &tab);
  }
}

void ASTDump::Visit(VariableAST* ast, void* data) {
  PRINT_AST(data, VariableAST)
  PRINT_TAB(data)
  std::cout << "variable name : " << ast->identifier().str_.c_str()
            << std::endl;
  if (ast->expression()) {
    int tab = GET_INT(data) + 1;
    ast->expression()->Accept(this, &tab);
  }
}

void ASTDump::Visit(CatchVariableAST* ast, void* data) {
  PRINT_AST(data, CatchVariableAST)
  if (ast->expression()) {
    int tab = GET_INT(data) + 1;
    ast->expression()->Accept(this, &tab);
  }
}

void ASTDump::Visit(VariableListAST* ast, void* data) {
  PRINT_AST(data, VariableListAST)
  for (VariableASTVector::iterator iter = ast->variable_list().begin();
       iter != ast->variable_list().end(); ++iter) {
    int tab = GET_INT(data) + 1;
    (*iter)->Accept(this, &tab);
  }
}

void ASTDump::Visit(FunctionStatementAST* ast, void* data) {
  PRINT_AST(data, FunctionStatementAST)
  if (ast->function_name().token_ != Token_EOF) {
    PRINT_TAB(data)
    std::cout << "function name : " << ast->function_name().str_.c_str()
              << std::endl;
  }
  int params_tab = GET_INT(data) + 1;
  ast->params()->Accept(this, &params_tab);

  int body_tab = GET_INT(data) + 1;
  ast->body()->Accept(this, &body_tab);
}

void ASTDump::Visit(BreakStatementAST* ast, void* data) {
  PRINT_AST(data, BreakStatementAST)
}

void ASTDump::Visit(ContinueStatementAST* ast, void* data) {
  PRINT_AST(data, ContinueStatementAST)
}

void ASTDump::Visit(ForStatementAST* ast, void* data) {
  PRINT_AST(data, ForStatementAST)
  if (ast->statement1()) {
    int tab = GET_INT(data) + 1;
    ast->statement1()->Accept(this, &tab);
  }

  if (ast->statement2()) {
    int tab = GET_INT(data) + 1;
    ast->statement2()->Accept(this, &tab);
  }

  for (ASTreeVector::iterator iter = ast->statement3().begin();
       iter != ast->statement3().end(); ++iter) {
    if (*iter) {
      int tab = GET_INT(data) + 1;
      (*iter)->Accept(this, &tab);
    }
  }

  if (ast->block()) {
    int tab = GET_INT(data) + 1;
    ast->block()->Accept(this, &tab);
  }
}

void ASTDump::Visit(DoWhileStatementAST* ast, void* data) {
  PRINT_AST(data, DoWhileStatementAST)
  if (ast->block()) {
    int tab = GET_INT(data) + 1;
    ast->block()->Accept(this, &tab);
  }

  if (ast->condition()) {
    int tab = GET_INT(data) + 1;
    ast->condition()->Accept(this, &tab);
  }
}

void ASTDump::Visit(TryCatchFinallyStatementAST* ast, void* data) {
  PRINT_AST(data, TryCatchFinallyStatementAST)
  if (ast->try_block()) {
    int tab = GET_INT(data) + 1;
    ast->try_block()->Accept(this, &tab);
  }

  if (ast->catch_block()) {
    int tab = GET_INT(data) + 1;
    ast->catch_block()->Accept(this, &tab);
  }

  if (ast->finally_block()) {
    int tab = GET_INT(data) + 1;
    ast->finally_block()->Accept(this, &tab);
  }
}

void ASTDump::Visit(WhileStatementAST* ast, void* data) {
  PRINT_AST(data, WhileStatementAST)

  if (ast->condition()) {
    int tab = GET_INT(data) + 1;
    ast->condition()->Accept(this, &tab);
  }

  if (ast->block()) {
    int tab = GET_INT(data) + 1;
    ast->block()->Accept(this, &tab);
  }
}

void ASTDump::Visit(IfStatementAST* ast, void* data) {
  PRINT_AST(data, IfStatementAST)

  int cond_tab = GET_INT(data) + 1;
  ast->condition()->Accept(this, &cond_tab);

  int true_br_tab = GET_INT(data) + 1;
  ast->true_branch()->Accept(this, &true_br_tab);

  if (ast->false_branch()) {
    int false_br_tab = GET_INT(data) + 1;
    ast->false_branch()->Accept(this, &false_br_tab);
  }
}

void ASTDump::Visit(TernaryStatementAST* ast, void* data) {
  PRINT_AST(data, TernaryStatementAST)

  int cond_tab = GET_INT(data) + 1;
  ast->condition()->Accept(this, &cond_tab);

  int true_br_tab = GET_INT(data) + 1;
  ast->true_branch()->Accept(this, &true_br_tab);

  int false_br_tab = GET_INT(data) + 1;
  ast->false_branch()->Accept(this, &false_br_tab);
}

void ASTDump::Visit(ElseStatementAST* ast, void* data) {
  PRINT_AST(data, ElseStatementAST)

  int tab = GET_INT(data) + 1;
  ast->block()->Accept(this, &tab);
}

void ASTDump::Visit(CaseStatementAST* ast, void* data) {
  PRINT_AST(data, CaseStatementAST)
  PRINT_TAB(data)
  std::cout << "case token type : ";
  switch (ast->key().token_) {
    case Token_Default:
      std::cout << "default" << std::endl;
      break;
    case Token_Number:
      std::cout << "number : " << ast->key().number_ << std::endl;
      break;
    default:
      std::cout << ast->key().token_ << std::endl;
  }

  int tab = GET_INT(data) + 1;
  ast->block()->Accept(this, &tab);
}

void ASTDump::Visit(AssignStatement* ast, void* data) {
  PRINT_AST(data, AssignStatement)

  PRINT_TAB(data)
  std::cout << "LexicalOp : " << LexicalOpNameArr[ast->lex_op()] << std::endl;

  PRINT_TAB(data)
  std::cout << "AssignmentOpToken : " << ast->assignment().token_ << std::endl;

  int var_tab = GET_INT(data) + 1;
  ast->variable()->Accept(this, &var_tab);

  int expr_tab = GET_INT(data) + 1;
  ast->expression()->Accept(this, &expr_tab);
}

void ASTDump::Visit(MemberAccessorAST* ast, void* data) {
  PRINT_AST(data, MemberAccessorAST)

  PRINT_TAB(data)
  std::cout << "Optional : " << ast->is_optional() << std::endl;

  int table_tab = GET_INT(data) + 1;
  ast->table()->Accept(this, &table_tab);

  int expr_tab = GET_INT(data) + 1;
  ast->member()->Accept(this, &expr_tab);
}

void ASTDump::Visit(FunctionCallAST* ast, void* data) {
  PRINT_AST(data, FunctionCallAST)

  PRINT_TAB(data)
  std::cout << "Optional : " << ast->is_optional() << std::endl;

  int caller_tab = GET_INT(data) + 1;
  ast->caller()->Accept(this, &caller_tab);

  int args_tab = GET_INT(data) + 1;
  ast->args()->Accept(this, &args_tab);
}

void ASTDump::Visit(ObjectLiteralAST* ast, void* data) {
  PRINT_AST(data, ObjectLiteralAST)

  for (auto iter = ast->property().begin(); iter != ast->property().end();
       ++iter) {
    int tab = GET_INT(data) + 1;
    PRINT_TAB(&tab)
    std::cout << "key : " << iter->first.c_str() << std::endl;
    iter->second->Accept(this, &tab);
  }
}

void ASTDump::Visit(ArrayLiteralAST* ast, void* data) {
  PRINT_AST(data, ArrayLiteralAST)

  if (ast->values()) {
    for (auto iter = ast->values()->expressions().begin();
         iter != ast->values()->expressions().end(); ++iter) {
      int tab = GET_INT(data) + 1;
      (*iter)->Accept(this, &tab);
    }
  }
}

#endif
}  // namespace lepus
}  // namespace lynx
