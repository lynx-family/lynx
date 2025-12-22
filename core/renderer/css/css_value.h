// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_VALUE_H_
#define CORE_RENDERER_CSS_CSS_VALUE_H_

#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "base/include/flex_optional.h"
#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "core/base/lynx_export.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace tasm {
enum class CSSValuePattern : uint8_t {
  EMPTY = 0,
  STRING = 1,
  NUMBER = 2,
  BOOLEAN = 3,
  ENUM = 4,
  PX = 5,
  RPX = 6,
  EM = 7,
  REM = 8,
  VH = 9,
  VW = 10,
  PERCENT = 11,
  CALC = 12,
  ENV = 13,
  ARRAY = 14,
  MAP = 15,
  PPX = 16,
  INTRINSIC = 17,
  SP = 18,
  FR = 19,
  COUNT = 20,
};

enum class CSSValueType : uint8_t {
  DEFAULT = 0,
  VARIABLE = 1,
};

enum class CSSFunctionType : uint8_t {
  DEFAULT = 0,
  REPEAT = 1,
  MINMAX = 2,
};

class CSSValue;
struct VarReference {
  size_t name_start;
  size_t name_end;
  size_t start;
  size_t end;
  size_t offset = 4;  // Default offset for var() format, use 2 for {{}} format
  base::String fallback;
  CSSParserConfigs parser_configs;
  std::string_view Name(const std::string& raw_value) const;
};

using CustomPropertiesMap = base::LinearFlatMap<base::String, CSSValue>;

class LYNX_EXPORT_FOR_DEVTOOL CSSValue {
 public:
  CSSValue() : pattern_(CSSValuePattern::EMPTY), type_(CSSValueType::DEFAULT) {}

  CSSValue(const base::String& value, CSSValuePattern pattern,
           CSSValueType type)
      : value_(value), pattern_(pattern), type_(type) {}
  CSSValue(base::String&& value, CSSValuePattern pattern, CSSValueType type)
      : value_(std::move(value)), pattern_(pattern), type_(type) {}
  CSSValue(const char* value, CSSValuePattern pattern, CSSValueType type)
      : value_(value), pattern_(pattern), type_(type) {}
  CSSValue(const std::string& value, CSSValuePattern pattern, CSSValueType type)
      : value_(value), pattern_(pattern), type_(type) {}
  CSSValue(std::string&& value, CSSValuePattern pattern, CSSValueType type)
      : value_(std::move(value)), pattern_(pattern), type_(type) {}

  template <typename EnumT, typename = std::enable_if_t<std::is_enum_v<EnumT>>>
  explicit CSSValue(EnumT value)
      : value_(static_cast<int32_t>(value)),
        pattern_(CSSValuePattern::ENUM),
        type_(CSSValueType::DEFAULT) {}

  explicit CSSValue(bool value)
      : value_(value),
        pattern_(CSSValuePattern::BOOLEAN),
        type_(CSSValueType::DEFAULT) {}

#define NumberConstructor(name, type)           \
  CSSValue(type value, CSSValuePattern pattern) \
      : value_(value), pattern_(pattern), type_(CSSValueType::DEFAULT) {}

  NumberType(NumberConstructor)
#undef NumberConstructor

      explicit CSSValue(const fml::RefPtr<lepus::CArray>& array)
      : value_(array),
        pattern_(CSSValuePattern::ARRAY),
        type_(CSSValueType::DEFAULT) {
  }

  explicit CSSValue(fml::RefPtr<lepus::CArray>&& array)
      : value_(std::move(array)),
        pattern_(CSSValuePattern::ARRAY),
        type_(CSSValueType::DEFAULT) {}

  // It is not recommended to construct CSSValue from opaque lepus::Value,
  // prefer other constructors.
  CSSValue(const lepus::Value& value, CSSValuePattern pattern,
           CSSValueType type = CSSValueType::DEFAULT)
      : value_(value), pattern_(pattern), type_(type) {}

  CSSValue(lepus::Value&& value, CSSValuePattern pattern,
           CSSValueType type = CSSValueType::DEFAULT)
      : value_(std::move(value)), pattern_(pattern), type_(type) {}

  CSSValue(const CSSValue& other)
      : value_(other.value_),
        default_value_(other.default_value_),
        pattern_(other.pattern_),
        type_(other.type_) {
    if (other.default_value_map_opt_) {
      default_value_map_opt_ =
          std::make_unique<lepus::Value>(*other.default_value_map_opt_);
    }
    if (other.var_references_) {
      var_references_ = std::make_unique<base::InlineVector<VarReference, 1>>(
          *other.var_references_);
      needs_variable_resolution_ = other.needs_variable_resolution_;
    }
  }

  CSSValue& operator=(const CSSValue& other) {
    if (this == &other) {
      return *this;
    }
    this->value_ = other.value_;
    this->default_value_ = other.default_value_;
    if (other.default_value_map_opt_) {
      this->default_value_map_opt_ =
          std::make_unique<lepus::Value>(*other.default_value_map_opt_);
    } else {
      this->default_value_map_opt_ = nullptr;
    }
    if (other.var_references_) {
      var_references_ = std::make_unique<base::InlineVector<VarReference, 1>>(
          *other.var_references_);
      needs_variable_resolution_ = other.needs_variable_resolution_;
    } else {
      var_references_ = nullptr;
    }
    this->pattern_ = other.pattern_;
    this->type_ = other.type_;
    return *this;
  }

  CSSValue(CSSValue&& other) = default;
  CSSValue& operator=(CSSValue&& other) = default;

  // TODO(yuyang), The `value_` variable in `CSSValue` will no longer be of type
  // `lepus::Value`.
  lepus::Value GetValue() const { return value_; }

  // TODO(yuyang), remove Unsafe after all refactor completes.
  class Unsafe {
    // ATTENTION: function under this class is for optimization purpose and is
    // UNSAFE to use.
   public:
    Unsafe() = delete;
    static inline lepus::Value& GetValueStorage(CSSValue& value) {
      return value.value_;
    }

    static inline const lepus::Value& GetValueStorage(const CSSValue& value) {
      return value.value_;
    }
  };

  CSSValuePattern GetPattern() const { return pattern_; }
  CSSValueType GetValueType() const { return type_; }
  const base::String& GetDefaultValue() const { return default_value_; }
  lepus::Value GetDefaultValueMapOpt() const {
    return default_value_map_opt_ ? *default_value_map_opt_ : lepus::Value();
  }

  void SetPattern(CSSValuePattern pattern) { pattern_ = pattern; }
  void SetValueAndPattern(const lepus::Value& value, CSSValuePattern pattern) {
    value_ = value;
    pattern_ = pattern;
    type_ = CSSValueType::DEFAULT;
  }

  void SetType(CSSValueType type) { type_ = type; }
  void SetDefaultValue(base::String default_val) {
    default_value_ = std::move(default_val);
  }
  void SetDefaultValueMap(lepus::Value default_value_map) {
    default_value_map_opt_ = nullptr;
    if (default_value_map != lepus::Value()) {
      default_value_map_opt_ =
          std::make_unique<lepus::Value>(std::move(default_value_map));
    }
  }

  template <typename T>
  T GetEnum() const {
    return (T)AsNumber();
  }

  fml::WeakRefPtr<lepus::CArray> GetArray() const& { return value_.Array(); }
  fml::RefPtr<lepus::CArray> GetArray() && { return std::move(value_).Array(); }

  double GetNumber() const { return value_.Number(); }
  double AsNumber() const { return value_.Number(); }
  bool GetBool() const { return value_.Bool(); }
  bool AsBool() const { return value_.Bool(); }

  base::String AsString() const& { return value_.String(); }
  base::String AsString() && { return std::move(value_).String(); }

  const std::string& AsStdString() const& { return value_.StdString(); }
  std::string AsStdString() && { return value_.StdString(); }

  void SetArray(fml::RefPtr<lepus::CArray>&& array) {
    value_.SetArray(std::move(array));
    pattern_ = CSSValuePattern::ARRAY;
    type_ = CSSValueType::DEFAULT;
  }

  void SetBoolean(bool value) {
    value_.SetBool(value);
    pattern_ = CSSValuePattern::BOOLEAN;
    type_ = CSSValueType::DEFAULT;
  }

  template <typename NumType>
  void SetNumber(NumType num, CSSValuePattern pattern) {
    DCHECK(pattern != CSSValuePattern::ENUM);  // Use SetEnum instead
    value_.SetNumber(num);
    pattern_ = pattern;
    type_ = CSSValueType::DEFAULT;
  }

  void SetString(const base::String& value, CSSValuePattern pattern) {
    value_.SetString(value);
    pattern_ = pattern;
    type_ = CSSValueType::DEFAULT;
  }

  void SetString(base::String&& value, CSSValuePattern pattern) {
    value_.SetString(std::move(value));
    pattern_ = pattern;
    type_ = CSSValueType::DEFAULT;
  }

  template <typename EnumT, typename = std::enable_if_t<std::is_enum_v<EnumT>>>
  void SetEnum(EnumT value) {
    value_.SetNumber(static_cast<int32_t>(value));
    pattern_ = CSSValuePattern::ENUM;
    type_ = CSSValueType::DEFAULT;
  }

  void SetVarReferences(
      std::unique_ptr<base::InlineVector<VarReference, 1>> var_references) {
    var_references_ = std::move(var_references);
    type_ = CSSValueType::VARIABLE;
    needs_variable_resolution_ = true;
  }
  const base::InlineVector<VarReference, 1>* GetVarReferences() const {
    return var_references_.get();
  }
  bool NeedsVariableResolution() const { return needs_variable_resolution_; }

  bool IsVariable() const { return type_ == CSSValueType::VARIABLE; }
  bool IsString() const { return pattern_ == CSSValuePattern::STRING; }
  bool IsNumber() const { return pattern_ == CSSValuePattern::NUMBER; }
  bool IsBoolean() const { return pattern_ == CSSValuePattern::BOOLEAN; }
  bool IsEnum() const { return pattern_ == CSSValuePattern::ENUM; }
  bool IsPx() const { return pattern_ == CSSValuePattern::PX; }
  bool IsPPx() const { return pattern_ == CSSValuePattern::PPX; }
  bool IsRpx() const { return pattern_ == CSSValuePattern::RPX; }
  bool IsEm() const { return pattern_ == CSSValuePattern::EM; }
  bool IsRem() const { return pattern_ == CSSValuePattern::REM; }
  bool IsVh() const { return pattern_ == CSSValuePattern::VH; }
  bool IsVw() const { return pattern_ == CSSValuePattern::VW; }
  bool IsPercent() const { return pattern_ == CSSValuePattern::PERCENT; }
  bool IsCalc() const { return pattern_ == CSSValuePattern::CALC; }
  bool IsArray() const { return pattern_ == CSSValuePattern::ARRAY; }
  bool IsMap() const { return pattern_ == CSSValuePattern::MAP; }
  bool IsEmpty() const { return pattern_ == CSSValuePattern::EMPTY; }
  bool IsEnv() const { return pattern_ == CSSValuePattern::ENV; }
  bool IsIntrinsic() const { return pattern_ == CSSValuePattern::INTRINSIC; }
  bool IsSp() const { return pattern_ == CSSValuePattern::SP; }

  std::string AsJsonString(bool map_key_ordered = false) const;

  friend bool operator==(const CSSValue& left, const CSSValue& right) {
    return left.pattern_ == right.pattern_ && left.value_ == right.value_;
  }

  friend bool operator!=(const CSSValue& left, const CSSValue& right) {
    return !(left == right);
  }

  using HandleCustomPropertyFunc =
      std::function<void(const base::String&, const base::String&)>;
  static void SubstituteAll(
      CustomPropertiesMap& custom_properties, int max_depth = 10,
      const HandleCustomPropertyFunc& handle_func = nullptr);
  static std::string Substitution(
      const CSSValue& css_value, const CustomPropertiesMap& variable_map,
      int max_depth = 10,
      const HandleCustomPropertyFunc& handle_func = nullptr);

  // Change the legacy css variable value which in a format with {{ variable }}
  // into the new VarReference based variables.
  // TODO(renzhongyue): reference values can be directly encode into template to
  // reduce runtime overheads.
  bool ToVarReference();

 private:
  class CycleDetector;
  friend class Unsafe;

  // Make a CSSValue of default type and string value. Normally for unit-tests.
  static CSSValue MakePlainString(const char* value) {
    return CSSValue(value, CSSValuePattern::STRING, CSSValueType::DEFAULT);
  }

  static std::string Substitution(
      const CSSValue& css_value, const CustomPropertiesMap& variable_map,
      const CycleDetector& detector, int max_depth = 10,
      const HandleCustomPropertyFunc& handle_func = nullptr);
  static std::string ResolveVariable(
      const std::string& var_name, const CustomPropertiesMap& custom_properties,
      const CycleDetector& detector, int max_depth = 10,
      const HandleCustomPropertyFunc& handle_func = nullptr);
  static std::string ResolveFallback(
      const VarReference& ref, const CustomPropertiesMap& variable_map,
      const CycleDetector& detector, int max_depth = 10,
      const HandleCustomPropertyFunc& handle_func = nullptr);
  mutable lepus::Value value_;
  mutable base::String default_value_;
  mutable std::unique_ptr<lepus::Value> default_value_map_opt_;
  mutable std::unique_ptr<base::InlineVector<VarReference, 1>> var_references_;
  mutable CSSValuePattern pattern_;
  mutable CSSValueType type_;
  // TODO(yangguangzhao.solace): Currently, `needs_variable_resolution_` serves
  // a dual purpose: it indicates
  // both whether the current CSSValue is handled by the new parsing logic and
  // whether it requires further resolution. After the old CSSVariable parsing
  // logic is deprecated, we will update this flag to solely represent its
  // single, intended meaning: whether the CSSValue needs further resolution.
  mutable bool needs_variable_resolution_{false};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_VALUE_H_
