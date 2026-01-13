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

#include "base/include/base_defines.h"
#include "base/include/bundled_optional.h"
#include "base/include/flex_optional.h"
#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "core/base/lynx_export.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/starlight/style/copyable.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/style/data_ref.h"

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
using CustomPropertiesMapRef =
    starlight::DataRef<starlight::Copyable<CustomPropertiesMap>>;

class LYNX_EXPORT_FOR_DEVTOOL CSSValue {
 public:
  CSSValue() {}  // __attr__ initialized to zero.
  ~CSSValue() { FreeValueStorage(); }

  CSSValue(const CSSValue& other);
  CSSValue& operator=(const CSSValue& other);
  CSSValue(CSSValue&& other) noexcept;
  CSSValue& operator=(CSSValue&& other) noexcept;

  CSSValue(const base::String& value, CSSValuePattern pattern,
           CSSValueType value_type);
  CSSValue(base::String&& value, CSSValuePattern pattern,
           CSSValueType value_type);
  CSSValue(const char* value, CSSValuePattern pattern, CSSValueType value_type);
  CSSValue(const std::string& value, CSSValuePattern pattern,
           CSSValueType value_type);
  CSSValue(std::string&& value, CSSValuePattern pattern,
           CSSValueType value_type);

  template <typename EnumT, typename = std::enable_if_t<std::is_enum_v<EnumT>>>
  explicit CSSValue(EnumT value) : val_int32(static_cast<int32_t>(value)) {
    // Firstly let `__attr__` be set to 0 and then assign other fields.
    type_ = lynx_value_int32;
    pattern_ = CSSValuePattern::ENUM;
  }

  explicit CSSValue(bool value) : val_bool(value) {
    // Firstly let `__attr__` be set to 0 and then assign other fields.
    type_ = lynx_value_bool;
    pattern_ = CSSValuePattern::BOOLEAN;
  }

  CSSValue(double value, CSSValuePattern pattern) : val_double(value) {
    // Firstly let `__attr__` be set to 0 and then assign other fields.
    type_ = lynx_value_double;
    pattern_ = pattern;
  }

  CSSValue(int32_t value, CSSValuePattern pattern) : val_int32(value) {
    // Firstly let `__attr__` be set to 0 and then assign other fields.
    type_ = lynx_value_int32;
    pattern_ = pattern;
  }

  CSSValue(uint32_t value, CSSValuePattern pattern) : val_uint32(value) {
    // Firstly let `__attr__` be set to 0 and then assign other fields.
    type_ = lynx_value_uint32;
    pattern_ = pattern;
  }

  CSSValue(int64_t value, CSSValuePattern pattern) : val_int64(value) {
    // Firstly let `__attr__` be set to 0 and then assign other fields.
    type_ = lynx_value_int64;
    pattern_ = pattern;
  }

  CSSValue(uint64_t value, CSSValuePattern pattern) : val_uint64(value) {
    // Firstly let `__attr__` be set to 0 and then assign other fields.
    type_ = lynx_value_uint64;
    pattern_ = pattern;
  }

  explicit CSSValue(const fml::RefPtr<lepus::CArray>& array);
  explicit CSSValue(fml::RefPtr<lepus::CArray>&& array);

  // It is not recommended to construct CSSValue from opaque lepus::Value,
  // prefer other constructors.
  CSSValue(const lepus::Value& value, CSSValuePattern pattern,
           CSSValueType value_type = CSSValueType::DEFAULT);
  CSSValue(lepus::Value&& value, CSSValuePattern pattern,
           CSSValueType value_type = CSSValueType::DEFAULT);

  lepus::Value GetValue() const;
  CSSValuePattern GetPattern() const { return pattern_; }
  CSSValueType GetValueType() const {
    return is_variable_ ? CSSValueType::VARIABLE : CSSValueType::DEFAULT;
  }
  base::String GetDefaultValue() const;
  lepus::Value GetDefaultValueMapOpt() const;

  void SetPattern(CSSValuePattern pattern) { pattern_ = pattern; }
  void SetValueAndPattern(const lepus::Value& value, CSSValuePattern pattern);
  void SetType(CSSValueType type) {
    is_variable_ = type == CSSValueType::VARIABLE;
  }
  void SetDefaultValue(base::String default_val);
  void SetDefaultValueMap(lepus::Value default_value_map);

  template <typename T>
  T GetEnum() const {
    return (T)AsNumber();
  }

  fml::WeakRefPtr<lepus::CArray> GetArray() const&;
  fml::RefPtr<lepus::CArray> GetArray() &&;

  double GetNumber() const;
  double AsNumber() const { return GetNumber(); }
  bool GetBool() const;
  bool AsBool() const { return GetBool(); }

  base::String AsString() const&;
  base::String AsString() &&;
  const std::string& AsStdString() const;

  void SetArray(fml::RefPtr<lepus::CArray>&& array);

  void SetBoolean(bool value);
  void SetNumber(double val, CSSValuePattern pattern);
  void SetNumber(int32_t val, CSSValuePattern pattern);
  void SetNumber(uint32_t val, CSSValuePattern pattern);
  void SetNumber(int64_t val, CSSValuePattern pattern);
  void SetNumber(uint64_t val, CSSValuePattern pattern);

  void SetString(const base::String& value, CSSValuePattern pattern);
  void SetString(base::String&& value, CSSValuePattern pattern);

  template <typename EnumT, typename = std::enable_if_t<std::is_enum_v<EnumT>>>
  void SetEnum(EnumT value) {
    FreeValueStorage();
    val_int32 = static_cast<int32_t>(value);
    type_ = lynx_value_int32;
    pattern_ = CSSValuePattern::ENUM;
    is_variable_ = false;
  }

  void SetVarReferences(base::Vector<VarReference> var_references);
  bool NeedsVariableResolution() const { return needs_variable_resolution_; }

  bool IsVariable() const { return is_variable_; }
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

  friend bool operator==(const CSSValue& left, const CSSValue& right);
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
  static std::string SubstitutionResolved(
      const CSSValue& css_value, const CustomPropertiesMap& variable_map,
      const HandleCustomPropertyFunc& handle_func = nullptr);

  // Change the legacy css variable value which in a format with {{ variable }}
  // into the new VarReference based variables.
  // TODO(renzhongyue): reference values can be directly encode into template to
  // reduce runtime overheads.
  bool ToVarReference();

  // A flag telling base containers such as `base::Vector<Value>` and maps
  // built on vector to optimize for reallocate, insert and erase.
  using TriviallyRelocatable = bool;

 private:
  class CycleDetector;
  friend class LynxBinaryBaseCSSReader;

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
  static std::string ResolveFallbackResolved(
      const VarReference& ref, const CustomPropertiesMap& variable_map,
      const HandleCustomPropertyFunc& handle_func = nullptr);

  BASE_INLINE bool IsValueStorageNumber() const {
    return type_ >= lynx_value_double && type_ <= lynx_value_uint64;
  }
  BASE_INLINE bool IsValueStorageReference() const {
    return type_ >= lynx_value_string && type_ <= lynx_value_object;
  }
  BASE_INLINE void FreeValueStorage() {
    if (IsValueStorageReference()) {
      reinterpret_cast<fml::RefCountedThreadSafeStorage*>(val_ptr)->Release();
    }
  }

  LYNX_VALUE_BASE_STORAGE_DEFINITION
  union {
    // All attributes initialized to 0 by single instruction.
    uintptr_t __attr__{0};

    struct {
      lynx_value_type type_;
      CSSValuePattern pattern_;

      struct {
        bool is_variable_ : 1;

        // TODO(yangguangzhao.solace): Currently, `needs_variable_resolution_`
        // serves a dual purpose: it indicates both whether the current
        // CSSValue is handled by the new parsing logic and whether it
        // requires further resolution. After the old CSSVariable parsing
        // logic is deprecated, we will update this flag to solely represent
        // its single, intended meaning: whether the CSSValue needs further
        // resolution.
        bool needs_variable_resolution_ : 1;
      };
    };
  };

  struct DefaultValueField {
    using Type = base::String;
  };
  struct DefaultValueMapField {
    using Type = lepus::Value;
  };
  struct VarReferenceField {
    using Type = base::Vector<VarReference>;
  };
  mutable base::BundledOptionals<DefaultValueField, DefaultValueMapField,
                                 VarReferenceField>
      optionals_;
};

static_assert(
    sizeof(CSSValue) <= 32,
    "The CSSValue struct has been precisely optimized to no more "
    "than 32 bytes. If you need to add fields and this assertion is "
    "broken, please contact the code owners of the CSS module first.");

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_VALUE_H_
