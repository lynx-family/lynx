// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/css_variable_handler.h"

#include <unordered_map>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

bool CSSVariableHandler::HandleCSSVariables(StyleMap& map,
                                            AttributeHolder* holder,
                                            const CSSParserConfigs& configs) {
  if (map.empty()) {
    return false;
  }

  if (!HasCSSVariableInStyleMap(map)) {
    return true;
  }
  // the CSSVariable order need to be kept.
  StyleMap style_map(CSSProperty::GetTotalParsedStyleCountFromMap(map));
  for (const auto& [id, css_value] : map) {
    if (css_value.IsVariable()) {
      const auto& value_expr = css_value.GetValue();
      if (value_expr.IsString()) {
        const std::optional<lepus::Value>& default_value_map_opt =
            css_value.GetDefaultValueMapOpt();
        auto property = GetCSSVariableByRule(
            value_expr.StdString(), holder, css_value.GetDefaultValue(),
            default_value_map_opt.has_value() ? *default_value_map_opt
                                              : lepus::Value());
        UnitHandler::Process(id, lepus::Value(std::move(property)), style_map,
                             configs);
      } else {
        UnitHandler::Process(id, lepus::Value(css_value.GetDefaultValue()),
                             style_map, configs);
      }
    } else {
      style_map[id] = css_value;
    }
  }

  map = std::move(style_map);
  return true;
}

bool CSSVariableHandler::HasCSSVariableInStyleMap(const StyleMap& map) {
  for (const auto& [_, css_value] : map) {
    if (css_value.IsVariable()) {
      return true;
    }
  }
  return false;
}

//    "The food taste {{ feeling }} !"
//    => rule: {{"feeling", "delicious"}}
//    => result: "The food taste delicious !"
base::String CSSVariableHandler::GetCSSVariableByRule(
    const std::string& format,
    base::MoveOnlyClosure<base::String, const std::string&> rule_matcher) {
  std::string variable_value;
  std::string maybe_key;
  int brace_start = -1;
  int brace_end = -1;
  int pre_brace_end = 0;
  for (int i = 0; static_cast<size_t>(i) < format.size(); ++i) {
    char c = format[i];
    switch (c) {
      case '{':
        brace_start = i;
        break;
      case '}':
        brace_end = brace_start != -1 ? i : -1;
        break;
      default:
        break;
    }
    if (brace_start != -1 && brace_end != -1) {
      variable_value.append(format, pre_brace_end,
                            brace_start - pre_brace_end - 1);
      maybe_key =
          std::string(&format[brace_start + 1], brace_end - brace_start - 1);
      base::String value = rule_matcher(maybe_key);

      // if rule_matcher finds nothings, we should just use defaultValue.
      if (value.empty()) {
        return value;
      }

      variable_value.append(value.str());
      // skip addition bracket characters
      pre_brace_end = brace_end + 2;
      brace_start = -1;
      brace_end = -1;
    }
  }
  if (static_cast<size_t>(pre_brace_end) < format.size()) {
    variable_value.append(format, pre_brace_end, format.size() - pre_brace_end);
  }
  return std::move(variable_value);
}

base::String CSSVariableHandler::GetCSSVariableByRule(
    const std::string& format, AttributeHolder* holder,
    const base::String& default_props, const lepus::Value& default_value_map) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CSSVariableHandler::GetCSSVariableByRule",
              "format", format);
  auto css_variable_value = GetCSSVariableByRule(
      format,
      [holder, self = this, &default_value_map](const std::string& maybe_key) {
        auto value = holder->GetCSSVariableValue(maybe_key);
        // If the default_value_map exists, look for possible default css var
        // values, and if we can't find them, change the css value to
        // default_props.
        if (value.empty()) {
          if (default_value_map.IsTable()) {
            auto table = default_value_map.Table().get();
            auto iter = table->find(maybe_key);
            if (iter != table->end()) {
              value = iter->second.String();
            }
          }
        }
        if (self->enable_fiber_arch_) {
          // In FiberArch, relating node with it's related css variables for
          // optimization.
          holder->AddCSSVariableRelated(maybe_key, value);
        }
        return value;
      });

  if (css_variable_value.empty()) {
    css_variable_value = default_props;
  }
  return css_variable_value;
}

}  // namespace tasm
}  // namespace lynx
