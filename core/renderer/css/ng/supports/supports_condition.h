// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_SUPPORTS_SUPPORTS_CONDITION_H_
#define CORE_RENDERER_CSS_NG_SUPPORTS_SUPPORTS_CONDITION_H_

#include <cstdint>
#include <string>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"

namespace lynx {

namespace lepus {
class Value;
}  // namespace lepus
using lepus_value = lepus::Value;

namespace css {

class SupportsConditionNode : public fml::RefCountedThreadSafeStorage {
 public:
  // Stable numbering for serialization. Append-only.
  enum class Type : uint8_t {
    kNested = 0,           // ( inner )
    kNot = 1,              // not <operand>
    kAnd = 2,              // left and right
    kOr = 3,               // left or right
    kDeclaration = 4,      // (property: value [!important])
    kEngineVersion = 5,    // -x-engine-version(<begin>, <end>)
    kFunction = 6,         // name(raw_args)
    kGeneralEnclosed = 7,  // catch-all, always false
  };

  explicit SupportsConditionNode(Type type) : type_(type) {}
  void ReleaseSelf() const override { delete this; }

  Type GetType() const { return type_; }

  virtual std::string Serialize() const = 0;
  virtual lepus_value ToLepus() const = 0;
  static fml::RefPtr<const SupportsConditionNode> FromLepus(
      const lepus_value& value);

 protected:
  ~SupportsConditionNode() override = default;

 private:
  Type type_;
};

class SupportsDeclNode final : public SupportsConditionNode {
 public:
  SupportsDeclNode(std::string property, std::string value, bool important)
      : SupportsConditionNode(Type::kDeclaration),
        property_(std::move(property)),
        value_(std::move(value)),
        important_(important) {}

  const std::string& Property() const { return property_; }
  const std::string& Value() const { return value_; }
  bool Important() const { return important_; }

  std::string Serialize() const override;
  lepus_value ToLepus() const override;

 private:
  std::string property_;
  std::string value_;
  bool important_ = false;
};

class SupportsEngineVersionNode final : public SupportsConditionNode {
 public:
  static constexpr uint16_t kVersionMax = 0xFFFF;

  // Pack major.minor into a single uint32: high 16 = major, low 16 = minor.
  static constexpr uint32_t PackVersion(uint16_t major, uint16_t minor) {
    return (static_cast<uint32_t>(major) << 16) | minor;
  }
  static constexpr uint16_t Major(uint32_t v) {
    return static_cast<uint16_t>(v >> 16);
  }
  static constexpr uint16_t Minor(uint32_t v) {
    return static_cast<uint16_t>(v & 0xFFFF);
  }

  static constexpr uint32_t kMaxVersion =
      (static_cast<uint32_t>(kVersionMax) << 16) | kVersionMax;

  SupportsEngineVersionNode(uint32_t begin, uint32_t end)
      : SupportsConditionNode(Type::kEngineVersion), begin_(begin), end_(end) {}

  uint32_t Begin() const { return begin_; }
  uint32_t End() const { return end_; }

  std::string Serialize() const override;
  lepus_value ToLepus() const override;

 private:
  uint32_t begin_;
  uint32_t end_;
};

class SupportsFunctionNode final : public SupportsConditionNode {
 public:
  SupportsFunctionNode(std::string name, std::string raw_args)
      : SupportsConditionNode(Type::kFunction),
        name_(std::move(name)),
        raw_args_(std::move(raw_args)) {}

  const std::string& Name() const { return name_; }
  const std::string& RawArgs() const { return raw_args_; }

  std::string Serialize() const override;
  lepus_value ToLepus() const override;

 private:
  std::string name_;
  std::string raw_args_;
};

class SupportsGeneralEnclosedNode final : public SupportsConditionNode {
 public:
  explicit SupportsGeneralEnclosedNode(std::string raw)
      : SupportsConditionNode(Type::kGeneralEnclosed), raw_(std::move(raw)) {}

  const std::string& Raw() const { return raw_; }

  std::string Serialize() const override;
  lepus_value ToLepus() const override;

 private:
  std::string raw_;
};

class SupportsUnaryNode final : public SupportsConditionNode {
 public:
  SupportsUnaryNode(Type type, fml::RefPtr<const SupportsConditionNode> operand)
      : SupportsConditionNode(type), operand_(std::move(operand)) {}

  const fml::RefPtr<const SupportsConditionNode>& Operand() const {
    return operand_;
  }

  std::string Serialize() const override;
  lepus_value ToLepus() const override;

 private:
  fml::RefPtr<const SupportsConditionNode> operand_;
};

class SupportsBinaryNode final : public SupportsConditionNode {
 public:
  SupportsBinaryNode(Type type, fml::RefPtr<const SupportsConditionNode> left,
                     fml::RefPtr<const SupportsConditionNode> right)
      : SupportsConditionNode(type),
        left_(std::move(left)),
        right_(std::move(right)) {}

  const fml::RefPtr<const SupportsConditionNode>& Left() const { return left_; }
  const fml::RefPtr<const SupportsConditionNode>& Right() const {
    return right_;
  }

  std::string Serialize() const override;
  lepus_value ToLepus() const override;

 private:
  fml::RefPtr<const SupportsConditionNode> left_;
  fml::RefPtr<const SupportsConditionNode> right_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_SUPPORTS_SUPPORTS_CONDITION_H_
