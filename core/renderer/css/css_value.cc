// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/css_value.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "base/include/value/table.h"
#include "base/include/vector.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/css/css_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
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
  ClearCustomPropertyDependencies();
  if (var_references.empty()) {
    optionals_.Release<VarReferenceField>();
  } else {
    optionals_.Get<VarReferenceField>() = std::move(var_references);
  }
  is_variable_ = true;
  needs_variable_resolution_ = true;
}

void CSSValue::SetCustomPropertyDependencies(
    CustomPropertyDependencyField::Type dependencies) {
  if (dependencies.empty()) {
    optionals_.Release<CustomPropertyDependencyField>();
  } else {
    optionals_.Get<CustomPropertyDependencyField>() = std::move(dependencies);
  }
}

void CSSValue::ClearCustomPropertyDependencies() {
  optionals_.Release<CustomPropertyDependencyField>();
}

void CSSValue::ReplayCustomPropertyDependencies(
    const HandleCustomPropertyFunc& handle_func) const {
  if (!handle_func || !optionals_.HasValue<CustomPropertyDependencyField>()) {
    return;
  }
  for (const auto& [name, value] :
       optionals_.Get<CustomPropertyDependencyField>()) {
    handle_func(name, value);
  }
}

void CSSValue::ReportCustomPropertyDependency(
    const std::string& name, const CSSValue* value,
    const HandleCustomPropertyFunc& handle_func) {
  if (!handle_func) {
    return;
  }
  handle_func(name, value ? value->AsString() : base::String());
  if (value) {
    value->ReplayCustomPropertyDependencies(handle_func);
  }
}

void CSSValue::SetValueAndPattern(const lepus::Value& value,
                                  CSSValuePattern pattern) {
  ClearCustomPropertyDependencies();
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
  ClearCustomPropertyDependencies();
  FreeValueStorage();
  val_ptr = reinterpret_cast<lynx_value_ptr>(array.AbandonRef());
  type_ = lynx_value_array;
  pattern_ = CSSValuePattern::ARRAY;
  is_variable_ = false;
}

void CSSValue::SetBoolean(bool value) {
  ClearCustomPropertyDependencies();
  FreeValueStorage();
  val_bool = value;
  type_ = lynx_value_bool;
  pattern_ = CSSValuePattern::BOOLEAN;
  is_variable_ = false;
}

void CSSValue::SetNumber(double val, CSSValuePattern pattern) {
  ClearCustomPropertyDependencies();
  FreeValueStorage();
  val_double = val;
  type_ = lynx_value_double;
  pattern_ = pattern;
  is_variable_ = false;
}

void CSSValue::SetNumber(int32_t val, CSSValuePattern pattern) {
  ClearCustomPropertyDependencies();
  FreeValueStorage();
  val_int32 = val;
  type_ = lynx_value_int32;
  pattern_ = pattern;
  is_variable_ = false;
}

void CSSValue::SetNumber(uint32_t val, CSSValuePattern pattern) {
  ClearCustomPropertyDependencies();
  FreeValueStorage();
  val_uint32 = val;
  type_ = lynx_value_uint32;
  pattern_ = pattern;
  is_variable_ = false;
}

void CSSValue::SetNumber(int64_t val, CSSValuePattern pattern) {
  ClearCustomPropertyDependencies();
  FreeValueStorage();
  val_int64 = val;
  type_ = lynx_value_int64;
  pattern_ = pattern;
  is_variable_ = false;
}

void CSSValue::SetNumber(uint64_t val, CSSValuePattern pattern) {
  ClearCustomPropertyDependencies();
  FreeValueStorage();
  val_uint64 = val;
  type_ = lynx_value_uint64;
  pattern_ = pattern;
  is_variable_ = false;
}

void CSSValue::SetString(const base::String& value, CSSValuePattern pattern) {
  ClearCustomPropertyDependencies();
  FreeValueStorage();
  auto* ptr = base::String::Unsafe::GetUntaggedStringRawRef(value);
  ptr->AddRef();
  val_ptr = reinterpret_cast<lynx_value_ptr>(ptr);
  type_ = lynx_value_string;
  pattern_ = pattern;
  is_variable_ = false;
}

void CSSValue::SetString(base::String&& value, CSSValuePattern pattern) {
  ClearCustomPropertyDependencies();
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
  explicit CycleDetector(const CustomPropertiesMap& variables,
                         bool detect_cycles = true)
      : variables_(variables) {
    if (detect_cycles) {
      FindAllSCCs();
    }
  }

  bool IsInCycle(const std::string& var_name) const {
    return cycles_.contains(var_name);
  }

  void ReuseCycles(const std::unordered_set<base::String>& cycles) {
    for (const auto& name : cycles) {
      cycles_.insert(name.str());
    }
  }

  template <typename Visitor>
  static void ForEachDependency(const CSSValue& value, const Visitor& visitor) {
    if (!value.IsVariable() ||
        !value.optionals_.HasValue<VarReferenceField>()) {
      return;
    }
    for (const auto& ref : value.optionals_.Get<VarReferenceField>()) {
      visitor(std::string(ref.Name(value.AsStdString())));
      // Fallback references participate in cycles even when the primary
      // reference exists and the fallback would not be substituted.
      if (!ref.fallback.empty()) {
        CSSStringParser parser(ref.fallback.c_str(),
                               static_cast<uint32_t>(ref.fallback.length()),
                               ref.parser_configs);
        ForEachDependency(parser.ParseVariable(), visitor);
      }
    }
  }

 private:
  void FindAllSCCs() {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, CSS_VALUE_FIND_ALL_SCCS);
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

    bool has_self_loop = false;
    auto var_it = variables_.find(var_name);
    if (var_it != variables_.end()) {
      ForEachDependency(var_it->second, [&](const std::string& dep_name) {
        has_self_loop |= dep_name == var_name;
        if (variables_.find(dep_name) != variables_.end()) {
          auto& dep_node = nodes_[dep_name];
          if (dep_node.index == -1) {
            StrongConnect(dep_name);
            node.low_link = std::min(node.low_link, dep_node.low_link);
          } else if (dep_node.on_stack) {
            node.low_link = std::min(node.low_link, dep_node.index);
          }
        }
      });
    }

    // Efficient SCC extraction
    if (node.low_link == node.index) {
      bool has_cycle = false;
      std::string w;
      do {
        w = stack_.top();
        stack_.pop();
        nodes_[w].on_stack = false;

        if (w != var_name || has_self_loop) {
          has_cycle = true;
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

class CSSValue::CustomPropertyResolutionCache {
 public:
  struct Graph {
    std::unordered_map<base::String, std::vector<base::String>> dependencies;
    std::unordered_set<base::String> cycles;
  };

  CustomPropertiesMapRef specified;
  CustomPropertiesMapRef resolved;
  std::shared_ptr<const Graph> graph;
  std::unordered_map<base::String, std::vector<base::String>> used_references;
};

bool CSSValue::HasSameVariableResolutionInput(const CSSValue& other) const {
  if (*this != other || IsVariable() != other.IsVariable() ||
      NeedsVariableResolution() != other.NeedsVariableResolution()) {
    return false;
  }
  if (!IsVariable()) {
    return true;
  }
  // Legacy references can have identical text and different fallback metadata.
  if (GetDefaultValue() != other.GetDefaultValue() ||
      GetDefaultValueMapOpt() != other.GetDefaultValueMapOpt()) {
    return false;
  }
  const auto* references = GetVarReferences();
  const auto* other_references = other.GetVarReferences();
  if (!references || !other_references) {
    return references == other_references;
  }
  return references->size() == other_references->size() &&
         std::equal(references->begin(), references->end(),
                    other_references->begin(),
                    [](const auto& a, const auto& b) {
                      return a.name_start == b.name_start &&
                             a.name_end == b.name_end && a.start == b.start &&
                             a.end == b.end && a.offset == b.offset &&
                             a.fallback == b.fallback;
                    });
}

void CSSValue::ResolveCustomProperties(
    const CustomPropertiesMapRef& specified, CustomPropertiesMapRef& resolved,
    CustomPropertyResolutionCacheRef& cache) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CSS_VALUE_RESOLVE_CUSTOM_PROPERTIES);
  // The two arguments may alias in the legacy element path.
  const auto specified_snapshot = specified;
  const auto& inputs = specified_snapshot.Get()->Value();
  const auto previous = cache;
  const auto same_value = [](const CSSValue& a, const CSSValue& b) {
    return a.HasSameVariableResolutionInput(b);
  };
  const auto same_dependencies = [](const CSSValue& a, const CSSValue& b) {
    const bool a_has = a.optionals_.HasValue<CustomPropertyDependencyField>();
    const bool b_has = b.optionals_.HasValue<CustomPropertyDependencyField>();
    return a_has == b_has &&
           (!a_has || a.optionals_.Get<CustomPropertyDependencyField>() ==
                          b.optionals_.Get<CustomPropertyDependencyField>());
  };

  const CustomPropertiesMap* old_inputs =
      previous ? &previous->specified.Get()->Value() : nullptr;
  bool same_inputs = old_inputs && old_inputs->size() == inputs.size();
  bool same_graph_inputs = previous != nullptr;
  size_t pending_count = 0;
  for (const auto& [name, value] : inputs) {
    const auto old = old_inputs ? old_inputs->find(name) : inputs.end();
    const bool same = old_inputs && old != old_inputs->end() &&
                      same_value(value, old->second);
    same_inputs &= same && same_dependencies(value, old->second);
    if (value.NeedsVariableResolution()) {
      ++pending_count;
      same_graph_inputs &= same;
    }
  }
  if (same_inputs) {
    resolved = previous->resolved;
    return;
  }
  if (pending_count == 0) {
    resolved = specified_snapshot;
    cache.reset();
    return;
  }

  same_graph_inputs &=
      previous && pending_count == previous->graph->dependencies.size();
  std::shared_ptr<const CustomPropertyResolutionCache::Graph> graph;
  std::shared_ptr<CustomPropertyResolutionCache::Graph> new_graph;
  if (same_graph_inputs) {
    graph = previous->graph;
  } else {
    new_graph = std::make_shared<CustomPropertyResolutionCache::Graph>();
    for (const auto& [name, value] : inputs) {
      if (!value.NeedsVariableResolution()) {
        continue;
      }
      auto& references = new_graph->dependencies[name];
      CycleDetector::ForEachDependency(value,
                                       [&](const std::string& dependency) {
                                         references.emplace_back(dependency);
                                       });
      std::sort(references.begin(), references.end());
      references.erase(std::unique(references.begin(), references.end()),
                       references.end());
    }
    if (previous && previous->graph->dependencies == new_graph->dependencies) {
      graph = previous->graph;
      new_graph.reset();
    } else {
      graph = new_graph;
    }
  }

  // Inherited computed values are leaves. Only changes to the local reference
  // graph can change its SCCs; changing terminal values does not rebuild it.
  CustomPropertiesMap cycle_inputs;
  if (new_graph) {
    for (const auto& [name, _] : graph->dependencies) {
      cycle_inputs.insert_or_assign(name, inputs.find(name)->second);
    }
  }
  CycleDetector detector(cycle_inputs, new_graph != nullptr);
  if (new_graph) {
    for (const auto& [name, _] : graph->dependencies) {
      if (detector.IsInCycle(name.str())) {
        new_graph->cycles.insert(name);
      }
    }
  } else {
    detector.ReuseCycles(graph->cycles);
  }

  resolved = specified_snapshot;
  auto& values = resolved.Access()->Value();
  const auto* old_values =
      previous ? &previous->resolved.Get()->Value() : nullptr;
  std::unordered_map<base::String, std::vector<base::String>> used_references;
  std::unordered_set<base::String> finished;
  std::vector<base::String> resolution_order;
  for (const auto& name : graph->cycles) {
    values.insert_or_assign(name,
                            CSSValue(base::String(), CSSValuePattern::STRING,
                                     CSSValueType::DEFAULT));
    finished.insert(name);
    resolution_order.push_back(name);
  }
  auto resolve_value = [&](auto&& self, const base::String& name) -> void {
    if (finished.count(name)) {
      return;
    }
    const auto& references = graph->dependencies.at(name);
    for (const auto& reference : references) {
      if (graph->dependencies.count(reference)) {
        self(self, reference);
      }
    }
    bool reuse =
        old_inputs && old_inputs->contains(name) &&
        same_value(inputs.find(name)->second, old_inputs->find(name)->second) &&
        !previous->graph->cycles.count(name);
    const auto* previous_references =
        reuse ? &previous->used_references.at(name) : nullptr;
    for (const auto& reference :
         previous_references ? *previous_references : references) {
      if (!reuse) {
        break;
      }
      auto current = values.find(reference);
      auto old = old_values->find(reference);
      if ((current == values.end()) != (old == old_values->end()) ||
          (current != values.end() &&
           !same_value(current->second, old->second))) {
        reuse = false;
      }
    }
    if (reuse) {
      values.insert_or_assign(name, old_values->find(name)->second);
      used_references[name] = *previous_references;
    } else {
      auto& used = used_references[name];
      const auto record_reference = [&used](const base::String& reference) {
        used.push_back(reference);
      };
      auto result = Substitution(inputs.find(name)->second, values, detector,
                                 10, nullptr, record_reference);
      values.insert_or_assign(
          name, CSSValue(std::move(result), CSSValuePattern::STRING,
                         CSSValueType::DEFAULT));
    }
    finished.insert(name);
    resolution_order.push_back(name);
  };
  for (const auto& [name, _] : graph->dependencies) {
    resolve_value(resolve_value, name);
  }

  // Refresh dependency metadata even when a computed string was reused. A
  // dependency can start referring to another variable without changing value.
  for (const auto& name : resolution_order) {
    CustomPropertyDependencyField::Type dependencies;
    std::unordered_set<base::String> visited;
    const auto record = [&dependencies](const base::String& dependency,
                                        const base::String& value) {
      dependencies.insert_or_assign(dependency, value);
    };
    if (!graph->cycles.count(name)) {
      for (const auto& reference : used_references.at(name)) {
        auto value = values.find(reference);
        ReportCustomPropertyDependency(
            reference.str(), value == values.end() ? nullptr : &value->second,
            record);
      }
    }
    const auto record_extra = [&dependencies](const base::String& dependency,
                                              const base::String& value) {
      if (!dependencies.contains(dependency)) {
        dependencies.insert_or_assign(dependency, value);
      }
    };
    auto collect = [&](auto&& self, const base::String& current) -> void {
      for (const auto& dependency : graph->dependencies.at(current)) {
        if (!visited.insert(dependency).second) {
          continue;
        }
        auto value = values.find(dependency);
        if (graph->dependencies.count(dependency)) {
          record_extra(dependency, value->second.AsString());
          self(self, dependency);
        } else {
          ReportCustomPropertyDependency(
              dependency.str(),
              value == values.end() ? nullptr : &value->second, record_extra);
        }
      }
    };
    collect(collect, name);
    auto& value = values.at(name);
    if (!value.optionals_.HasValue<CustomPropertyDependencyField>() ||
        value.optionals_.Get<CustomPropertyDependencyField>() != dependencies) {
      value.SetCustomPropertyDependencies(std::move(dependencies));
    }
  }

  auto next = std::make_shared<CustomPropertyResolutionCache>();
  next->specified = specified_snapshot;
  next->resolved = resolved;
  next->graph = std::move(graph);
  next->used_references = std::move(used_references);
  cache = std::move(next);
}

std::string CSSValue::AsJsonString(bool map_key_ordered) const {
  return lepus::lepusValueToString(GetValue(), map_key_ordered);
}

std::string CSSValue::ResolveVariable(
    const std::string& var_name, const CustomPropertiesMap& custom_properties,
    const CycleDetector& detector, int max_depth,
    const HandleCustomPropertyFunc& handle_func,
    const HandleVariableReferenceFunc& on_reference) {
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
                                max_depth - 1, handle_func, on_reference);
  ;
}

void CSSValue::SubstituteAll(CustomPropertiesMap& custom_properties,
                             int max_depth,
                             const HandleCustomPropertyFunc& handle_func) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CSS_VALUE_SUBSTITUTE_ALL);
  auto first_unresolved = std::find_if(
      custom_properties.begin(), custom_properties.end(),
      [](const auto& entry) { return entry.second.NeedsVariableResolution(); });
  if (first_unresolved == custom_properties.end()) {
    return;
  }

  CycleDetector detector(custom_properties);
  for (auto it = first_unresolved; it != custom_properties.end(); ++it) {
    auto& entry = *it;
    auto& value = entry.second;
    if (value.NeedsVariableResolution()) {
      CustomPropertyDependencyField::Type dependencies;
      HandleCustomPropertyFunc collecting_handle_func =
          [&dependencies, &handle_func](const base::String& dependency_name,
                                        const base::String& dependency_value) {
            dependencies.insert_or_assign(dependency_name, dependency_value);
            if (handle_func) {
              handle_func(dependency_name, dependency_value);
            }
          };
      std::string property =
          CSSValue::Substitution(value, custom_properties, detector, max_depth,
                                 collecting_handle_func);
      if (detector.IsInCycle(entry.first.str())) {
        // A cyclic declaration cannot become valid through its own fallback.
        property.clear();
      }
      CSSValue resolved_value(std::move(property), CSSValuePattern::STRING,
                              CSSValueType::DEFAULT);
      resolved_value.SetCustomPropertyDependencies(std::move(dependencies));
      value = std::move(resolved_value);
    }
  }
}

std::string CSSValue::Substitution(
    const CSSValue& css_value, const CustomPropertiesMap& custom_properties,
    int max_depth, const HandleCustomPropertyFunc& handle_func) {
  const bool has_pending_references = std::any_of(
      custom_properties.begin(), custom_properties.end(),
      [](const auto& entry) { return entry.second.NeedsVariableResolution(); });
  CycleDetector detector(custom_properties, has_pending_references);

  return Substitution(css_value, custom_properties, detector, max_depth,
                      handle_func);
}

namespace {
inline std::string StripImportantFromString(std::string value) {
  const char* stripped_start;
  uint32_t stripped_len;
  if (tasm::StripImportant(value.c_str(), static_cast<uint32_t>(value.length()),
                           &stripped_start, &stripped_len)) {
    value.resize(stripped_len);
  }
  return value;
}
}  // namespace

std::string CSSValue::Substitution(
    const CSSValue& css_value, const CustomPropertiesMap& custom_properties,
    const CycleDetector& detector, int max_depth,
    const HandleCustomPropertyFunc& handle_func,
    const HandleVariableReferenceFunc& on_reference) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CSS_VALUE_SUBSTITUTION);
  if (css_value.IsVariable() &&
      !css_value.optionals_.HasValue<VarReferenceField>()) {
    CSSValue normalized_value(css_value);
    if (normalized_value.ToVarReference()) {
      return Substitution(normalized_value, custom_properties, detector,
                          max_depth, handle_func, on_reference);
    }
  }

  if (!css_value.IsVariable() ||
      !css_value.optionals_.HasValue<VarReferenceField>()) {
    return css_value.AsStdString();
  }

  const std::string& raw_value = css_value.AsStdString();
  const auto& var_refs = css_value.optionals_.Get<VarReferenceField>();

  if (var_refs.empty()) {
    return raw_value;
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
    if (on_reference) {
      on_reference(var_it != custom_properties.end() ? var_it->first
                                                     : base::String(dep_name));
    }
    ReportCustomPropertyDependency(
        dep_name, var_it != custom_properties.end() ? &var_it->second : nullptr,
        handle_func);

    // Fast path: check if variable exists and not in cycle
    if (var_it != custom_properties.end() && !detector.IsInCycle(dep_name)) {
      // Variable exists and is not in a cycle - resolve it normally
      std::string resolved_value =
          ResolveVariable(dep_name, custom_properties, detector, max_depth,
                          handle_func, on_reference);

      if (!resolved_value.empty()) {
        result.append(std::move(resolved_value));
      } else {
        result.append(CSSValue::ResolveFallback(var_ref, custom_properties,
                                                detector, max_depth,
                                                handle_func, on_reference));
      }
    } else if (!var_ref.fallback.empty()) {
      // Variable not found or in cycle - use fallback
      result.append(CSSValue::ResolveFallback(var_ref, custom_properties,
                                              detector, max_depth, handle_func,
                                              on_reference));
    }

    last_pos = var_ref.end;
  }

  // Append remaining text after last variable reference
  if (last_pos < raw_value.size()) {
    result.append(raw_value, last_pos);
  }

  return StripImportantFromString(std::move(result));
}

std::string CSSValue::SubstitutionResolved(
    const CSSValue& css_value, const CustomPropertiesMap& custom_properties,
    const HandleCustomPropertyFunc& handle_func) {
  return SubstitutionResolved(css_value, custom_properties, handle_func,
                              nullptr);
}

std::string CSSValue::SubstitutionResolved(
    const CSSValue& css_value, const CustomPropertiesMap& custom_properties,
    const HandleCustomPropertyFunc& handle_func,
    const HandleVariableReferenceFunc& on_reference) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CSS_VALUE_SUBSTITUTION_RESOLVED);
  if (css_value.IsVariable() &&
      !css_value.optionals_.HasValue<VarReferenceField>()) {
    CSSValue normalized_value(css_value);
    if (normalized_value.ToVarReference()) {
      return SubstitutionResolved(normalized_value, custom_properties,
                                  handle_func, on_reference);
    }
  }

  if (!css_value.IsVariable() ||
      !css_value.optionals_.HasValue<VarReferenceField>()) {
    return css_value.AsStdString();
  }

  const std::string& raw_value = css_value.AsStdString();
  const auto& var_refs = css_value.optionals_.Get<VarReferenceField>();

  if (var_refs.empty()) {
    return raw_value;
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
    if (on_reference) {
      on_reference(var_it != custom_properties.end() ? var_it->first
                                                     : base::String(dep_name));
    }
    ReportCustomPropertyDependency(
        dep_name, var_it != custom_properties.end() ? &var_it->second : nullptr,
        handle_func);

    if (var_it != custom_properties.end()) {
      // Variable exists - resolve it normally
      // Since custom_properties are already resolved in
      // CollectCustomProperties, we can directly use the string value.
      result.append(var_it->second.AsStdString());
    } else if (!var_ref.fallback.empty()) {
      // Variable not found - use fallback
      result.append(CSSValue::ResolveFallbackResolved(
          var_ref, custom_properties, handle_func, on_reference));
    }

    last_pos = var_ref.end;
  }

  // Append remaining text after last variable reference
  if (last_pos < raw_value.size()) {
    result.append(raw_value, last_pos);
  }

  return StripImportantFromString(std::move(result));
}

std::string CSSValue::ResolveFallback(
    const VarReference& ref, const CustomPropertiesMap& variable_map,
    const CycleDetector& detector, int max_depth,
    const HandleCustomPropertyFunc& handle_func,
    const HandleVariableReferenceFunc& on_reference) {
  if (ref.fallback.empty()) {
    return "";
  }
  CSSStringParser parser(ref.fallback.c_str(),
                         static_cast<uint32_t>(ref.fallback.length()),
                         ref.parser_configs);
  CSSValue css_value = parser.ParseVariable();
  return CSSValue::Substitution(css_value, variable_map, detector,
                                max_depth - 1, handle_func, on_reference);
}

std::string CSSValue::ResolveFallbackResolved(
    const VarReference& ref, const CustomPropertiesMap& variable_map,
    const HandleCustomPropertyFunc& handle_func,
    const HandleVariableReferenceFunc& on_reference) {
  if (ref.fallback.empty()) {
    return "";
  }
  CSSStringParser parser(ref.fallback.c_str(),
                         static_cast<uint32_t>(ref.fallback.length()),
                         ref.parser_configs);
  CSSValue css_value = parser.ParseVariable();
  return CSSValue::SubstitutionResolved(css_value, variable_map, handle_func,
                                        on_reference);
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
