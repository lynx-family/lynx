// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/media_query/media_query_exp.h"

#include <sstream>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"

namespace lynx {
namespace css {

namespace {

MediaFeatureOperator FlipRelOp(MediaFeatureOperator op) {
  switch (op) {
    case MediaFeatureOperator::kLt:
      return MediaFeatureOperator::kGt;
    case MediaFeatureOperator::kLe:
      return MediaFeatureOperator::kGe;
    case MediaFeatureOperator::kGt:
      return MediaFeatureOperator::kLt;
    case MediaFeatureOperator::kGe:
      return MediaFeatureOperator::kLe;
    case MediaFeatureOperator::kEq:
    case MediaFeatureOperator::kNone:
      return op;
  }
  return op;
}

const char* OpToString(MediaFeatureOperator op) {
  switch (op) {
    case MediaFeatureOperator::kEq:
      return "=";
    case MediaFeatureOperator::kLt:
      return "<";
    case MediaFeatureOperator::kLe:
      return "<=";
    case MediaFeatureOperator::kGt:
      return ">";
    case MediaFeatureOperator::kGe:
      return ">=";
    case MediaFeatureOperator::kNone:
      return "";
  }
  return "";
}

void AppendValue(std::ostringstream& os, const MediaFeatureValue& value) {
  switch (value.Type()) {
    case MediaFeatureType::kNumber:
      os << value.Numeric();
      return;
    case MediaFeatureType::kRatio:
      os << value.Numeric() << "/" << value.Denominator();
      return;
    case MediaFeatureType::kIdent:
      os << value.Text();
      return;
    case MediaFeatureType::kBoolean:
    case MediaFeatureType::kInvalid:
      // Boolean form has no printable right-hand side; invalid values keep
      // the raw token text for diagnostics.
      os << value.Text();
      return;
    case MediaFeatureType::kDimension: {
      os << value.Numeric();
      switch (value.Unit()) {
        case MediaFeatureUnit::kPixels:
          os << "px";
          break;
        case MediaFeatureUnit::kRem:
          os << "rem";
          break;
        case MediaFeatureUnit::kEm:
          os << "em";
          break;
        case MediaFeatureUnit::kPercent:
          os << "%";
          break;
        case MediaFeatureUnit::kViewportWidth:
          os << "vw";
          break;
        case MediaFeatureUnit::kViewportHeight:
          os << "vh";
          break;
        case MediaFeatureUnit::kViewportMin:
          os << "vmin";
          break;
        case MediaFeatureUnit::kViewportMax:
          os << "vmax";
          break;
        case MediaFeatureUnit::kDppx:
          os << "dppx";
          break;
        case MediaFeatureUnit::kDpi:
          os << "dpi";
          break;
        case MediaFeatureUnit::kDpcm:
          os << "dpcm";
          break;
        case MediaFeatureUnit::kUnknown:
          break;
      }
      return;
    }
  }
}

void AppendFeature(std::ostringstream& os, const MediaFeature& feature) {
  os << "(";
  if (feature.IsBoolean()) {
    os << feature.Name();
  } else if (feature.HasRightBound()) {
    // `A op1 name op2 B` — left_op is stored anchored on the feature name,
    // so we flip it back to get the original `value < name` form.
    AppendValue(os, feature.LeftValue());
    os << " " << OpToString(FlipRelOp(feature.LeftOperator())) << " "
       << feature.Name() << " " << OpToString(feature.RightOperator()) << " ";
    AppendValue(os, feature.RightValue());
  } else if (feature.LeftOperator() == MediaFeatureOperator::kNone) {
    // Plain form `(name: value)` -- there was a value but no explicit op.
    os << feature.Name() << ": ";
    AppendValue(os, feature.LeftValue());
  } else {
    os << feature.Name() << " " << OpToString(feature.LeftOperator()) << " ";
    AppendValue(os, feature.LeftValue());
  }
  os << ")";
}

}  // namespace

std::string MediaQueryFeatureExpNode::Serialize() const {
  std::ostringstream os;
  AppendFeature(os, feature_);
  return os.str();
}

std::string MediaQueryNestedExpNode::Serialize() const {
  std::string inner = inner_ ? inner_->Serialize() : std::string();
  return "(" + inner + ")";
}

std::string MediaQueryNotExpNode::Serialize() const {
  std::string operand = operand_ ? operand_->Serialize() : std::string();
  return "not " + operand;
}

std::string MediaQueryAndExpNode::Serialize() const {
  std::string left = Left() ? Left()->Serialize() : std::string();
  std::string right = Right() ? Right()->Serialize() : std::string();
  return left + " and " + right;
}

std::string MediaQueryOrExpNode::Serialize() const {
  std::string left = Left() ? Left()->Serialize() : std::string();
  std::string right = Right() ? Right()->Serialize() : std::string();
  return left + " or " + right;
}

// ---- ToLepus / FromLepus ---------------------------------------------------
//
// Each subclass ToLepus returns a two-element array `[ type(u8), payload ]`:
//   kFeature -> payload = MediaFeature::ToLepus()
//   kNested  -> payload = inner ExpNode ToLepus (or false if null)
//   kNot     -> payload = operand ExpNode ToLepus (or false if null)
//   kAnd/kOr -> payload = [ left ExpNode ToLepus, right ExpNode ToLepus ]
//
// The polymorphic MediaQueryExpNode::FromLepus factory inspects slot 0 and
// rebuilds the matching concrete subclass. Invalid shapes collapse to null
// so callers can surface the resulting MediaQuerySet as "not all".

namespace {

fml::RefPtr<const MediaQueryExpNode> ExpNodeFromLepusOrNull(
    const lepus_value& value) {
  if (!value.IsArray()) return nullptr;
  return MediaQueryExpNode::FromLepus(value);
}

lepus_value ExpNodeToLepusOrFalse(const MediaQueryExpNode* node) {
  if (!node) return lepus_value(false);
  return node->ToLepus();
}

}  // namespace

lepus_value MediaQueryFeatureExpNode::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(Type::kFeature));
  arr->emplace_back(feature_.ToLepus());
  return lepus_value(std::move(arr));
}

lepus_value MediaQueryNestedExpNode::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(Type::kNested));
  arr->emplace_back(ExpNodeToLepusOrFalse(inner_.get()));
  return lepus_value(std::move(arr));
}

lepus_value MediaQueryNotExpNode::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(Type::kNot));
  arr->emplace_back(ExpNodeToLepusOrFalse(operand_.get()));
  return lepus_value(std::move(arr));
}

lepus_value MediaQueryAndExpNode::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(Type::kAnd));
  auto children = lepus::CArray::Create();
  children->emplace_back(ExpNodeToLepusOrFalse(Left().get()));
  children->emplace_back(ExpNodeToLepusOrFalse(Right().get()));
  arr->emplace_back(std::move(children));
  return lepus_value(std::move(arr));
}

lepus_value MediaQueryOrExpNode::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(Type::kOr));
  auto children = lepus::CArray::Create();
  children->emplace_back(ExpNodeToLepusOrFalse(Left().get()));
  children->emplace_back(ExpNodeToLepusOrFalse(Right().get()));
  arr->emplace_back(std::move(children));
  return lepus_value(std::move(arr));
}

// static
fml::RefPtr<const MediaQueryExpNode> MediaQueryExpNode::FromLepus(
    const lepus_value& value) {
  if (!value.IsArray()) return nullptr;
  const auto& arr = value.Array();
  if (arr->size() < 2) return nullptr;
  const uint32_t raw_type = arr->get(0).UInt32();
  const lepus_value& payload = arr->get(1);
  switch (static_cast<Type>(raw_type)) {
    case Type::kFeature: {
      return fml::MakeRefCounted<MediaQueryFeatureExpNode>(
          MediaFeature::FromLepus(payload));
    }
    case Type::kNested: {
      auto inner = ExpNodeFromLepusOrNull(payload);
      return fml::MakeRefCounted<MediaQueryNestedExpNode>(std::move(inner));
    }
    case Type::kNot: {
      auto operand = ExpNodeFromLepusOrNull(payload);
      if (!operand) {
        return nullptr;
      }
      return fml::MakeRefCounted<MediaQueryNotExpNode>(std::move(operand));
    }
    case Type::kAnd: {
      if (!payload.IsArray()) return nullptr;
      const auto& kids = payload.Array();
      if (kids->size() < 2) return nullptr;
      auto left = ExpNodeFromLepusOrNull(kids->get(0));
      auto right = ExpNodeFromLepusOrNull(kids->get(1));
      return fml::MakeRefCounted<MediaQueryAndExpNode>(std::move(left),
                                                       std::move(right));
    }
    case Type::kOr: {
      if (!payload.IsArray()) return nullptr;
      const auto& kids = payload.Array();
      if (kids->size() < 2) return nullptr;
      auto left = ExpNodeFromLepusOrNull(kids->get(0));
      auto right = ExpNodeFromLepusOrNull(kids->get(1));
      return fml::MakeRefCounted<MediaQueryOrExpNode>(std::move(left),
                                                      std::move(right));
    }
  }
  return nullptr;
}

}  // namespace css
}  // namespace lynx
