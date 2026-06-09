// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/supports/supports_evaluator.h"

#include <string>

#include "core/renderer/css/css_property.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace css {

namespace {

bool IsCustomPropertyName(const std::string& name) {
  return tasm::CSSProperty::IsCustomProperty(
      name.c_str(), static_cast<uint32_t>(name.length()));
}

}  // namespace

bool SupportsEvaluator::Eval(const SupportsConditionNode& node) const {
  switch (node.GetType()) {
    case SupportsConditionNode::Type::kEngineVersion:
      return EvalEngineVersion(
          static_cast<const SupportsEngineVersionNode&>(node));

    case SupportsConditionNode::Type::kDeclaration:
      return EvalDeclaration(static_cast<const SupportsDeclNode&>(node));

    case SupportsConditionNode::Type::kNested: {
      const auto& unary = static_cast<const SupportsUnaryNode&>(node);
      return Eval(unary.Operand().get());
    }

    case SupportsConditionNode::Type::kNot: {
      const auto& unary = static_cast<const SupportsUnaryNode&>(node);
      return !Eval(unary.Operand().get());
    }

    case SupportsConditionNode::Type::kAnd: {
      const auto& binary = static_cast<const SupportsBinaryNode&>(node);
      return Eval(binary.Left().get()) && Eval(binary.Right().get());
    }

    case SupportsConditionNode::Type::kOr: {
      const auto& binary = static_cast<const SupportsBinaryNode&>(node);
      return Eval(binary.Left().get()) || Eval(binary.Right().get());
    }

    case SupportsConditionNode::Type::kFunction:
    case SupportsConditionNode::Type::kGeneralEnclosed:
      return false;
  }
  return false;
}

bool SupportsEvaluator::EvalDeclaration(const SupportsDeclNode& node) const {
  if (IsCustomPropertyName(node.Property())) {
    return true;
  }
  const tasm::CSSPropertyID property_id =
      tasm::CSSProperty::GetPropertyID(node.Property());
  if (!tasm::CSSProperty::IsPropertyValid(property_id)) {
    return false;
  }

  tasm::StyleMap output;
  return tasm::UnitHandler::Process(property_id, lepus::Value(node.Value()),
                                    output, configs_);
}

bool SupportsEvaluator::EvalEngineVersion(
    const SupportsEngineVersionNode& node) const {
  return node.Begin() <= engine_version_ && engine_version_ < node.End();
}

}  // namespace css
}  // namespace lynx
