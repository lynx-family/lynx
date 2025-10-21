// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/css_value.h"

#include <algorithm>

#include "base/include/vector.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/runtime/vm/lepus/json_parser.h"

namespace lynx {
namespace tasm {

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
        var_it->second.var_references_) {
      const auto& refs = *var_it->second.var_references_;
      for (const auto& var_ref : refs) {
        const std::string dep_name(var_ref.Name(var_it->second.AsString()));

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
              var_it->second.var_references_) {
            for (const auto& var_ref : *var_it->second.var_references_) {
              if (var_ref.Name(var_it->second.AsString()) == w) {
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
  return lepus::lepusValueToString(value_, map_key_ordered);
}

bool CSSValue::AsBool() const { return value_.Bool(); }

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
    return css_value.AsString();
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
      custom_properties[name] = CSSValue(lepus::Value(std::move(property)));
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
  if (!css_value.IsVariable() || !css_value.var_references_) {
    return css_value.AsString();
  }

  const std::string& raw_value = css_value.AsString();
  const auto& var_refs = *css_value.var_references_;

  if (var_refs.empty()) {
    return css_value.AsString();
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

std::string_view VarReference::Name(const std::string& raw_value) const {
  if (name_start >= raw_value.size() || name_end > raw_value.size() ||
      name_start >= name_end) {
    return "";
  }
  constexpr size_t kVarParamOffset = 4;
  return std::string_view(raw_value).substr(
      start + kVarParamOffset + name_start, name_end - name_start);
}

}  // namespace tasm
}  // namespace lynx
