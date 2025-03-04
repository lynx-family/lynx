// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/generator/source_generator.h"

#include <algorithm>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/sorted_for_each.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/component_attributes.h"
#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/parser.h"
#include "core/runtime/vm/lepus/visitor.h"
#include "core/template_bundle/template_codec/generator/template_dynamic_component_parser.h"
#include "core/template_bundle/template_codec/generator/template_page_parser.h"
#include "core/template_bundle/template_codec/generator/template_scope.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace tasm {

static constexpr char kContextPrefix[] = "_context";

std::unique_ptr<SourceGenerator> SourceGenerator::GenerateParser(
    const EncoderOptions &encoder_options) {
  switch (encoder_options.generator_options_.instance_type_) {
    case PackageInstanceType::CARD:
      return std::make_unique<TemplatePageParser>(encoder_options);
    case PackageInstanceType::DYNAMIC_COMPONENT:
      return std::make_unique<TemplateDynamicComponentParser>(encoder_options);
    default:
      return nullptr;
  }
}

SourceGenerator::SourceGenerator(
    const std::string &json, const rapidjson::Value &worklet,
    const rapidjson::Value &script_map, const rapidjson::Value &packed_script,
    PackageInstanceType type, PackageInstanceDSL dsl,
    const CompileOptions &compile_options, const lepus::Value &trial_options,
    const SourceGeneratorOptions &generator_options, bool single_page,
    PackageInstanceBundleModuleMode bundle_module_mode,
    const std::string &lepus_js_code)
    : json_(json),
      lepus_js_code_(lepus_js_code),
      current_page_(nullptr),
      current_dynamic_component_(nullptr),
      current_component_(nullptr),
      current_template_(nullptr),
      template_helper_(new TemplateHelper),
      dsl_(dsl),
      compile_options_(compile_options),
      generator_options_(generator_options),
      is_air_strict_(compile_options.lynx_air_mode_ ==
                     CompileOptionAirMode::AIR_MODE_STRICT),
      is_single_page_(single_page) {
  if (type == PackageInstanceType::CARD) {
    package_instance_ =
        std::make_unique<App>(json, trial_options, worklet, script_map,
                              packed_script, dsl, bundle_module_mode);
  } else {
    package_instance_ =
        std::make_unique<HotSwapApp>(json, trial_options, worklet, script_map,
                                     packed_script, dsl, bundle_module_mode);
  }
}

std::string SourceGenerator::lepusRuntime() {
  rapidjson::Document document;
  const char *lepusRuntime = "lepusRuntime";
  document.Parse(json_);
  if (document.HasMember(lepusRuntime)) {
    return document[lepusRuntime].GetString();
  } else {
    return "";
  }
}

//
// Format string to code and erase brace. For example:
//
//    " a a " => "' a a '"
//    " a {{b}} a " => "' a ' + b + ' a '"
//
std::string SourceGenerator::FormatValue(std::string str) {
  if (str.empty()) {
    return "\"\"";
  }

  std::stringstream ss;
  const char *byte_array = str.c_str();
  bool is_id = false;
  bool is_string = false;
  int barces_pair = 0;
  for (int i = 0; i < str.size(); ++i) {
    if (byte_array[i] == '{') {
      if (!is_id) {
        if (is_string) {
          ss << '\"';
        }
        if (i != 0) {
          ss << '+';
        }
        if (!is_id) {
          ss << '(';
        }
        is_id = true;
        is_string = false;
      }
      barces_pair++;
      if (barces_pair > 2) {
        ss << '{';
      }
      continue;
    } else if (byte_array[i] == '}') {
      barces_pair--;
      if (barces_pair >= 2) {
        ss << '}';
      }
      if (barces_pair != 0) {
        continue;
      }
      if (is_id) {
        ss << ')';
      }
      is_id = false;
      continue;
    } else if (!is_id) {
      if (!is_string) {
        if (i != 0) {
          ss << '+';
        }
        is_string = true;
        is_id = false;
        ss << '\"';
      }
    }
    if (!is_id && byte_array[i] == '"') {
      ss << "\\\"";
    } else if (!is_id && byte_array[i] == '\\' && i < str.size() - 1 &&
               byte_array[i + 1] == '"') {
      continue;
    } else {
      ss << byte_array[i];
    }
    if (is_string && i == str.size() - 1) {
      ss << '\"';
    }
  }
  return ss.str();
}

std::string SourceGenerator::FormatCSSPropertyID(std::string name) {
  if (enable_css_property_id_optimization_) {
    CSSPropertyID id = CSSProperty::GetPropertyID(name);
    if (CSSProperty::IsPropertyValid(id)) {
      std::stringstream result;
      result << (int)id;
      return result.str();
    }
    if (!IsVariableString(name)) {
      std::stringstream error;
      error << "Error In TemplateParse: \"" << name
            << "\" is not supported now !";
      throw lynx::lepus::EncodeException(error.str().c_str());
    }
  }
  return FormatValue(name);
}

//
// Use the following rules to format strings
//
//    "The food taste {feeling} !"
//      => rule: {{"feeling", "delicious"}}
//      => result: "The food taste delicious !"
//
// Notice: Nesting brace is not supported as a key in rule
//
std::string SourceGenerator::FormatStringWithRule(
    const std::string &format,
    const std::unordered_map<std::string, std::string> &rule) {
  std::stringstream ss;
  std::string maybe_key;
  int brace_start = -1;
  int brace_end = -1;
  int pre_brace_end = 0;
  for (int i = 0; i < format.size(); ++i) {
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
      ss.write(&format[pre_brace_end], brace_start - pre_brace_end);
      maybe_key =
          std::string(&format[brace_start + 1], brace_end - brace_start - 1);
      auto it = rule.find(maybe_key);
      if (it != rule.end() && !maybe_key.empty()) {
        ss << it->second;
      } else {
        ss.write(&format[brace_start], brace_end - brace_start + 1);
      }
      pre_brace_end = brace_end + 1;
      brace_start = -1;
      brace_end = -1;
    }
  }
  if (pre_brace_end < format.size()) {
    ss.write(&format[pre_brace_end], format.size() - pre_brace_end);
  }
  return ss.str();
}

int32_t SourceGenerator::GetCSSForComponent(Component *component) {
  std::stringstream ttss_path;
  ttss_path << component->path() << kTTSSResourceSuffix;
  auto it = ttss_ids_.find(ttss_path.str());
  if (it != ttss_ids_.end()) {
    return it->second;
  } else {
    return -1;
  }
}

// static
bool SourceGenerator::JsonArrayContains(const rapidjson::Value &haystack,
                                        const std::string &needle) {
  auto type = haystack.GetType();
  if (type != rapidjson::Type::kArrayType) {
    return false;
  }

  for (rapidjson::SizeType i = 0; i < haystack.Size(); i++) {
    if (haystack[i].GetType() == rapidjson::Type::kStringType &&
        haystack[i].GetString() == needle) {
      return true;
    }
  }
  return false;
}

//
// Check if a string contains variable.
//
//    "aa"            =>    raw string
//    "{{aa}}"        =>    variable
//    "a {{aa}} a "   =>    variable
//
bool SourceGenerator::IsVariableString(const std::string &str) {
  return str.find("{{") != std::string::npos;
}

// Check if a string has no variable.
bool SourceGenerator::IsNonVariableString(const std::string &str) {
  return (str.find("{{") == 0);
}

std::string SourceGenerator::GetLepusCode() const {
  // if result empty, return lepus_js_code_ directly.
  if (result().empty()) {
    return lepus_js_code_;
  }

  std::string lepus_code;
  for (auto &item : result()) {
    lepus_code += item.second;
  }
  return lepus_code;
}

std::string SourceGenerator::DumpJSON(const rapidjson::Value &element) {
  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
  element.Accept(writer);
  return sb.GetString();
}

bool SourceGenerator::IsSlot(const rapidjson::Value &element) {
  if (element["slot"].IsObject() &&
      VALUE(element["slot"]).GetStringLength() > 0) {
    return true;
  }
  return false;
}

bool SourceGenerator::IsComponentAttrs(const std::string &name) {
  std::string name_lower = name;
  std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                 ::tolower);
  return ComponentAttributes::GetAttrNames().find(name_lower) !=
         ComponentAttributes::GetAttrNames().end();
}

// An visitor to mark variable that used in generated source
class TopVariableFinder : public lepus::Visitor {
 public:
  TopVariableFinder(VariableUsageRecorder *recorder)
      : usage_recorder_(recorder) {}
  ~TopVariableFinder() {}
  void Visit(lepus::ChunkAST *ast, void *data) {
    ast->block()->Accept(this, nullptr);
  }
  void Visit(lepus::BlockAST *ast, void *data) {
    for (lepus::ASTreeVector::iterator iter = ast->statements().begin();
         iter != ast->statements().end(); ++iter) {
      (*iter)->Accept(this, nullptr);
    }
    if (ast->return_statement()) {
      ast->return_statement()->Accept(this, nullptr);
    }
  }

  void Visit(lepus::BlockAST *ast, void *data, bool gen) { Visit(ast, data); }

  void Visit(lepus::CatchBlockAST *ast, void *data) {
    if (ast->catch_identifier()) {
      ast->catch_identifier()->Accept(this, nullptr);
    }
    ast->block()->Accept(this, nullptr);
  }
  void Visit(lepus::TryCatchFinallyStatementAST *ast, void *data) {
    ast->try_block()->Accept(this, nullptr);
    if (ast->catch_block()) {
      ast->catch_block()->Accept(this, nullptr);
    }
    if (ast->finally_block()) {
      ast->finally_block()->Accept(this, nullptr);
    }
  }
  void Visit(lepus::ReturnStatementAST *ast, void *data) {
    ast->expression()->Accept(this, nullptr);
  }
  void Visit(lepus::LiteralAST *ast, void *data) {
    if (ast->token().token_ == lepus::Token_Id && ast->token().IsString()) {
      usage_recorder_->MarkVariableInUse(ast->token().str_);
    }
  }
  void Visit(lepus::ThrowStatementAST *ast, void *data) {
    if (ast->throw_identifier()) {
      ast->throw_identifier()->Accept(this, nullptr);
    }
  }
  void Visit(lepus::NamesAST *ast, void *data) {}
  void Visit(lepus::BinaryExprAST *ast, void *data) {
    ast->left()->Accept(this, nullptr);
    ast->right()->Accept(this, nullptr);
  }
  void Visit(lepus::UnaryExpression *ast, void *data) {
    ast->expression()->Accept(this, nullptr);
  }
  void Visit(lepus::ExpressionListAST *ast, void *data) {
    for (lepus::ASTreeVector::iterator iter = ast->expressions().begin();
         iter != ast->expressions().end(); ++iter) {
      (*iter)->Accept(this, nullptr);
    }
  }
  void Visit(lepus::VariableAST *ast, void *data) {
    ast->expression()->Accept(this, nullptr);
  }
  void Visit(lepus::CatchVariableAST *ast, void *data) {
    ast->expression()->Accept(this, nullptr);
  }
  void Visit(lepus::VariableListAST *ast, void *data) {}
  void Visit(lepus::FunctionStatementAST *ast, void *data) {}
  void Visit(lepus::ForStatementAST *ast, void *data) {
    ast->statement1()->Accept(this, nullptr);
    ast->statement2()->Accept(this, nullptr);
  }
  void Visit(lepus::DoWhileStatementAST *ast, void *data) {}
  void Visit(lepus::BreakStatementAST *ast, void *data) {}
  void Visit(lepus::ContinueStatementAST *ast, void *data) {}
  void Visit(lepus::WhileStatementAST *ast, void *data) {}
  void Visit(lepus::IfStatementAST *ast, void *data) {
    ast->condition()->Accept(this, nullptr);
  }
  void Visit(lepus::ElseStatementAST *ast, void *data) {}
  void Visit(lepus::CaseStatementAST *ast, void *data) {}
  void Visit(lepus::AssignStatement *ast, void *data) {
    ast->variable()->Accept(this, nullptr);
    ast->expression()->Accept(this, nullptr);
  }
  void Visit(lepus::MemberAccessorAST *ast, void *data) {
    ast->table()->Accept(this, nullptr);
    ast->member()->Accept(this, nullptr);
  }
  void Visit(lepus::FunctionCallAST *ast, void *data) {
    ast->args()->Accept(this, nullptr);
  }
  void Visit(lepus::TernaryStatementAST *ast, void *data) {
    ast->condition()->Accept(this, nullptr);
    ast->false_branch()->Accept(this, nullptr);
    ast->true_branch()->Accept(this, nullptr);
  }
  void Visit(lepus::ObjectLiteralAST *ast, void *data) {
    base::SortedForEach(ast->property(), [this](const auto &it) {
      it.second->Accept(this, nullptr);
    });
  }
  void Visit(lepus::ArrayLiteralAST *ast, void *data) {
    ast->values()->Accept(this, nullptr);
  }

 private:
  VariableUsageRecorder *usage_recorder_;
};

static inline bool PositionInRanges(
    int pos, const std::vector<std::pair<int, int>> &ranges) {
  for (auto range : ranges) {
    if (pos >= range.first && pos < range.second) return true;
  }
  return false;
}

std::string SourceGenerator::FormatThemedStatement(const std::string &str,
                                                   std::string::size_type pos) {
  std::vector<std::pair<int, int>> ranges;
  int s = 0, e = 0;
  while ((s = (int)str.find("{{", e)) >= 0 && (e = str.find("}}", s + 2)) > 0) {
    ranges.push_back({s, e});
  }
  if (!PositionInRanges(pos, ranges)) {
    return str;
  }

  std::stringstream ss;
  ss << str.substr(0, pos);
  const int len = str.size();
  const char *cStr = str.c_str();
  while (pos < len) {
    int kwdLen = 0;
    bool addSep = false;
    const char *curStr = cStr + pos;
    switch (*curStr) {
      case 'T':
        if (strncmp(curStr, "Theme(", 6) == 0) {
          kwdLen = 6;
          addSep = true;
        }
        break;
      case 'L':
        if (strncmp(curStr, "Lang(", 5) == 0) {
          kwdLen = 5;
          addSep = true;
        }
        break;
      default:
        break;
    }
    if (kwdLen > 0) {
      ss << str.substr(pos, kwdLen);
      ss << "$kTemplateAssembler";
      if (addSep) ss << ",";
      pos += kwdLen;
    } else {
      ss << *curStr;
      ++pos;
    }
    auto newPos = std::string::npos;
    while (true) {
      newPos = str.find("_sys", pos);
      if (newPos == std::string::npos) {
        ss << str.substr(pos, len - pos);
        pos = len;
        break;
      }
      ss << str.substr(pos, newPos + 4 - pos);
      pos = newPos + 4;
      if (PositionInRanges(newPos, ranges)) {
        break;
      }
    }
  }
  auto retStr = ss.str();
  // printf("==%s=%s=\r\n", str.c_str(), retStr.c_str());
  return retStr;
}

std::string SourceGenerator::GenComponentPreprocessFunction(
    std::string path, Component *component) {
  std::string preprocessFunction = "";
  const std::string preprocessPath = "processorPath";
  rapidjson::Value &templateApi = *(component->template_api());

  if (!templateApi.IsNull() && !templateApi[preprocessPath].IsNull()) {
    std::string component_path = path;
    std::string path = templateApi[preprocessPath].GetString();
    std::string temp_preprocessFunction = path.c_str();
    std::string base = "registerDataProcessor(";
    std::string replace = "registerDataProcessor($kTemplateAssembler, ";

    for (int i = 0;
         (i = temp_preprocessFunction.find(base, i)) != std::string::npos;) {
      temp_preprocessFunction =
          temp_preprocessFunction.replace(i++, base.length(), replace);
    }

    // handle for 'getDerivedStateFromProps' cases.
    std::string single_quoted_get_derived = "\'getDerivedStateFromProps\')";
    std::string single_quoted_get_derived_replace =
        "\"getDerivedStateFromProps\")";
    for (int j = 0; (j = temp_preprocessFunction.find(single_quoted_get_derived,
                                                      j)) != std::string::npos;
         j += single_quoted_get_derived_replace.length()) {
      temp_preprocessFunction = temp_preprocessFunction.replace(
          j++, single_quoted_get_derived.length(),
          single_quoted_get_derived_replace);
    }
    // getDerivedStateFromProps
    std::string get_derived_end = "getDerivedStateFromProps\")";
    std::string get_derived_end_replace = "getDerivedStateFromProps\", \"";
    get_derived_end_replace += component_path;
    get_derived_end_replace += "\")";
    for (int j = 0; (j = temp_preprocessFunction.find(get_derived_end, j)) !=
                    std::string::npos;
         j += get_derived_end_replace.length()) {
      temp_preprocessFunction = temp_preprocessFunction.replace(
          j++, get_derived_end.length(), get_derived_end_replace);
    }

    // handle for 'shouldComponentUpdate' cases.
    std::string single_quoted_should_update = "\'shouldComponentUpdate\')";
    std::string single_quoted_should_update_replace =
        "\"shouldComponentUpdate\")";
    for (int j = 0; (j = temp_preprocessFunction.find(
                         single_quoted_should_update, j)) != std::string::npos;
         j += single_quoted_should_update_replace.length()) {
      temp_preprocessFunction = temp_preprocessFunction.replace(
          j++, single_quoted_should_update.length(),
          single_quoted_should_update_replace);
    }
    // shouldComponentUpdate
    std::string should_component_update_end = "shouldComponentUpdate\")";
    std::string should_component_update_end_replace =
        "shouldComponentUpdate\", \"";
    should_component_update_end_replace += component_path;
    should_component_update_end_replace += "\")";
    for (int j = 0; (j = temp_preprocessFunction.find(
                         should_component_update_end, j)) != std::string::npos;
         j += should_component_update_end_replace.length()) {
      temp_preprocessFunction = temp_preprocessFunction.replace(
          j++, should_component_update_end.length(),
          should_component_update_end_replace);
    }
    if (!temp_preprocessFunction.empty()) {
      preprocessFunction += "\n{\n" + temp_preprocessFunction + "\n}\n";
    }
  }

  return preprocessFunction;
}

std::string SourceGenerator::GenPreBuildInFunctions() {
  if (!generator_options_.enable_tt_for_full_version ||
      !generator_options_.has_tt_for_command ||
      !compile_options_.enable_lepus_ng_) {
    return "";
  }
  // clang-format off
  return "function $ttForRenderList($object, $renderItemFunc) {"
            "if($object === undefined || $object === null){"
              "return;"
            "}"
            "let $type = typeof $object;"
            "if ($type === \"function\") {"
              "return;"
            "}"           
            "if ($type === \"number\") {"
              "for(let $index=0; $index<$object; ++$index) {"
                "$renderItemFunc($index, $index, $index);"
              "}"
            "}else if($type ===\"string\") {"
              "for (let $index = 0; $index < _GetLength($object); ++$index) {"
                "let $item = _IndexOf($object, $index);"
                "$renderItemFunc($index, $index,$item);"
              "}"
            "}else {"
              "let $isArray = Array.isArray($object);"
              "let $keys = Object.keys($object);"
              "for(let $index =0; $index< _GetLength($keys); ++$index) {"
                "let $key = $keys[$index];"
                "let $item = $isArray?_IndexOf($object, $index):$object[$key];"
             
                "$renderItemFunc($index, $key, $item);"
              "}"
            "}"
         "};";
  // clang-format on
}

std::string SourceGenerator::GenElementWorkletRequire(
    Component *cur_component) {
  static std::string pre_declare = "let __inner_worklet_target_id = null;";
  static std::string page_source =
      "__inner_worklet_target_id=-1;"
      "{require}";
  static std::string component_source =
      "__inner_worklet_target_id={id};"
      "{require}";
  static std::string require_source =
      "{"
      "{code}"
      "};";
  // Find necessary component and template for page
  std::set<Component *> necessary_components;
  FindNecessaryComponentInComponent(package_instance_.get(), cur_component,
                                    necessary_components);

  std::string total_require = "";
  for (auto *component : necessary_components) {
    const auto &worklet = package_instance_->GetWorklet(component->path());
    if (!worklet.IsObject()) {
      continue;
    }
    std::set<std::string> require;
    for (const auto &pair : worklet.GetObject()) {
      if (!pair.value.IsObject()) {
        LOGE(pair.name.GetString() << " worklet info is not object");
        continue;
      }
      const static auto &f = [](const auto &path, const auto &info,
                                const auto &key,
                                const std::string &default_value = "") {
        if (!info.HasMember(key)) {
          LOGE("The " << path << " worklet info doesn't contain " << key);
          return false;
        }
        if (!info[key].IsString()) {
          LOGE("The " << path << " worklet info'" << key << " is not string");
          return false;
        }
        if (!default_value.empty() &&
            std::string(info[key].GetString()) != default_value) {
          LOGI("The " << path << " worklet is not " << default_value);
          return false;
        }
        return true;
      };
      const auto &path = pair.name.GetString();
      const auto &val = pair.value.GetObject();
      if (f(path, val, "type", "element") && f(path, val, "code")) {
        const auto &code = pair.value["code"].GetString();
        std::unordered_map<std::string, std::string> rule = {{"code", code}};
        require.insert(FormatStringWithRule(require_source, rule));
      }
    }
    if (require.empty()) {
      return "";
    }
    std::string code = "";
    for (const auto &r : require) {
      code += r;
    }
    std::unordered_map<std::string, std::string> rule = {
        {"id", std::to_string(component->id())}, {"require", code.c_str()}};
    if (component->IsPage()) {
      total_require += FormatStringWithRule(page_source, rule);
    }
    total_require += FormatStringWithRule(component_source, rule);
  }
  if (total_require.empty()) {
    return total_require;
  }
  total_require = pre_declare + total_require;
  return total_require;
}

std::string SourceGenerator::GenPagePreprocessFunction(Page *page) {
  // Template API
  std::string preprocessFunction = "";
  const std::string preprocessPath = "processorPath";
  rapidjson::Value &templateApi = *(page->template_api());
  if (!templateApi.IsNull() && !templateApi[preprocessPath].IsNull()) {
    std::string path = templateApi[preprocessPath].GetString();
    preprocessFunction += path.c_str();
    std::string base = "registerDataProcessor(";
    std::string replace = "registerDataProcessor($kTemplateAssembler, ";
    for (int i = 0;
         (i = preprocessFunction.find(base, i)) != std::string::npos;) {
      preprocessFunction =
          preprocessFunction.replace(i++, base.length(), replace);
    }
  }
  if (closure_fix_ && !preprocessFunction.empty()) {
    preprocessFunction += "\n}\n";
    preprocessFunction.insert(0, "\n{\n");
  }
  return preprocessFunction;
}

std::string SourceGenerator::GenTemplateFunctions(Component *component) {
  // Template API
  std::string templateFunctions = "";
  const std::string templateFunctionsPath = "templateFunctionsPath";
  const std::string templateFunctionsName = "templateFunctionsName";
  const std::string entryCode = "entryCode";
  if (component->template_api() == nullptr) {
    return templateFunctions;
  }
  rapidjson::Value &templateApi = *(component->template_api());
  if (!templateApi.IsNull() && !templateApi[templateFunctionsPath].IsNull()) {
    std::string path = "";
    std::string name = "";
    std::string func_str = "";
    std::unordered_set<std::string> func_set;
    if (templateApi.HasMember(entryCode)) {
      templateFunctions += templateApi[entryCode].GetString();
    }
    for (int i = 0; i != templateApi[templateFunctionsPath].Size(); ++i) {
      path = templateApi[templateFunctionsPath][i].GetString();
      name = templateApi[templateFunctionsName][i].GetString();
      func_str = path.c_str();

      templateFunctions += "let " + name + " = {};\n";
      templateFunctions += "(function(exports){\n";
      templateFunctions += func_str;
      templateFunctions += "})(" + name + ");\n";
    }
  }

  return templateFunctions;
}

std::string SourceGenerator::GenScriptMapDefines(
    Component *component, const std::string &component_str) {
  // Template Script
  const auto &script_map = package_instance_->script_map();
  std::string script_defines;
  if (compile_options_.enable_lepus_ng_ &&
      Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                              FEATURE_TEMPLATE_SCRIPT) &&
      script_map.IsObject() && script_map.HasMember(component->path())) {
    const auto &scripts = script_map[component->path().c_str()];
    if (scripts.IsObject()) {
      for (const auto &pair : scripts.GetObject()) {
        if (pair.value.IsString()) {
          std::unordered_map<std::string, std::string> rule = {
              {"workletName", pair.name.GetString()},
              {"workletReference", GenStatement(pair.value)},
              {"component_str", component_str}};

          // clang-format off
          static std::string source =
            "{workletName} = __require({workletReference});"
            "registerElementWorklet({workletName}, '{workletName}', {component_str});";
          static std::string declare_source =
            "let {workletName} = __require({workletReference});"
            "registerElementWorklet({workletName}, '{workletName}', {component_str});";
          // clang-format on
          // If workletName has been declared, use source. Otherwise use
          // declare_source
          if (component->variables_in_use().find(pair.name.GetString()) !=
              component->variables_in_use().end()) {
            script_defines =
                script_defines + FormatStringWithRule(source, rule);
          } else {
            script_defines =
                script_defines + FormatStringWithRule(declare_source, rule);
          }
        }
      }
    }
  }

  return script_defines;
}

std::string SourceGenerator::FormatContextDataStatement(
    const std::string &str, std::string::size_type pos) {
  std::vector<std::pair<int, int>> ranges;
  int s, e = 0;
  while ((s = (int)str.find("{{", e)) >= 0 && (e = str.find("}}", s + 2)) > 0) {
    ranges.push_back({s, e});
  }
  if (!PositionInRanges(pos, ranges)) {
    return str;
  }

  std::stringstream ss;
  ss << str.substr(0, pos);
  const int len = str.size();
  const char *c_str = str.c_str();
  while (pos < len) {
    int kwd_len = 0;
    bool add_sep = false;
    const char *cur_str = c_str + pos;
    if (*cur_str == '(') {
      kwd_len = 1;
      add_sep = true;
    }
    if (kwd_len > 0) {
      ss << str.substr(pos, kwd_len);
      ss << "$component";
      if (add_sep) ss << ",";
      pos += kwd_len;
    } else {
      ss << *cur_str;
      ++pos;
    }
    auto new_pos = std::string::npos;
    while (true) {
      new_pos = str.find(kContextPrefix, pos);
      if (new_pos == std::string::npos) {
        ss << str.substr(pos, len - pos);
        pos = len;
        break;
      }
      ss << str.substr(pos, new_pos + 8 - pos);
      pos = new_pos + 8;
      if (PositionInRanges(new_pos, ranges)) {
        break;
      }
    }
  }
  return ss.str();
}

void SourceGenerator::AddDynamicComponentDeclaration(const std::string &key,
                                                     const std::string &value) {
  dynamic_component_declarations_[key] = value;
}

// Statement should be wrapped by {{}} otherwise will be treated as string
std::string SourceGenerator::GenStatement(const std::string &statement) {
  if (statement.size() == 0) {
    return "null";
  }
  auto source = FormatValue(statement);
  if (IsVariableString(statement)) {
    auto pos = statement.find("_sys");
    if (pos != std::string::npos) {
      source = FormatValue(FormatThemedStatement(statement, pos + 4));
    }

    auto context_pos = statement.find(kContextPrefix);
    if (context_pos != std::string::npos) {
      source =
          FormatValue(FormatContextDataStatement(statement, context_pos + 8));
    }
  }

  parser::InputStream input;
  input.Write(source);
  lepus::Scanner scanner(&input);
  lepus::Parser parser(&scanner);

  auto chunk = std::unique_ptr<lepus::ASTree>(parser.Parse());
  TopVariableFinder finder(current_template_);
  chunk->Accept(&finder, nullptr);

  return source;
}

std::string SourceGenerator::GenStatement(const rapidjson::Value &statement) {
  if (statement.IsNull()) {
    return "null";
  }
  DCHECK(statement.IsString() || statement.IsBool() || statement.IsNumber() ||
         statement.IsArray() || statement.IsObject());

  if (statement.IsBool()) {
    return statement.GetBool() ? "true" : "false";
  }

  if (statement.IsNumber()) {
    std::stringstream ss;
    ss << statement.GetDouble();
    return ss.str();
  }

  if (statement.IsArray()) {
    std::stringstream ss;
    ss << '[';
    const auto &ary = statement.GetArray();
    for (int i = 0; i < ary.Size(); ++i) {
      ss << GenStatement(ary[i]);
      if (i != ary.Size() - 1) {
        ss << ',';
      }
    }
    ss << ']';
    return ss.str();
  }

  if (statement.IsObject()) {
    std::stringstream ss;
    ss << '{';
    for (const auto &pair : statement.GetObject()) {
      ss << GenStatement(pair.name);
      ss << ':';
      ss << GenStatement(pair.value);
      ss << ',';
    }
    ss << '}';
    return ss.str();
  }

  std::string str = statement.GetString();
  return GenStatement(str);
}

void SourceGenerator::GenPageMould(Page *page) {
  // Generate page mould
  auto page_mould = std::make_unique<PageMould>();
  if (page->data().IsObject()) {
    lepus::Value data = lepus::Value(lepus::Dictionary::Create());
    for (auto it = page->data().MemberBegin(); it != page->data().MemberEnd();
         ++it) {
      if (!it->name.IsString()) {
        THROW_ERROR_MSG("data name is not string");
      }

      // changed by wby, encode all the 'data',  the 'unused' data maybe used in
      // js
      base::String key(it->name.GetString());
      data.Table()->SetValue(key, lepus::jsonValueTolepusValue(it->value));
    }
    page_mould->set_data(data);
  }
  page_mould->set_id(page->id());
  page_mould->set_css_id(GetCSSForComponent(page));
  for (const auto &it : page->dependent_components()) {
    auto dp = package_instance_->GetComponent(it.second);
    page_mould->AddDependentComponentId(dp->id());
  }

  // DynamicComponent Info
  for (const auto &it : page->dependent_dynamic_components()) {
    this->AddDynamicComponentDeclaration(it.first, it.second);
  }

  page_moulds_[page->path()] = std::move(page_mould);
}

void SourceGenerator::GenComponentMould(Component *component) {
  // Gen component mould
  ComponentMould *component_mould = nullptr;
  if (component->IsDynamicComponent()) {
    component_mould = new DynamicComponentMould();
  } else {
    component_mould = new ComponentMould();
  }
  component_mould->set_path(component->path());
  component_mould->set_name(component->name());
  if (component->data().IsObject()) {
    lepus::Value data = lepus::Value(lepus::Dictionary::Create());
    for (auto it = component->data().MemberBegin();
         it != component->data().MemberEnd(); ++it) {
      DCHECK(it->name.IsString());
      if (component->IsVariableInUse(it->name.GetString())) {
        base::String key(it->name.GetString());
        data.Table()->SetValue(key, lepus::jsonValueTolepusValue(it->value));
      }
    }
    component_mould->set_data(data);
  }
  if (component->props().IsObject()) {
    lepus::Value props = lepus::Value(lepus::Dictionary::Create());
    for (auto it = component->props().MemberBegin();
         it != component->props().MemberEnd(); ++it) {
      DCHECK(it->name.IsString());
      if (component->IsVariableInUse(it->name.GetString())) {
        base::String key(it->name.GetString());
        props.Table()->SetValue(key, lepus::jsonValueTolepusValue(it->value));
      }
    }
    component_mould->set_properties(props);
  }
  component_mould->set_properties(
      lepus::jsonValueTolepusValue(component->props()));
  component_mould->set_external_classes(
      lepus::jsonValueTolepusValue(component->external_classes()));
  std::stringstream mould_name;
  mould_name << "kComponentMould" << component->id();
  component_mould->set_id(component->id());
  component_mould->set_css_id(GetCSSForComponent(component));
  for (const auto &it : component->dependent_components()) {
    auto dp = package_instance_->GetComponent(it.second);
    component_mould->AddDependentComponentId(dp->id());
  }

  // Dynamic Component Info
  for (const auto &it : component->dependent_dynamic_components()) {
    this->AddDynamicComponentDeclaration(it.first, it.second);
  }

  if (component->IsDynamicComponent()) {
    dynamic_component_moulds_[mould_name.str()] =
        std::unique_ptr<DynamicComponentMould>(
            static_cast<DynamicComponentMould *>(component_mould));
  } else {
    component_moulds_[mould_name.str()] =
        std::unique_ptr<ComponentMould>(component_mould);
  }
}

//
// Check if a JSON value is an available string.
//
//    "   "  => unavailable
//    ""     => unavailable
//    " a "  => available
//
bool SourceGenerator::IsAvailableString(const rapidjson::Value &str) {
  if (!str.IsString()) {
    return false;
  }
  const char *c_str = str.GetString();
  size_t length = strlen(str.GetString());
  if (length == 0) {
    return false;
  }
  while (*(c_str++) == ' ' && length-- != 0) {
  }
  return length != 0;
}

std::string SourceGenerator::GetComponentPath(const std::string &tag) {
  return current_component_->GetDependentComponentPath(tag);
}

bool SourceGenerator::IsComponent(const std::string &tag) {
  if (tag.compare("view") == 0 || tag.compare("text") == 0 ||
      tag.compare("image") == 0) {
    return false;
  }
  return current_component_->IsDependentComponent(tag);
}

bool SourceGenerator::IsComponentIs(const rapid_value &element) {
  DCHECK(element.IsObject());
  auto &tag = VALUE(element["tagName"]);
  DCHECK(tag.IsString());
  std::string tag_name = tag.GetString();
  if (tag_name.compare("component") == 0 && element.HasMember("is")) {
    return true;
  }
  return false;
}

bool SourceGenerator::IsDynamicComponent(const std::string &tag) {
  if (tag.compare("view") == 0 || tag.compare("text") == 0 ||
      tag.compare("image") == 0) {
    return false;
  }
  return current_component_->IsDependentDynamicComponent(tag);
}

std::string SourceGenerator::DynamicComponentEntryName() {
  return package_instance_->EntryName();
}

void SourceGenerator::ParseCard() {
  auto &pages = static_cast<App *>(package_instance_.get())->pages();
  for (auto &page : pages) {
    if (is_single_page_ && !page->is_default_entry()) {
      continue;
    }
    if (page->is_default_entry()) {
      main_page_id_ = page->id();
      app_mould_.main_page_id = main_page_id_;
    }

    page_config_[page->id()] = page->config();
    GenPageSource(page.get());
  }
  for (auto &[_, component] : package_instance_->components()) {
    component_config_[component->id()] = component->config();
  }
  app_mould_.themedTrans_ = ((App *)package_instance_.get())->getThemedTrans();
}

void SourceGenerator::ParseDynamicComponent() {
  for (auto &[_, component] : package_instance_->components()) {
    component_config_[component->id()] = component->config();
  }
  auto &dynamic_components =
      static_cast<HotSwapApp *>(package_instance_.get())->DynamicComponents();
  for (auto &dynamic_component : dynamic_components) {
    component_config_[dynamic_component->id()] = dynamic_component->config();
    GenDynamicComponentSource(dynamic_component.get());
  }
}

}  // namespace tasm
}  // namespace lynx
