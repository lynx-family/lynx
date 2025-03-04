// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/parser.h"

#include <memory>
#include <unordered_map>
#include <utility>

#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/lepus_utils.h"
#include "core/runtime/vm/lepus/syntax_tree.h"
#include "core/runtime/vm/lepus/token.h"

namespace lynx {
namespace lepus {

ASTree* Parser::Parse() { return ParseChunk(); }

ASTree* Parser::ParseChunk() {
  int64_t start_line = GetLineCol();
  ASTree* block = ParseBlock();
  if (NextToken().token_ != Token_EOF) {
    throw CompileException("Expect <EOF>", current_token_,
                           GetPartStr(current_token_));
  }
  ASTree* tree = new ChunkAST(block);
  tree->SetLineCol(start_line);
  tree->SetEndLineCol(GetLineCol());
  return tree;
}

ASTree* Parser::ParseBlock() {
  BlockAST* block = new BlockAST;
  BlockScope block_scope(block, this);
  while (LookAhead().token_ != Token_EOF && LookAhead().token_ != Token_Else &&
         LookAhead().token_ != Token_Elseif &&
         LookAhead().token_ != Token_Case &&
         LookAhead().token_ != Token_Default && LookAhead().token_ != '}') {
    ASTree* statement = ParseStatement();
    if (statement)
      block->statements().push_back(std::unique_ptr<ASTree>(statement));
  }
  return block;
}

ASTree* Parser::ParseCatchBlock() {
  CatchBlockAST* catchBlock = new CatchBlockAST;
  BlockScope block_scope(catchBlock, this);
  bool has_binding = false;
  if (LookAhead().token_ == '(') {
    has_binding = true;
  }

  if (has_binding) {
    NextToken();  // skip '('
    ASTree* catch_identifier = ParseCatchIdentifier();
    catchBlock->catch_identifier().reset(catch_identifier);
    if (LookAhead().token_ != ')') {
      throw CompileException("expect ')' in catch statement", current_token_,
                             GetPartStr(current_token_));
    }
    NextToken();  // skip ')'
  }

  if (LookAhead().token_ == '{') {
    NextToken();  // skip '{'
    catchBlock->block().reset(ParseBlock());
    if (LookAhead().token_ != '}') {
      throw CompileException("expect '}' in catch body", current_token_,
                             GetPartStr(current_token_));
    }
    NextToken();  // skip '}'
  } else {
    throw CompileException("except '{' after catch(e)", current_token_,
                           GetPartStr(current_token_));
  }
  return catchBlock;
}

ASTree* Parser::ParseBlockSingleLine() {
  BlockAST* block = new BlockAST;
  ASTree* statement = ParseStatement();
  if (statement)
    block->statements().push_back(std::unique_ptr<ASTree>(statement));
  return block;
}

ASTree* Parser::ParseReturnStatement() {
  NextToken();
  ReturnStatementAST* return_statement = new ReturnStatementAST;
  return_statement->expression().reset(ParseExpression());
  return return_statement;
}

ASTree* Parser::ParseBreakStatement() {
  if (!GetIsInLoop()) {
    throw CompileException("expect break in the loop scope", LookAhead(),
                           GetPartStr(LookAhead()));
  }
  ASTree* ret = new BreakStatementAST(NextToken());
  ret->SetLineCol(GetLineCol());
  return ret;
}

ASTree* Parser::ParseContinueStatement() {
  if (!GetIsInLoop()) {
    throw CompileException("expect continue in the loop scope", LookAhead(),
                           GetPartStr(LookAhead()));
  }
  ASTree* ret = new ContinueStatementAST(NextToken());
  ret->SetLineCol(GetLineCol());
  return ret;
}

ASTree* Parser::ParseForStatement() {
  LoopScope loopScope(this);
  NextToken();  // skip for
  if (LookAhead().token_ != '(') {
    throw CompileException("expect '('", current_token_,
                           GetPartStr(current_token_));
  }
  ForStatementAST* ast = new ForStatementAST;
  BlockScope bsp(ast, this);
  NextToken();
  ast->statement1().reset(ParseStatement());
  if (ast->statement1()) {
    if (NextToken().token_ != ';') {
      throw CompileException("expect ';'", current_token_,
                             GetPartStr(current_token_));
    }
  }
  ast->statement2().reset(ParseExpression());
  if (ast->statement2()) {
    if (NextToken().token_ != ';') {
      throw CompileException("expect ';'", current_token_,
                             GetPartStr(current_token_));
    }
  }
  do {
    ast->statement3().push_back(std::unique_ptr<ASTree>(ParseStatement()));
  } while (NextToken().token_ == ',');
  if (current_token_.token_ != ')') {
    throw CompileException("expect ')'", current_token_,
                           GetPartStr(current_token_));
  }
  int next_token = LookAhead().token_;
  if (next_token == ';') {
    NextToken();
    ast->block().reset(nullptr);
    return ast;
  } else if (next_token != '{') {
    ast->block().reset(ParseBlockSingleLine());
    return ast;
  }
  NextToken();
  ast->block().reset(ParseBlock());
  if (NextToken().token_ != '}') {
    throw CompileException("expect '}'", current_token_,
                           GetPartStr(current_token_));
  }
  return ast;
}

ASTree* Parser::ParseDoWhileStatement() {
  LoopScope loop_scope(this);
  NextToken();

  ASTree* block = nullptr;
  int64_t start_line = GetLineCol();
  if (NextToken().token_ != '{') {
    block = ParseBlockSingleLine();
  } else {
    block = ParseBlock();

    if (NextToken().token_ != '}') {
      throw CompileException("expect '}'", current_token_,
                             GetPartStr(current_token_));
    }
  }

  if (NextToken().token_ != Token_While) {
    throw CompileException("expect 'while'", current_token_,
                           GetPartStr(current_token_));
  }

  ASTree* condition = ParseExpression();

  if (NextToken().token_ != ';') {
    throw CompileException("expect ';'", current_token_,
                           GetPartStr(current_token_));
  }
  ASTree* ret = new DoWhileStatementAST(condition, block);
  ret->SetLineCol(start_line);
  ret->SetEndLineCol(GetLineCol());
  return ret;
}

ASTree* Parser::ParseWhileStatement() {
  LoopScope loop_scope(this);
  NextToken();
  int64_t start_line = GetLineCol();
  ASTree* condition = ParseExpression();
  ASTree* ret = nullptr;
  if (NextToken().token_ != '{') {
    // signal line block
    ret = new WhileStatementAST(condition, ParseBlockSingleLine());
    ret->SetLineCol(start_line);
    ret->SetEndLineCol(GetLineCol());
    return ret;
  }

  ASTree* block = ParseBlock();

  if (NextToken().token_ != '}') {
    throw CompileException("expect '}'", current_token_,
                           GetPartStr(current_token_));
  }
  ret = new WhileStatementAST(condition, block);
  ret->SetEndLineCol(GetLineCol());
  ret->SetLineCol(start_line);
  return ret;
}

ASTree* Parser::ParseTryCatchFinallyStatement() {
  NextToken();  // skip 'try'
  if (LookAhead().token_ != '{') {
    throw CompileException("expect '{'", current_token_,
                           GetPartStr(current_token_));
  }
  NextToken();  // skip '{'
  TryCatchFinallyStatementAST* ast = new TryCatchFinallyStatementAST();
  BlockScope bsp(ast, this);
  ASTree* try_block = ParseBlock();
  if (LookAhead().token_ != '}') {
    throw CompileException("expect '{'", current_token_,
                           GetPartStr(current_token_));
  }
  NextToken();  // skip '}'
  ast->try_block().reset(try_block);

  int next_token = LookAhead().token_;
  if (next_token != Token_Finally && next_token != Token_Catch) {
    throw CompileException("expect 'catch' or 'finally' in try statement",
                           current_token_, GetPartStr(current_token_));
  }

  if (next_token == Token_Catch) {
    NextToken();  // skip 'catch'
    ast->catch_block().reset(ParseCatchBlock());
  }

  if (LookAhead().token_ == Token_Finally) {
    NextToken();            // skip 'finally'
    ASTree* finally_block;  // = ParseBlock();
    if (LookAhead().token_ == '{') {
      NextToken();  // skip '{'
      finally_block = ParseBlock();
      ast->finally_block().reset(finally_block);
      if (LookAhead().token_ != '}') {
        throw CompileException("expect '}' ", current_token_,
                               GetPartStr(current_token_));
      }
      NextToken();  // skip '}'
    } else {
      throw CompileException("except '{' after 'finally' ", current_token_,
                             GetPartStr(current_token_));
    }
  }
  return ast;
}

ASTree* Parser::ParseIfStatement() {
  NextToken();  // skip 'if'

  int64_t start_line = GetLineCol();
  ASTree* condition = ParseExpression();
  ASTree* true_branch = nullptr;
  if (LookAhead().token_ == '{') {
    NextToken();  // skip '{'
    true_branch = ParseBlock();

    if (NextToken().token_ != '}') {  // skip '}'
      throw CompileException("expect '}'", current_token_,
                             GetPartStr(current_token_));
    }
  } else {
    true_branch = ParseBlockSingleLine();
  }

  ASTree* false_branch = nullptr;
  if (LookAhead().token_ == Token_Else)
    false_branch = ParseElseStatement();
  else if (LookAhead().token_ == Token_Elseif) {
    false_branch = ParseElseIfStatement();
  }

  ASTree* ret = new IfStatementAST(condition, true_branch, false_branch);
  ret->SetLineCol(start_line);
  ret->SetEndLineCol(GetLineCol());
  return ret;
}

ASTree* Parser::ParseElseStatement() {
  NextToken();  // skip 'else'

  ASTree* block = nullptr;
  if (LookAhead().token_ == '{') {
    NextToken();  // skip '{'
    block = ParseBlock();

    if (NextToken().token_ != '}') {  // skip '}'
      throw CompileException("expect '}'", current_token_,
                             GetPartStr(current_token_));
    }
  } else {
    block = ParseBlockSingleLine();
  }
  ASTree* ret = new ElseStatementAST(block);
  ret->SetLineCol(GetLineCol());
  return ret;
}

ASTree* Parser::ParseElseIfStatement() {
  NextToken();  // skip 'elseif'

  ASTree* condition = ParseExpression();
  ASTree* true_branch = nullptr;
  if (LookAhead().token_ == '{') {
    NextToken();  // skip '{'
    true_branch = ParseBlock();

    if (NextToken().token_ != '}') {  // skip '}'
      throw CompileException("expect '}'", current_token_,
                             GetPartStr(current_token_));
    }
  } else {
    true_branch = ParseBlockSingleLine();
  }

  ASTree* false_branch = nullptr;
  if (LookAhead().token_ == Token_Else)
    false_branch = ParseElseStatement();
  else if (LookAhead().token_ == Token_Elseif) {
    false_branch = ParseElseIfStatement();
  }
  ASTree* ret = new IfStatementAST(condition, true_branch, false_branch);
  ret->SetLineCol(GetLineCol());
  return ret;
}

ASTree* Parser::ParseFunctionStatement() {
  NextToken();
  FunctionStatementAST* function = new FunctionStatementAST();
  function->SetLineCol(GetLineCol());
  if (LookAhead().token_ == Token_Id) {
    NextToken();
    function->set_function_name(current_token_);
  }

  if (LookAhead().token_ != '(') {
    throw CompileException("expect '(' ", LookAhead(), GetPartStr(LookAhead()));
  }

  NamesAST* names = new NamesAST;
  names->SetLineCol(GetLineCol());
  while (NextToken().token_ != ')') {
    if (current_token_.token_ == Token_Id) {
      names->names().push_back(current_token_);
    }
  }
  function->params().reset(names);

  if (LookAhead().token_ != '{') {
    throw CompileException("expect '{' ", current_token_,
                           GetPartStr(current_token_));
  }
  NextToken();
  function->body().reset(ParseBlock());

  if (LookAhead().token_ != '}') {
    throw CompileException("expect '}' ", current_token_,
                           GetPartStr(current_token_));
  }
  NextToken();
  function->SetEndLineCol(GetLineCol());
  return function;
}

ASTree* Parser::ParseCaseStatement() {
  LoopScope loopScope(this);
  NextToken();
  if (current_token_.token_ != Token_Case &&
      current_token_.token_ != Token_Default) {
    throw CompileException("expect case/default ", current_token_,
                           GetPartStr(current_token_));
  }
  bool is_default = current_token_.token_ == Token_Default ? true : false;
  Token key = is_default ? current_token_ : NextToken();
  if (LookAhead().token_ != ':') {
    throw CompileException("expect ':' ", current_token_,
                           GetPartStr(current_token_));
  }
  NextToken();

  return new CaseStatementAST(is_default, key, ParseBlock());
}

ASTree* Parser::ParseThrowStatement() {
  NextToken();  // skip 'throw'
  if (LookAhead().token_ != '(') {
    throw CompileException("expect '(' ", current_token_,
                           GetPartStr(current_token_));
  }
  NextToken();  // skip '('
  ASTree* throwIdentifier = new LiteralAST(LookAhead());
  ThrowStatementAST* throwStatement = new ThrowStatementAST();
  throwStatement->throw_identifier().reset(throwIdentifier);
  NextToken();
  if (LookAhead().token_ != ')') {
    throw CompileException("expect ')' ", current_token_,
                           GetPartStr(current_token_));
  }
  NextToken();  // ')'
  if (LookAhead().token_ != ';') {
    throw CompileException("expect ';' ", current_token_,
                           GetPartStr(current_token_));
  }
  NextToken();  // ';'
  return throwStatement;
}

ASTree* Parser::ParseBlockStatement() {
  NextToken();  // skip '{'
  ASTree* block = ParseBlock();
  if (NextToken().token_ != '}') {  // skip '}'
    throw CompileException("expect '}'", current_token_,
                           GetPartStr(current_token_));
  }
  return block;
}

// support:
/*
 * import xx from 'xxx';
 * import xx from 'xxx'
 */
ASTree* Parser::ParseImportStatement() {
  while (LookAhead().token_ != Token_EOF) {
    if (LookAhead().token_ == Token_From) {
      NextToken();       // skip 'from'
      ParseStatement();  // parse 'xxxx'
      break;
    }
    NextToken();
  }
  if (LookAhead().token_ == ';') {
    NextToken();  // ';'
  }
  return NULL;
}

// support:
/*
 * export {xxx}
 * export {xxx};
 * export default let a = 1;
 * export default function test() {}
 */
ASTree* Parser::ParseExportStatement() {
  NextToken();  // skip 'export'
  ASTree* tree = NULL;
  while (LookAhead().token_ != ';') {
    if (LookAhead().token_ == Token_Function) {
      tree = ParseStatement();
      return tree;
    } else if (LookAhead().token_ == Token_Var) {
      tree = ParseStatement();
      if (LookAhead().token_ != ';') {
        throw CompileException("expect ';' in export statement", current_token_,
                               GetPartStr(current_token_));
      }
      return tree;
    } else if (LookAhead().token_ == '{') {
      while (LookAhead().token_ != '}') {
        if (LookAhead().token_ == Token_EOF) {
          throw CompileException("expect '}' in export statement",
                                 current_token_, GetPartStr(current_token_));
        }
        NextToken();
      }
      NextToken();  // skip '}'
      return tree;
    } else {
      if (LookAhead().token_ == Token_EOF) {
        throw CompileException("expect ';' in export statement", current_token_,
                               GetPartStr(current_token_));
      }
      NextToken();
    }
  }
  return tree;
}

ASTree* Parser::ParseCatchIdentifier() {
  if (LookAhead().token_ != Token_Id) {
    throw CompileException("invalid assign", LookAhead(),
                           GetPartStr(LookAhead()));
  }
  CatchVariableAST* var = new CatchVariableAST;
  var->SetLineCol(GetLineCol());
  NextToken();
  if (LookAhead().token_ != ')') {
    throw CompileException("expect token ')'", LookAhead(),
                           GetPartStr(LookAhead()));
  }
  var->identifier() = current_token_;
  var->expression().reset(ParseExpression());
  return var;
}

ASTree* Parser::ParseVarStatement() {
  VariableListAST* var_list = new VariableListAST;
  do {
    NextToken();
    if (LookAhead().token_ != Token_Id) {
      throw CompileException("invalid assign", LookAhead(),
                             GetPartStr(LookAhead()));
    }
    VariableAST* var = new VariableAST;
    var->SetLineCol(GetLineCol());
    NextToken();
    var->identifier() = current_token_;
    if (LookAhead().token_ == '=') {
      NextToken();
      var->expression().reset(ParseExpression());
    }
    var_list->variable_list().push_back(std::unique_ptr<VariableAST>(var));
  } while (LookAhead().token_ == ',');
  return var_list;
}

ASTree* Parser::ParseOtherStatement() { return ParseExpression(); }

ASTree* Parser::ParseStatement() {
  int64_t start_line = GetLineCol();
  ASTree* ret = nullptr;
  switch (LookAhead().token_) {
    case ';':
      NextToken();
      break;
    case Token_Break:
      ret = ParseBreakStatement();
      break;
    case Token_Continue:
      ret = ParseContinueStatement();
      break;
    case Token_For:
      ret = ParseForStatement();
      break;
    case Token_Do:
      ret = ParseDoWhileStatement();
      break;
    case Token_While:
      ret = ParseWhileStatement();
      break;
    case Token_Try:
      ret = ParseTryCatchFinallyStatement();
      break;
    case Token_If:
      ret = ParseIfStatement();
      break;
    case Token_Function:
      ret = ParseFunctionStatement();
      break;
    case Token_Var:
      ret = ParseVarStatement();
      break;
    case Token_Return:
      ret = ParseReturnStatement();
      break;
    case Token_Switch:
      throw CompileException("Switch Keyword Is Not Supported", LookAhead(),
                             GetPartStr(LookAhead()));
      break;
    case Token_Throw:
      ret = ParseThrowStatement();
      break;
    case '{': {
      ret = ParseBlockStatement();
      break;
    }
    case Token_Import: {
      ret = ParseImportStatement();
      break;
    }
    case Token_Export: {
      ret = ParseExportStatement();
      break;
    }
    default:
      ret = ParseOtherStatement();
      break;
  }
  if (ret) {
    ret->SetEndLineCol(GetLineCol());
    ret->SetLineCol(start_line);
  }

  return ret;
}

ASTree* Parser::ParseExpressionList(const int& token) {
  std::unique_ptr<ExpressionListAST> ast =
      std::make_unique<ExpressionListAST>();
  do {
    NextToken();
    if (LookAhead().token_ == token) {
      return ast.release();
    }
    ast->expressions().push_back(std::unique_ptr<ASTree>(ParseExpression()));
  } while (LookAhead().token_ == ',');
  return ast.release();
}

ASTree* Parser::ParseExpression(ASTree* left, int left_priority, Token token) {
  bool need_delete_left = true;
  auto left_conditional_deleter = [&need_delete_left](ASTree* left) {
    if (need_delete_left) {
      delete left;
    }
  };
  std::unique_ptr<ASTree, decltype(left_conditional_deleter)> left_unique_ptr{
      left, left_conditional_deleter};
  ASTree* expression = nullptr;
  if (LookAhead().token_ == '-' || LookAhead().token_ == '!' ||
      LookAhead().token_ == '~' || LookAhead().token_ == '+' ||
      LookAhead().token_ == Token_Typeof) {
    NextToken();
    expression = new UnaryExpression;
    expression->SetLineCol(GetLineCol());
    static_cast<UnaryExpression*>(expression)->op_token() = current_token_;
    static_cast<UnaryExpression*>(expression)
        ->expression()
        .reset(ParseExpression(new ASTree, 90, Token()));
  } else if (IsPrimaryExpr(LookAhead().token_)) {
    expression = ParsePrimaryExpr();
  } else if (LookAhead().token_ == ')' || LookAhead().token_ == ';') {
    return expression;
  } else {
    throw CompileException("error expression", LookAhead(),
                           GetPartStr(LookAhead()));
  }

  while (true) {
    int right_priority = Priority(LookAhead().token_);
    if (left_priority < right_priority) {
      if (LookAhead().token_ == '?') {
        expression = ParseTernaryOperation(expression);
      } else {
        expression = ParseExpression(expression, right_priority, NextToken());
      }
    } else if (left_priority == 0) {
      return expression;
    } else if (left_priority == right_priority) {
      need_delete_left = false;
      expression = new BinaryExprAST(left, expression, token);
      expression->SetLineCol(GetLineCol());
      return ParseExpression(expression, right_priority, NextToken());
    } else if ((left_priority == OperatorPriority_And ||
                left_priority == OperatorPriority_Or) &&
               right_priority == OperatorPriority_NullCoal) {
      throw CompileException("?? can't be used in combination with && or ||",
                             LookAhead(), GetPartStr(LookAhead()));
    } else {
      if (token.token_ != Token_EOF) {
        need_delete_left = false;
        expression = new BinaryExprAST(left, expression, token);
        expression->SetLineCol(GetLineCol());
      }
      return expression;
    }
  }

  return expression;
}

ASTree* Parser::ParsePrimaryExpr() {
  ASTree* expr = nullptr;
  switch (LookAhead().token_) {
    case Token_Nil:
    case Token_Undefined: {
      expr = new LiteralAST(NextToken());
      expr->SetLineCol(GetLineCol());
      break;
    }
    case Token_Function:
      expr = ParseFunctionStatement();
      break;
    case Token_Id:
    case Token_False:
    case Token_True:
    case Token_Number:
    case Token_String:
    case '(':
    case Token_INC:
    case Token_DEC:
    case Token_RegExp:
    case '[':
      expr = ParsePrefixExpr();
      break;
    case '{':
      expr = ParseTableLiteral();
      break;
    default:
      break;
  }
  return expr;
}

ASTree* Parser::ParseTableLiteral() {
  NextToken();
  std::unordered_map<base::String, std::unique_ptr<ASTree>> property;
  if (!Token::IsObjectKey(LookAhead().token_) && LookAhead().token_ != '}') {
    throw CompileException("Map only support string for property key.",
                           LookAhead(), GetPartStr(LookAhead()));
  }
  while (Token::IsObjectKey(LookAhead().token_)) {
    base::String key;
    if (LookAhead().token_ == Token_Number) {
      std::ostringstream s;
      s << LookAhead().number_;
      key = s.str();
    } else {
      key = LookAhead().str_;
    }
    NextToken();
    if (LookAhead().token_ != ':') {
      throw CompileException("expect token : ", LookAhead(),
                             GetPartStr(LookAhead()));
    } else {
      NextToken();
    }
    if (property.find(key) != property.end()) {
      throw CompileException("duplicate key in map", LookAhead(),
                             GetPartStr(LookAhead()));
    }
    property[key] = std::unique_ptr<ASTree>(ParseExpression());
    if (LookAhead().token_ == ',') {
      NextToken();
    } else {
      break;
    }
  }
  if (LookAhead().token_ != '}') {
    throw CompileException("expect token } ", LookAhead(),
                           GetPartStr(LookAhead()));
  }
  NextToken();  // Skip }
  ASTree* ret = new ObjectLiteralAST(std::move(property));
  ret->SetLineCol(GetLineCol());
  return ret;
}

ASTree* Parser::ParseArrayLiteral() {
  ExpressionListAST* ast =
      static_cast<ExpressionListAST*>(ParseExpressionList(']'));
  if (LookAhead().token_ != ']') {
    throw CompileException("expect ]", LookAhead(), GetPartStr(LookAhead()));
  }
  NextToken();  // Skip ]
  ASTree* ret = new ArrayLiteralAST(ast);
  ret->SetLineCol(GetLineCol());
  return ret;
}

ASTree* Parser::ParsePrefixExpr(ExprType* type) {
  ASTree* expr = nullptr;
  AutomaticType auto_type = Automatic_None;
  if (LookAhead().token_ == '[') {
    expr = ParseArrayLiteral();
  } else if (LookAhead().token_ == '(') {
    NextToken();
    expr = ParseExpression();
    if (type) *type = ExprType_Normal;
    if (LookAhead().token_ != ')')
      throw CompileException("expect )", LookAhead(), GetPartStr(LookAhead()));
    NextToken();
  } else if (LookAhead().token_ == Token_INC ||
             LookAhead().token_ == Token_DEC) {
    auto_type = NextToken().token_ == Token_INC ? Automatic_Inc_Before
                                                : Automatic_Dec_Before;
    expr = ParseExpression();
    if (type) *type = ExprType_Var;
  } else {
    std::string id = LookAhead().str_.str();
    if (id == "String" || id == "Array" || id == "Number" || id == "Object") {
      if (LookAhead2().token_ == '(') {
        throw CompileException(id.c_str(), " Is Not Supported,", LookAhead(),
                               GetPartStr(LookAhead()));
      }
    }
    expr = new LiteralAST(LookAhead());
    expr->SetLineCol(GetLineCol());
    if (type) *type = ExprType_Var;
    NextToken();
  }
  expr = ParsePrefixExprEnd(expr, type);

  if (auto_type != Automatic_None) {
    if (expr->type() == ASTType_Literal) {
      static_cast<LiteralAST*>(expr)->auto_type() = auto_type;
    } else if (expr->type() == ASTType_MemberAccessor) {
    }
  }

  return expr;
}

ASTree* Parser::ParsePrefixExprEnd(ASTree* expr, ExprType* type) {
  if (LookAhead().token_ == Token_Optional_Chaining) {
    expr = ParseOptionalChaining(expr, type);
    return ParsePrefixExprEnd(expr, type);
  } else if ((LookAhead().token_ == '[') || (LookAhead().token_ == '.')) {
    expr = ParseVar(expr, false);
    if (type) *type = ExprType_Var;
    return ParsePrefixExprEnd(expr, type);
  } else if (LookAhead().token_ == '(') {
    if (type) *type = ExprType_FunctionCall;
    expr = ParseFunctionCall(expr);
    NextToken();
    return ParsePrefixExprEnd(expr, type);
  } else if (LookAhead().token_ == Token_INC ||
             LookAhead().token_ == Token_DEC) {
    if (expr->type() == ASTType_Literal) {
      static_cast<LiteralAST*>(expr)->auto_type() =
          NextToken().token_ == Token_INC ? Automatic_Inc_After
                                          : Automatic_Dec_After;
    } else if (expr->type() == ASTType_MemberAccessor) {
    }
    return expr;
  } else if (LookAhead().token_ == '=' ||
             LookAhead().token_ == Token_ASSIGN_ADD ||
             LookAhead().token_ == Token_ASSIGN_SUB ||
             LookAhead().token_ == Token_ASSIGN_MUL ||
             LookAhead().token_ == Token_ASSIGN_DIV ||
             LookAhead().token_ == Token_ASSIGN_MOD ||
             LookAhead().token_ == Token_ASSIGN_BIT_OR ||
             LookAhead().token_ == Token_ASSIGN_BIT_AND ||
             LookAhead().token_ == Token_ASSIGN_BIT_XOR ||
             LookAhead().token_ == Token_ASSIGN_Pow) {
    Token assignment = NextToken();
    auto ast = ParseExpression();
    if (expr->type() == ASTType_MemberAccessor &&
        static_cast<MemberAccessorAST*>(expr)->is_optional()) {
      throw CompileException("Optional Chaining Assignment Is Not Supported,",
                             LookAhead(), GetPartStr(LookAhead()));
    }
    ASTree* ret = new AssignStatement(assignment, expr, ast);
    ret->SetLineCol(GetLineCol());
    return ret;
  } else {
    return expr;
  }
}

ASTree* Parser::ParseTernaryOperation(ASTree* condition) {
  NextToken();
  ASTree* exprTrue = ParseExpression();
  if (LookAhead().token_ != ':') {
    throw CompileException("except : ", LookAhead(), GetPartStr(LookAhead()));
  }
  NextToken();
  ASTree* exprFalse = ParseExpression();
  ASTree* ret = new TernaryStatementAST(condition, exprTrue, exprFalse);
  ret->SetLineCol(GetLineCol());
  return ret;
}

ASTree* Parser::ParseOptionalChaining(ASTree* ast, ExprType* type) {
  if (LookAhead2().token_ == '[') {
    NextToken();  // skip '?.'
    ast = ParseVar(ast, true);
    if (type) *type = ExprType_Var;
  } else if (LookAhead2().token_ == '(') {
    NextToken();
    ast = ParseFunctionCall(ast, true);
    if (type) *type = ExprType_FunctionCall;
    NextToken();  // skip ')'
  } else {
    ast = ParseVar(ast, true);
    if (type) *type = ExprType_Var;
  }
  return ast;
}

ASTree* Parser::ParseVar(ASTree* table, bool is_optional) {
  NextToken();  // skip '.' '?.' '['
  if (current_token_.token_ == '.' ||
      current_token_.token_ == Token_Optional_Chaining) {
    if (!(LookAhead().token_ == Token_Id ||
          (Token::IsObjectKey(LookAhead().token_) &&
           LookAhead().token_ != Token_String &&
           LookAhead().token_ != Token_Number))) {
      throw CompileException("expect <id>", LookAhead(),
                             GetPartStr(LookAhead()));
    }
    NextToken();
    // In table member accessor, member token should be string instead of id
    Token temp_token = current_token_;
    temp_token.token_ = Token_String;
    auto member = new LiteralAST(temp_token);
    member->SetLineCol(GetLineCol());
    ASTree* node = new MemberAccessorAST(table, member, is_optional);
    node->SetLineCol(GetLineCol());
    return node;
  } else if (current_token_.token_ == '[') {
    if (LookAhead().token_ == Token_Number || LookAhead().token_ == Token_Id ||
        LookAhead().token_ == Token_String || LookAhead().token_ == '(') {
      auto member = ParseExpression();
      if (LookAhead().token_ != ']') {
        throw CompileException("expect ]", LookAhead(),
                               GetPartStr(LookAhead()));
      }
      NextToken();  // skip ']'
      ASTree* node = new MemberAccessorAST(table, member, is_optional);
      node->SetLineCol(GetLineCol());
      return node;
    }
    throw CompileException("expect <id>", LookAhead(), GetPartStr(LookAhead()));
  }
  return nullptr;
}

ASTree* Parser::ParseNames() {
  if (NextToken().token_ != Token_Id) {
    throw CompileException("error name", current_token_,
                           GetPartStr(current_token_));
  }
  NamesAST* names = new NamesAST;
  names->names().push_back(current_token_);
  names->SetLineCol(GetLineCol());
  while (LookAhead().token_ == ',') {
    NextToken();
    if (NextToken().token_ != Token_Id) {
      throw CompileException("error name", current_token_,
                             GetPartStr(current_token_));
    }
    names->names().push_back(current_token_);
  }
  return names;
}

ASTree* Parser::ParseFunctionCall(ASTree* caller, bool is_optional) {
  int64_t start_line = GetLineCol();
  ASTree* args = ParseExpressionList(')');
  if (LookAhead().token_ != ')') {
    throw CompileException("do not support anonymous array for function call",
                           current_token_, GetPartStr(current_token_));
  }
  ASTree* node = new FunctionCallAST(caller, args, is_optional);
  node->SetLineCol(start_line);
  node->SetEndLineCol(GetLineCol());
  return node;
}

Parser::BlockScope::BlockScope(ASTree* block, Parser* parser)
    : block_(block), parser_(parser) {
  block_->SetLineCol(parser->GetLineCol());
}

Parser::BlockScope::~BlockScope() {
  block_->SetEndLineCol(parser_->GetLineCol());
}

void Parser::SetSdkVersion(const std::string& sdk_version) {
  sdk_version_ = sdk_version;
}

}  // namespace lepus
}  // namespace lynx
