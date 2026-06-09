// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/supports/supports_condition.h"

#include <sstream>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"

namespace lynx {
namespace css {

std::string SupportsDeclNode::Serialize() const {
  std::ostringstream os;
  os << "(" << property_ << ": " << value_;
  if (important_) {
    os << " !important";
  }
  os << ")";
  return os.str();
}

std::string SupportsEngineVersionNode::Serialize() const {
  auto version_str = [](uint32_t v) -> std::string {
    if (v == kMaxVersion) return "*";
    return std::to_string(Major(v)) + "." + std::to_string(Minor(v));
  };
  return "-x-engine-version(" + version_str(begin_) + ", " + version_str(end_) +
         ")";
}

std::string SupportsFunctionNode::Serialize() const {
  return name_ + "(" + raw_args_ + ")";
}

std::string SupportsGeneralEnclosedNode::Serialize() const { return raw_; }

std::string SupportsUnaryNode::Serialize() const {
  std::string child = operand_ ? operand_->Serialize() : std::string();
  if (GetType() == Type::kNot) {
    return "not " + child;
  }
  // kNested
  return "(" + child + ")";
}

std::string SupportsBinaryNode::Serialize() const {
  std::string l = left_ ? left_->Serialize() : std::string();
  std::string r = right_ ? right_->Serialize() : std::string();
  const char* op = (GetType() == Type::kAnd) ? " and " : " or ";
  return l + op + r;
}

// ToLepus / FromLepus: each node serializes as [ type(u8), ...payload ].

namespace {

fml::RefPtr<const SupportsConditionNode> NodeFromLepusOrNull(
    const lepus_value& value) {
  if (!value.IsArray()) return nullptr;
  return SupportsConditionNode::FromLepus(value);
}

lepus_value NodeToLepusOrFalse(const SupportsConditionNode* node) {
  if (!node) return lepus_value(false);
  return node->ToLepus();
}

}  // namespace

lepus_value SupportsDeclNode::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(Type::kDeclaration));
  arr->emplace_back(property_);
  arr->emplace_back(value_);
  arr->emplace_back(important_);
  return lepus_value(std::move(arr));
}

lepus_value SupportsEngineVersionNode::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(Type::kEngineVersion));
  arr->emplace_back(begin_);
  arr->emplace_back(end_);
  return lepus_value(std::move(arr));
}

lepus_value SupportsFunctionNode::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(Type::kFunction));
  arr->emplace_back(name_);
  arr->emplace_back(raw_args_);
  return lepus_value(std::move(arr));
}

lepus_value SupportsGeneralEnclosedNode::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(Type::kGeneralEnclosed));
  arr->emplace_back(raw_);
  return lepus_value(std::move(arr));
}

lepus_value SupportsUnaryNode::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(GetType()));
  arr->emplace_back(NodeToLepusOrFalse(operand_.get()));
  return lepus_value(std::move(arr));
}

lepus_value SupportsBinaryNode::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(GetType()));
  arr->emplace_back(NodeToLepusOrFalse(left_.get()));
  arr->emplace_back(NodeToLepusOrFalse(right_.get()));
  return lepus_value(std::move(arr));
}

// static
fml::RefPtr<const SupportsConditionNode> SupportsConditionNode::FromLepus(
    const lepus_value& value) {
  if (!value.IsArray()) return nullptr;
  const auto& arr = value.Array();
  if (arr->size() < 2) return nullptr;

  const uint32_t raw_type = arr->get(0).UInt32();
  switch (static_cast<Type>(raw_type)) {
    case Type::kDeclaration: {
      if (arr->size() < 4) return nullptr;
      std::string property = arr->get(1).StdString();
      std::string val = arr->get(2).StdString();
      bool important = arr->get(3).Bool();
      return fml::MakeRefCounted<SupportsDeclNode>(std::move(property),
                                                   std::move(val), important);
    }

    case Type::kEngineVersion: {
      if (arr->size() < 3) return nullptr;
      uint32_t begin = arr->get(1).UInt32();
      uint32_t end = arr->get(2).UInt32();
      return fml::MakeRefCounted<SupportsEngineVersionNode>(begin, end);
    }

    case Type::kFunction: {
      if (arr->size() < 3) return nullptr;
      std::string name = arr->get(1).StdString();
      std::string raw_args = arr->get(2).StdString();
      return fml::MakeRefCounted<SupportsFunctionNode>(std::move(name),
                                                       std::move(raw_args));
    }

    case Type::kGeneralEnclosed: {
      if (arr->size() < 2) return nullptr;
      std::string raw = arr->get(1).StdString();
      return fml::MakeRefCounted<SupportsGeneralEnclosedNode>(std::move(raw));
    }

    case Type::kNested: {
      if (arr->size() < 2) return nullptr;
      auto operand = NodeFromLepusOrNull(arr->get(1));
      if (!operand) return nullptr;
      return fml::MakeRefCounted<SupportsUnaryNode>(Type::kNested,
                                                    std::move(operand));
    }

    case Type::kNot: {
      if (arr->size() < 2) return nullptr;
      auto operand = NodeFromLepusOrNull(arr->get(1));
      if (!operand) return nullptr;
      return fml::MakeRefCounted<SupportsUnaryNode>(Type::kNot,
                                                    std::move(operand));
    }

    case Type::kAnd: {
      if (arr->size() < 3) return nullptr;
      auto left = NodeFromLepusOrNull(arr->get(1));
      auto right = NodeFromLepusOrNull(arr->get(2));
      return fml::MakeRefCounted<SupportsBinaryNode>(
          Type::kAnd, std::move(left), std::move(right));
    }

    case Type::kOr: {
      if (arr->size() < 3) return nullptr;
      auto left = NodeFromLepusOrNull(arr->get(1));
      auto right = NodeFromLepusOrNull(arr->get(2));
      return fml::MakeRefCounted<SupportsBinaryNode>(Type::kOr, std::move(left),
                                                     std::move(right));
    }
  }
  return nullptr;
}

}  // namespace css
}  // namespace lynx
