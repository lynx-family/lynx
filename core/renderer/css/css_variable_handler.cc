// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/css_variable_handler.h"

#include <utility>

#include "base/include/no_destructor.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace tasm {

bool CSSVariableHandler::HandleCSSVariables(StyleMap& map,
                                            AttributeHolder* holder,
                                            const CSSParserConfigs& configs) {
  if (map.empty()) {
    return false;
  }

  auto first_variable_iter = map.end();
  for (auto it = map.begin(); it != map.end(); ++it) {
    if (it->second.IsVariable()) {
      first_variable_iter = it;
      break;
    }
  }

  if (first_variable_iter == map.end()) {
    return false;
  }
  // the CSSVariable order need to be kept.
  StyleMap style_map;
  style_map.reserve(CSSProperty::GetTotalParsedStyleCountFromMap(map));

  for (auto it = map.begin(); it != first_variable_iter; ++it) {
    style_map[it->first] = std::move(it->second);
  }

  for (auto it = first_variable_iter; it != map.end(); ++it) {
    if (it->second.IsVariable()) {
      const auto& id = it->first;
      const auto& css_value = it->second;
      if (css_value.NeedsVariableResolution()) {
        ResolveCSSVariables(id, css_value, style_map, holder, configs);
        continue;
      }

      const auto& value_expr = css_value.GetValue();
      if (value_expr.IsString()) {
        auto default_value_map_opt = css_value.GetDefaultValueMapOpt();
        auto property = GetCSSVariableByRule(value_expr.StdString(), holder,
                                             css_value.GetDefaultValue(),
                                             default_value_map_opt);
        UnitHandler::Process(id, lepus::Value(std::move(property)), style_map,
                             configs);
      } else {
        UnitHandler::Process(id, lepus::Value(css_value.GetDefaultValue()),
                             style_map, configs);
      }
    } else {
      style_map[it->first] = std::move(it->second);
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

bool CSSVariableHandler::HasCSSVariableInHolder(const AttributeHolder* holder) {
  if (!holder) {
    return false;
  }
  return !(holder->css_variables_map().empty() &&
           holder->GetCSSInlineVariables().empty());
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CSS_HANDLER_GET_VARIABLE_BY_RULE, "format",
              format);
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

static CustomPropertiesMap* EmptyCustomPropertyMap() {
  static base::NoDestructor<CustomPropertiesMap> map;
  return map.get();
}

void CSSVariableHandler::ResolveCSSVariables(CSSPropertyID id,
                                             const CSSValue& value,
                                             StyleMap& style_map,
                                             AttributeHolder* holder,
                                             const CSSParserConfigs& configs) {
  if (!enable_fiber_arch_) {
    // Resolve CSS variables only in FiberArch.
    return;
  }
  if (!holder) {
    LOGE("ResolveCSSVariables: holder is null");
    return;
  }

  const CustomPropertiesMap* custom_properties = holder->GetCustomProperties();
  if (!custom_properties) {
    custom_properties = EmptyCustomPropertyMap();
  }

  const auto handle_custom_property_func = [holder](const base::String& name,
                                                    const base::String& value) {
    holder->AddCSSVariableRelated(name, value);
  };
  auto property = CSSValue::SubstitutionResolved(value, *custom_properties,
                                                 handle_custom_property_func);
  UnitHandler::Process(id, lepus::Value(std::move(property)), style_map,
                       configs);
}

}  // namespace tasm
}  // namespace lynx
