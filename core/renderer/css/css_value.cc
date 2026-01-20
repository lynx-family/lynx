// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/css_value.h"

#include <algorithm>
#include <cmath>

#include "base/include/value/table.h"
#include "base/include/vector.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/runtime/lepus/json_parser.h"

namespace lynx {
namespace tasm {

CSSValue::CSSValue(const CSSValue& other)
    : val_uint64(other.val_uint64),
      __attr__(other.__attr__),
      optionals_(other.optionals_) {
  static_assert(
      offsetof(CSSValue, optionals_) - offsetof(CSSValue, __attr__) ==
          sizeof(CSSValue::__attr__),
      "When modifying the attributes union, the type of the `__attr__` "
      "variable needs to be adjusted simultaneously to ensure that its {0} "
      "initialization can clear the entire structure bits to zero.");

  if (IsValueStorageReference()) {
    reinterpret_cast<fml::RefCountedThreadSafeStorage*>(val_ptr)->AddRef();
  }
}

CSSValue& CSSValue::operator=(const CSSValue& other) {
  if (unlikely(this == &other)) {
    return *this;
  }
  FreeValueStorage();
  val_uint64 = other.val_uint64;
  __attr__ = other.__attr__;
  optionals_ = other.optionals_;
  if (IsValueStorageReference()) {
    reinterpret_cast<fml::RefCountedThreadSafeStorage*>(val_ptr)->AddRef();
  }
  return *this;
}

CSSValue::CSSValue(CSSValue&& other) noexcept
    : val_uint64(other.val_uint64),
      __attr__(other.__attr__),
      optionals_(std::move(other.optionals_)) {
  other.__attr__ = 0;
}

CSSValue& CSSValue::operator=(CSSValue&& other) noexcept {
  if (likely(this != &other)) {
    this->~CSSValue();
    new (this) CSSValue(std::move(other));
  }
  return *this;
}

CSSValue::CSSValue(const base::String& value, CSSValuePattern pattern,
                   CSSValueType value_type) {
  // Firstly let `__attr__` be set to 0 and then assign other fields.
  type_ = lynx_value_string;
  pattern_ = pattern;
  is_variable_ = value_type == CSSValueType::VARIABLE;
  auto* str = base::String::Unsafe::GetUntaggedStringRawRef(value);
  val_ptr = reinterpret_cast<lynx_value_ptr>(str);
  str->AddRef();
}

CSSValue::CSSValue(base::String&& value, CSSValuePattern pattern,
                   CSSValueType value_type) {
  // Firstly let `__attr__` be set to 0 and then assign other fields.
  type_ = lynx_value_string;
  pattern_ = pattern;
  is_variable_ = value_type == CSSValueType::VARIABLE;
  auto* str = base::String::Unsafe::GetUntaggedStringRawRef(value);
  val_ptr = reinterpret_cast<lynx_value_ptr>(str);
  if (str != base::String::Unsafe::GetStringRawRef(value)) {
    str->AddRef();
  }
  base::String::Unsafe::SetStringToEmpty(value);
}

CSSValue::CSSValue(const char* value, CSSValuePattern pattern,
                   CSSValueType value_type) {
  // Firstly let `__attr__` be set to 0 and then assign other fields.
  type_ = lynx_value_string;
  pattern_ = pattern;
  is_variable_ = value_type == CSSValueType::VARIABLE;
  auto* str = base::RefCountedStringImpl::Unsafe::RawCreate(value);
  val_ptr = reinterpret_cast<lynx_value_ptr>(str);
}

CSSValue::CSSValue(const std::string& value, CSSValuePattern pattern,
                   CSSValueType value_type) {
  // Firstly let `__attr__` be set to 0 and then assign other fields.
  type_ = lynx_value_string;
  pattern_ = pattern;
  is_variable_ = value_type == CSSValueType::VARIABLE;
  auto* ptr = base::RefCountedStringImpl::Unsafe::RawCreate(value);
  val_ptr = reinterpret_cast<lynx_value_ptr>(ptr);
}

CSSValue::CSSValue(std::string&& value, CSSValuePattern pattern,
                   CSSValueType value_type) {
  // Firstly let `__attr__` be set to 0 and then assign other fields.
  type_ = lynx_value_string;
  pattern_ = pattern;
  is_variable_ = value_type == CSSValueType::VARIABLE;
  auto* ptr = base::RefCountedStringImpl::Unsafe::RawCreate(std::move(value));
  val_ptr = reinterpret_cast<lynx_value_ptr>(ptr);
}

CSSValue::CSSValue(const fml::RefPtr<lepus::CArray>& array)
    : val_ptr(reinterpret_cast<lynx_value_ptr>(array.get())) {
  // Firstly let `__attr__` be set to 0 and then assign other fields.
  type_ = lynx_value_array;
  pattern_ = CSSValuePattern::ARRAY;
  array.get()->AddRef();
}

CSSValue::CSSValue(fml::RefPtr<lepus::CArray>&& array)
    : val_ptr(reinterpret_cast<lynx_value_ptr>(array.AbandonRef())) {
  // Firstly let `__attr__` be set to 0 and then assign other fields.
  type_ = lynx_value_array;
  pattern_ = CSSValuePattern::ARRAY;
}

CSSValue::CSSValue(const lepus::Value& value, CSSValuePattern pattern,
                   CSSValueType value_type)
    : val_uint64(value.value().val_uint64) {
  // Firstly let `__attr__` be set to 0 and then assign other fields.
  type_ = value.value().type;
  pattern_ = pattern;
  DCHECK(type_ <= lynx_value_map);
  is_variable_ = value_type == CSSValueType::VARIABLE;
  if (IsValueStorageReference()) {
    reinterpret_cast<fml::RefCountedThreadSafeStorage*>(val_ptr)->AddRef();
  }
}

CSSValue::CSSValue(lepus::Value&& value, CSSValuePattern pattern,
                   CSSValueType value_type)
    : val_uint64(value.value().val_uint64) {
  // Firstly let `__attr__` be set to 0 and then assign other fields.
  type_ = value.value().type;
  pattern_ = pattern;
  DCHECK(type_ <= lynx_value_map);
  is_variable_ = value_type == CSSValueType::VARIABLE;
  const_cast<lynx_value&>(value.value()).type = lynx_value_null;
}

lepus::Value CSSValue::GetValue() const {
  lepus::Value result(lepus::Value::kUnsafeCreateAsUninitialized);
  const_cast<lynx_value&>(result.value()).val_uint64 = val_uint64;
  const_cast<lynx_value&>(result.value()).type = type_;
  if (IsValueStorageReference()) {
    reinterpret_cast<fml::RefCountedThreadSafeStorage*>(val_ptr)->AddRef();
  }
  return result;
}

base::String CSSValue::GetDefaultValue() const {
  if (optionals_.HasValue<DefaultValueField>()) {
    return optionals_.Get<DefaultValueField>();
  } else {
    return base::String();
  }
}

lepus::Value CSSValue::GetDefaultValueMapOpt() const {
  if (optionals_.HasValue<DefaultValueMapField>()) {
    return optionals_.Get<DefaultValueMapField>();
  } else {
    return lepus::Value();
  }
}

void CSSValue::SetDefaultValue(base::String default_val) {
  if (default_val.empty()) {
    // Release instead of assignment to prevent the memory of dynamic property
    // parts from being created unexpectedly.
    optionals_.Release<DefaultValueField>();
  } else {
    optionals_.Get<DefaultValueField>() = std::move(default_val);
  }
}

void CSSValue::SetDefaultValueMap(lepus::Value default_value_map) {
  if (default_value_map.IsTable() && !default_value_map.Table()->empty()) {
    optionals_.Get<DefaultValueMapField>() = std::move(default_value_map);
  } else {
    optionals_.Release<DefaultValueMapField>();
  }
}

void CSSValue::SetVarReferences(base::Vector<VarReference> var_references) {
  if (var_references.empty()) {
    optionals_.Release<VarReferenceField>();
  } else {
    optionals_.Get<VarReferenceField>() = std::move(var_references);
  }
  is_variable_ = true;
  needs_variable_resolution_ = true;
}

void CSSValue::SetValueAndPattern(const lepus::Value& value,
                                  CSSValuePattern pattern) {
  FreeValueStorage();
  val_uint64 = value.value().val_uint64;
  type_ = value.value().type;
  pattern_ = pattern;
  if (IsValueStorageReference()) {
    reinterpret_cast<fml::RefCountedThreadSafeStorage*>(val_ptr)->AddRef();
  }
}

fml::WeakRefPtr<lepus::CArray> CSSValue::GetArray() const& {
  return fml::WeakRefPtr<lepus::CArray>(
      val_ptr != nullptr && type_ == lynx_value_array
          ? reinterpret_cast<lepus::CArray*>(val_ptr)
          : lepus::Value::DummyArray());
}

fml::RefPtr<lepus::CArray> CSSValue::GetArray() && {
  if (val_ptr != nullptr && type_ == lynx_value_array) {
    return fml::RefPtr<lepus::CArray>(
        reinterpret_cast<lepus::CArray*>(val_ptr));
  }
  return lepus::CArray::Create();
}

double CSSValue::GetNumber() const {
  switch (type_) {
    case lynx_value_double:
      return val_double;
    case lynx_value_int32:
      return val_int32;
    case lynx_value_uint32:
      return val_uint32;
    case lynx_value_int64:
      return val_int64;
    case lynx_value_uint64:
      return val_uint64;
    default:
      return 0.f;
  }
}

bool CSSValue::GetBool() const {
  if (type_ == lynx_value_bool) {
    return val_bool;
  } else {
    return GetValue().Bool();
  }
}

base::String CSSValue::AsString() const& {
  if (type_ == lynx_value_string) {
    return base::String::Unsafe::ConstructWeakRefStringFromRawRef(
        reinterpret_cast<base::RefCountedStringImpl*>(val_ptr));
  } else {
    return base::String();
  }
}

base::String CSSValue::AsString() && {
  if (type_ == lynx_value_string) {
    return base::String::Unsafe::ConstructStringFromRawRef(
        reinterpret_cast<base::RefCountedStringImpl*>(val_ptr));
  } else {
    return base::String();
  }
}

const std::string& CSSValue::AsStdString() const {
  if (type_ == lynx_value_string) {
    return reinterpret_cast<base::RefCountedStringImpl*>(val_ptr)->str();
  } else {
    return base::RefCountedStringImpl::Unsafe::kEmptyString.str();
  }
}

void CSSValue::SetArray(fml::RefPtr<lepus::CArray>&& array) {
  FreeValueStorage();
  val_ptr = reinterpret_cast<lynx_value_ptr>(array.AbandonRef());
  type_ = lynx_value_array;
  pattern_ = CSSValuePattern::ARRAY;
  is_variable_ = false;
}

void CSSValue::SetBoolean(bool value) {
  FreeValueStorage();
  val_bool = value;
  type_ = lynx_value_bool;
  pattern_ = CSSValuePattern::BOOLEAN;
  is_variable_ = false;
}

void CSSValue::SetNumber(double val, CSSValuePattern pattern) {
  FreeValueStorage();
  val_double = val;
  type_ = lynx_value_double;
  pattern_ = pattern;
  is_variable_ = false;
}

void CSSValue::SetNumber(int32_t val, CSSValuePattern pattern) {
  FreeValueStorage();
  val_int32 = val;
  type_ = lynx_value_int32;
  pattern_ = pattern;
  is_variable_ = false;
}

void CSSValue::SetNumber(uint32_t val, CSSValuePattern pattern) {
  FreeValueStorage();
  val_uint32 = val;
  type_ = lynx_value_uint32;
  pattern_ = pattern;
  is_variable_ = false;
}

void CSSValue::SetNumber(int64_t val, CSSValuePattern pattern) {
  FreeValueStorage();
  val_int64 = val;
  type_ = lynx_value_int64;
  pattern_ = pattern;
  is_variable_ = false;
}

void CSSValue::SetNumber(uint64_t val, CSSValuePattern pattern) {
  FreeValueStorage();
  val_uint64 = val;
  type_ = lynx_value_uint64;
  pattern_ = pattern;
  is_variable_ = false;
}

void CSSValue::SetString(const base::String& value, CSSValuePattern pattern) {
  FreeValueStorage();
  auto* ptr = base::String::Unsafe::GetUntaggedStringRawRef(value);
  ptr->AddRef();
  val_ptr = reinterpret_cast<lynx_value_ptr>(ptr);
  type_ = lynx_value_string;
  pattern_ = pattern;
  is_variable_ = false;
}

void CSSValue::SetString(base::String&& value, CSSValuePattern pattern) {
  FreeValueStorage();
  auto* ptr = base::String::Unsafe::GetUntaggedStringRawRef(value);
  if (ptr != base::String::Unsafe::GetStringRawRef(value)) {
    ptr->AddRef();
  }
  val_ptr = reinterpret_cast<lynx_value_ptr>(ptr);
  type_ = lynx_value_string;
  base::String::Unsafe::SetStringToEmpty(value);
  pattern_ = pattern;
  is_variable_ = false;
}

bool operator==(const CSSValue& left, const CSSValue& right) {
  if (left.pattern_ != right.pattern_) {
    return false;
  }

  if (left.IsValueStorageNumber()) {
    if (right.IsValueStorageNumber()) {
      return std::fabs(left.GetNumber() - right.GetNumber()) < 0.000001;
    } else {
      return false;
    }
  }

  if (left.type_ != right.type_) {
    return false;
  }

  switch (left.type_) {
    case lynx_value_null:
    case lynx_value_undefined:
      return true;
    case lynx_value_bool:
      return left.val_bool == right.val_bool;
    case lynx_value_nan:
      return false;  // Two NAN values are not equal? The same logic as
                     // lepus::Value.
    case lynx_value_string:
      return left.AsStdString() == right.AsStdString();
    case lynx_value_map:
      return *reinterpret_cast<lepus::Dictionary*>(left.val_ptr) ==
             *reinterpret_cast<lepus::Dictionary*>(right.val_ptr);
    case lynx_value_array:
      return *reinterpret_cast<lepus::CArray*>(left.val_ptr) ==
             *reinterpret_cast<lepus::CArray*>(right.val_ptr);
    default:
      break;
  }
  return false;
}

// Optimized structure for Tarjan's SCC algorithm
struct alignas(4) SCCNode {
  int32_t index = -1;
  int32_t low_link = -1;
  bool on_stack = false;
  uint8_t padding[3] = {0};  // Padding for cache alignment
};

class CSSValue::CycleDetector {
 public:
  explicit CycleDetector(const CustomPropertiesMap& variables)
      : variables_(variables) {
    // Process all variables when used_vars is empty
    FindAllSCCs();
  }

  bool IsInCycle(const std::string& var_name) const {
    return cycles_.contains(var_name);
  }

 private:
  void FindAllSCCs() {
    index_ = 0;
    nodes_.reserve(variables_.size());

    for (const auto& [var_name, _] : variables_) {
      const std::string& var_name_str = var_name.str();
      if (nodes_[var_name_str].index == -1) {
        StrongConnect(var_name_str);
      }
    }
  }

  void StrongConnect(const std::string& var_name) {
    SCCNode& node = nodes_[var_name];
    node.index = index_++;
    node.low_link = node.index;
    stack_.push(var_name);
    node.on_stack = true;

    // Optimized dependency traversal
    auto var_it = variables_.find(var_name);
    if (var_it != variables_.end() && var_it->second.IsVariable() &&
        var_it->second.optionals_.HasValue<VarReferenceField>()) {
      const auto& refs = var_it->second.optionals_.Get<VarReferenceField>();
      for (const auto& var_ref : refs) {
        const std::string dep_name(var_ref.Name(var_it->second.AsStdString()));

        if (variables_.find(dep_name) != variables_.end()) {
          auto& dep_node = nodes_[dep_name];
          if (dep_node.index == -1) {
            StrongConnect(dep_name);
            node.low_link = std::min(node.low_link, dep_node.low_link);
          } else if (dep_node.on_stack) {
            node.low_link = std::min(node.low_link, dep_node.index);
          }
        }
      }
    }

    // Efficient SCC extraction
    if (node.low_link == node.index) {
      bool has_cycle = false;
      std::string w;
      do {
        w = stack_.top();
        stack_.pop();
        nodes_[w].on_stack = false;

        if (w != var_name) {
          has_cycle = true;
        } else {
          // Check self-loop
          auto var_it = variables_.find(w);
          if (var_it != variables_.end() && var_it->second.IsVariable() &&
              var_it->second.optionals_.HasValue<VarReferenceField>()) {
            for (const auto& var_ref :
                 var_it->second.optionals_.Get<VarReferenceField>()) {
              if (var_ref.Name(var_it->second.AsStdString()) == w) {
                has_cycle = true;
                break;
              }
            }
          }
        }

        if (has_cycle) {
          cycles_.insert(w);
        }
      } while (w != var_name);
    }
  }

  const CustomPropertiesMap& variables_;
  base::InlineLinearFlatMap<std::string, SCCNode, 32> nodes_;
  base::InlineLinearFlatSet<std::string, 8> cycles_;
  base::InlineStack<std::string, 24> stack_;
  int index_ = 0;
};

std::string CSSValue::AsJsonString(bool map_key_ordered) const {
  return lepus::lepusValueToString(GetValue(), map_key_ordered);
}

std::string CSSValue::ResolveVariable(
    const std::string& var_name, const CustomPropertiesMap& custom_properties,
    const CycleDetector& detector, int max_depth,
    const HandleCustomPropertyFunc& handle_func) {
  if (max_depth <= 0) {
    return "";
  }

  auto value = custom_properties.find(var_name);
  if (value == custom_properties.end()) {
    return "";
  }

  const CSSValue& css_value = value->second;
  // If it's a simple string value (no variables), return it directly
  if (!css_value.IsVariable()) {
    return css_value.AsStdString();
  }

  return CSSValue::Substitution(css_value, custom_properties, detector,
                                max_depth - 1, handle_func);
  ;
}

void CSSValue::SubstituteAll(CustomPropertiesMap& custom_properties,
                             int max_depth,
                             const HandleCustomPropertyFunc& handle_func) {
  CycleDetector detector(custom_properties);
  for (auto& [name, value] : custom_properties) {
    if (value.NeedsVariableResolution()) {
      auto property = CSSValue::Substitution(value, custom_properties, detector,
                                             max_depth, handle_func);
      custom_properties[name] = CSSValue(
          std::move(property), CSSValuePattern::STRING, CSSValueType::DEFAULT);
    }
  }
}

std::string CSSValue::Substitution(
    const CSSValue& css_value, const CustomPropertiesMap& custom_properties,
    int max_depth, const HandleCustomPropertyFunc& handle_func) {
  // Build cycle detector for all variables (optimized for performance)
  CycleDetector detector(custom_properties);

  return Substitution(css_value, custom_properties, detector, max_depth,
                      handle_func);
}

std::string CSSValue::Substitution(
    const CSSValue& css_value, const CustomPropertiesMap& custom_properties,
    const CycleDetector& detector, int max_depth,
    const HandleCustomPropertyFunc& handle_func) {
  if (!css_value.IsVariable() ||
      !css_value.optionals_.HasValue<VarReferenceField>()) {
    return css_value.AsStdString();
  }

  const std::string& raw_value = css_value.AsStdString();
  const auto& var_refs = css_value.optionals_.Get<VarReferenceField>();

  if (var_refs.empty()) {
    return css_value.AsStdString();
  }

  std::string result;

  // Smart size estimation with move semantics
  size_t estimated_size = raw_value.size() << 2;
  result.reserve(std::min(estimated_size, size_t{4096}));

  size_t last_pos = 0;

  for (const auto& var_ref : var_refs) {
    // Append text before this variable reference
    result.append(raw_value, last_pos, var_ref.start - last_pos);

    const std::string dep_name(var_ref.Name(raw_value));

    auto var_it = custom_properties.find(dep_name);
    if (handle_func) {
      if (var_it != custom_properties.end()) {
        handle_func(dep_name, var_it->second.AsString());
      } else {
        handle_func(dep_name, base::String());
      }
    }

    // Fast path: check if variable exists and not in cycle
    if (var_it != custom_properties.end() && !detector.IsInCycle(dep_name)) {
      // Variable exists and is not in a cycle - resolve it normally
      std::string resolved_value = ResolveVariable(
          dep_name, custom_properties, detector, max_depth, handle_func);

      if (!resolved_value.empty()) {
        result.append(std::move(resolved_value));
      } else {
        result.append(CSSValue::ResolveFallback(
            var_ref, custom_properties, detector, max_depth, handle_func));
      }
    } else if (!var_ref.fallback.empty()) {
      // Variable not found or in cycle - use fallback
      result.append(CSSValue::ResolveFallback(
          var_ref, custom_properties, detector, max_depth, handle_func));
    }

    last_pos = var_ref.end;
  }

  // Append remaining text after last variable reference
  if (last_pos < raw_value.size()) {
    result.append(raw_value, last_pos);
  }

  // Return by move to avoid copy
  return result;
}

std::string CSSValue::SubstitutionResolved(
    const CSSValue& css_value, const CustomPropertiesMap& custom_properties,
    const HandleCustomPropertyFunc& handle_func) {
  if (!css_value.IsVariable() ||
      !css_value.optionals_.HasValue<VarReferenceField>()) {
    return css_value.AsStdString();
  }

  const std::string& raw_value = css_value.AsStdString();
  const auto& var_refs = css_value.optionals_.Get<VarReferenceField>();

  if (var_refs.empty()) {
    return css_value.AsStdString();
  }

  std::string result;

  // Smart size estimation with move semantics
  size_t estimated_size = raw_value.size() << 2;
  result.reserve(std::min(estimated_size, size_t{4096}));

  size_t last_pos = 0;

  for (const auto& var_ref : var_refs) {
    // Append text before this variable reference
    result.append(raw_value, last_pos, var_ref.start - last_pos);

    const std::string dep_name(var_ref.Name(raw_value));

    auto var_it = custom_properties.find(dep_name);
    if (handle_func) {
      if (var_it != custom_properties.end()) {
        handle_func(dep_name, var_it->second.AsString());
      } else {
        handle_func(dep_name, base::String());
      }
    }

    if (var_it != custom_properties.end()) {
      // Variable exists - resolve it normally
      // Since custom_properties are already resolved in
      // CollectCustomProperties, we can directly use the string value.
      result.append(var_it->second.AsStdString());
    } else if (!var_ref.fallback.empty()) {
      // Variable not found - use fallback
      result.append(CSSValue::ResolveFallbackResolved(
          var_ref, custom_properties, handle_func));
    }

    last_pos = var_ref.end;
  }

  // Append remaining text after last variable reference
  if (last_pos < raw_value.size()) {
    result.append(raw_value, last_pos);
  }

  // Return by move to avoid copy
  return result;
}

std::string CSSValue::ResolveFallback(
    const VarReference& ref, const CustomPropertiesMap& variable_map,
    const CycleDetector& detector, int max_depth,
    const HandleCustomPropertyFunc& handle_func) {
  if (ref.fallback.empty()) {
    return "";
  }
  CSSStringParser parser(ref.fallback.c_str(),
                         static_cast<uint32_t>(ref.fallback.length()),
                         ref.parser_configs);
  CSSValue css_value = parser.ParseVariable();
  return CSSValue::Substitution(css_value, variable_map, detector,
                                max_depth - 1, handle_func);
}

std::string CSSValue::ResolveFallbackResolved(
    const VarReference& ref, const CustomPropertiesMap& variable_map,
    const HandleCustomPropertyFunc& handle_func) {
  if (ref.fallback.empty()) {
    return "";
  }
  CSSStringParser parser(ref.fallback.c_str(),
                         static_cast<uint32_t>(ref.fallback.length()),
                         ref.parser_configs);
  CSSValue css_value = parser.ParseVariable();
  return CSSValue::SubstitutionResolved(css_value, variable_map, handle_func);
}

std::string_view VarReference::Name(const std::string& raw_value) const {
  if (name_start >= raw_value.size() || name_end > raw_value.size() ||
      name_start >= name_end) {
    return "";
  }
  return std::string_view{raw_value}.substr(start + offset + name_start,
                                            name_end - name_start);
}

bool CSSValue::ToVarReference() {
  if (!IsVariable() || optionals_.HasValue<VarReferenceField>()) {
    // Not a variable value or already a reference value.
    return false;
  }

  // First, call ReleaseTransfer to retrieve the possible default_value and
  // default_value_map, which will release the entire buffer. Then, call
  // Get<VarReferenceField>, and the newly created buffer will only store the
  // VarReference field.
  auto default_value = optionals_.ReleaseTransfer<DefaultValueField>();
  auto default_value_map = optionals_.ReleaseTransfer<DefaultValueMapField>();

  const auto& format = AsStdString();
  auto& var_references = optionals_.Get<VarReferenceField>();
  const auto* default_value_map_pointer =
      default_value_map.IsTable() ? default_value_map.Table().get() : nullptr;

  // Look for {{variable}} patterns
  size_t pos = 0;
  while (pos < format.size()) {
    // Find opening {{
    size_t open_start = format.find("{{", pos);
    if (open_start == std::string::npos) break;

    // Find closing }}
    size_t close_end = format.find("}}", open_start + 2);
    if (close_end == std::string::npos) break;

    VarReference ref;
    // For {{--color}} format, we need to set positions to work with Name()
    // method For {{--color}}, we want the variable name "--color" to be
    // extracted correctly Use offset = 2 for {{}} format (vs default 4 for
    // var() format)
    ref.start = open_start;   // Base offset (position of {{)
    ref.end = close_end + 2;  // End of string (position after }})
    ref.offset =
        2;  // Use 2 for {{}} format instead of default 4 for var() format
    ref.name_start = 0;  // Variable name starts at position 2 in the string
    ref.name_end =
        close_end - open_start - 2;  // length of variable name (excluding }})

    if (default_value_map_pointer) {
      auto name = ref.Name(format);
      if (auto it = default_value_map_pointer->find(std::string(name));
          it != default_value_map_pointer->end()) {
        ref.fallback = it->second.String();
      }
    } else if (!default_value.empty()) {
      ref.fallback = default_value;
    }
    var_references.emplace_back(std::move(ref));

    // Move past this match
    pos = close_end + 2;
  }

  needs_variable_resolution_ = true;
  return true;
}

}  // namespace tasm
}  // namespace lynx
