// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Shared implementation for `LynxTemplateBundleConverter`. Compiled into both
// the OSS build and the internal build; the only piece that differs between
// them is `SerializeMTSBundle`, which is provided by a swappable translation
// unit (see `lynx_template_bundle_converter.cc`).

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/css_font_face_token.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/css/ng/selector/lynx_css_selector_list.h"
#include "core/renderer/css/ng/style/condition_rule.h"
#include "core/renderer/css/ng/style/style_rule.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/template_bundle/lynx_template_bundle_converter.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace tasm {

// Provided by whichever `lynx_template_bundle_converter.cc` variant is linked
// in this build. Kept as a free function (rather than a virtual on the
// converter class) so that decode-only builds can swap in a version that knows
// about additional engines (e.g. RTS) without touching the public header.
void SerializeMTSBundle(
    const std::shared_ptr<runtime::ContextBundle>& context_bundle,
    rapidjson::Document& document);

namespace {

template <typename T>
std::string JoinCSSParts(const std::vector<T>& parts, const char* separator) {
  std::string result;
  for (const auto& part : parts) {
    if (!result.empty()) {
      result.append(separator);
    }
    result.append(part);
  }
  return result;
}

struct CSSDeclaration {
  std::string name;
  std::string value;
  bool important = false;
};

void AppendDeclaration(std::vector<CSSDeclaration>& declarations,
                       std::string name, std::string value,
                       bool important = false) {
  if (name.empty() || value.empty()) {
    return;
  }
  declarations.push_back({std::move(name), std::move(value), important});
}

std::string SerializeDeclarations(std::vector<CSSDeclaration>& declarations) {
  std::sort(declarations.begin(), declarations.end(),
            [](const CSSDeclaration& lhs, const CSSDeclaration& rhs) {
              if (lhs.name != rhs.name) {
                return lhs.name < rhs.name;
              }
              if (lhs.important != rhs.important) {
                return !lhs.important;
              }
              return lhs.value < rhs.value;
            });

  std::vector<std::string> serialized;
  serialized.reserve(declarations.size());
  for (const auto& declaration : declarations) {
    std::string text = declaration.name + ": " + declaration.value;
    if (declaration.important) {
      text += " !important";
    }
    text.push_back(';');
    serialized.emplace_back(std::move(text));
  }
  return JoinCSSParts(serialized, " ");
}

std::string SerializeCSSString(const std::string& value) {
  std::string result;
  result.reserve(value.size() + 2);
  result.push_back('"');
  for (const char character : value) {
    switch (character) {
      case '\\':
        result += "\\\\";
        break;
      case '"':
        result += "\\\"";
        break;
      case '\n':
        result += "\\A ";
        break;
      case '\r':
        result += "\\D ";
        break;
      case '\f':
        result += "\\C ";
        break;
      default:
        result.push_back(character);
        break;
    }
  }
  result.push_back('"');
  return result;
}

bool IsSimpleFontFamilyName(const std::string& value) {
  if (value.empty()) {
    return false;
  }
  const auto is_ident_start = [](unsigned char character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z') || character == '_';
  };

  size_t index = 0;
  if (value.front() == '-') {
    if (value.size() == 1) {
      return false;
    }
    if (value[1] == '-') {
      index = 2;
    } else if (is_ident_start(value[1])) {
      index = 1;
    } else {
      return false;
    }
  } else if (is_ident_start(value.front())) {
    index = 1;
  } else {
    return false;
  }

  for (; index < value.size(); ++index) {
    const unsigned char character = value[index];
    if (!(is_ident_start(character) || (character >= '0' && character <= '9') ||
          character == '-')) {
      return false;
    }
  }
  return true;
}

std::string SerializeFontFamilyName(const std::string& value) {
  const bool is_quoted =
      value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                            (value.front() == '\'' && value.back() == '\''));
  return is_quoted || IsSimpleFontFamilyName(value) ? value
                                                    : SerializeCSSString(value);
}

void AppendStyleMapDeclarations(const StyleMap* styles,
                                const StyleMap* important_styles,
                                std::vector<CSSDeclaration>& declarations) {
  if (styles != nullptr) {
    for (const auto& entry : *styles) {
      if (important_styles != nullptr &&
          important_styles->find(entry.first) != important_styles->end()) {
        continue;
      }
      AppendDeclaration(
          declarations, CSSProperty::GetPropertyName(entry.first).c_str(),
          CSSDecoder::CSSValueToString(entry.first, entry.second, true));
    }
  }

  if (important_styles != nullptr) {
    for (const auto& entry : *important_styles) {
      AppendDeclaration(
          declarations, CSSProperty::GetPropertyName(entry.first).c_str(),
          CSSDecoder::CSSValueToString(entry.first, entry.second, true), true);
    }
  }
}

void AppendCSSVariableDeclarations(const CSSVariableMap& variables,
                                   std::vector<CSSDeclaration>& declarations) {
  for (const auto& entry : variables) {
    AppendDeclaration(declarations, entry.first.str(), entry.second.str());
  }
}

void AppendCustomPropertyDeclarations(
    const CustomPropertiesMap& properties,
    std::vector<CSSDeclaration>& declarations) {
  for (const auto& entry : properties) {
    AppendDeclaration(
        declarations, entry.first.str(),
        CSSDecoder::CSSValueToString(kPropertyStart, entry.second, true));
  }
}

std::string SerializeDeclarations(const CSSParseToken& token) {
  // GetAttributes() lazily converts raw descriptors. Decoding has completed
  // before this converter is called, so this const_cast only materializes the
  // already-decoded representation and does not change the CSS semantics.
  auto* mutable_token = const_cast<CSSParseToken*>(&token);
  const auto& styles = mutable_token->GetAttributes();
  const auto& important_styles = mutable_token->GetImportantAttributes();

  std::vector<CSSDeclaration> declarations;
  AppendStyleMapDeclarations(&styles, &important_styles, declarations);
  AppendCSSVariableDeclarations(token.GetStyleVariables(), declarations);
  return SerializeDeclarations(declarations);
}

std::string SerializeDeclarations(
    const StyleMap* styles, const CustomPropertiesMap* custom_properties) {
  std::vector<CSSDeclaration> declarations;
  AppendStyleMapDeclarations(styles, nullptr, declarations);
  if (custom_properties != nullptr) {
    AppendCustomPropertyDeclarations(*custom_properties, declarations);
  }
  return SerializeDeclarations(declarations);
}

std::string SerializeStyleRule(const css::StyleRule& rule) {
  const auto selectors =
      css::LynxCSSSelectorList::SelectorsText(rule.FirstSelector());
  if (selectors.empty() || rule.Token() == nullptr) {
    return {};
  }

  const auto declarations = SerializeDeclarations(*rule.Token());
  return selectors + " {" +
         (declarations.empty() ? std::string() : " " + declarations) + " }";
}

struct SerializedStyleRule {
  const css::StyleRule* rule = nullptr;
  css::CascadeLayer* layer = nullptr;
  std::string text;
};

struct CSSSerializationContext {
  std::vector<std::string> rules;
  std::unordered_set<const CSSParseToken*> emitted_tokens;
  std::unordered_set<const css::StyleRule*> emitted_style_rules;
  std::unordered_set<const css::ConditionRule*> emitted_condition_rules;
  std::unordered_set<const CSSKeyframesToken*> emitted_keyframes;
  std::unordered_set<const CSSFontFaceRule*> emitted_font_faces;
  std::unordered_map<const css::CascadeLayer*, std::string> layer_names;
};

void CollectLayerNames(
    const css::CascadeLayer* parent, const std::string& parent_name,
    std::unordered_map<const css::CascadeLayer*, std::string>& layer_names) {
  if (parent == nullptr) {
    return;
  }
  for (const auto& child : parent->GetDirectSubLayers()) {
    const auto& child_name = child->GetName();
    const std::string name =
        parent_name.empty() ? child_name : parent_name + "." + child_name;
    layer_names.emplace(child.get(), name);
    CollectLayerNames(child.get(), name, layer_names);
  }
}

std::string AddLayerWrapper(const std::string& rule,
                            const css::CascadeLayer* layer,
                            const CSSSerializationContext& context) {
  if (layer == nullptr) {
    return rule;
  }
  const auto it = context.layer_names.find(layer);
  if (it == context.layer_names.end() || it->second.empty()) {
    return rule;
  }
  return "@layer " + it->second + " { " + rule + " }";
}

void AppendRuleSetStyles(const css::RuleSet& rule_set,
                         CSSSerializationContext& context,
                         std::vector<std::string>& output) {
  std::vector<SerializedStyleRule> style_rules;
  rule_set.ForEachStyleRule(
      [&style_rules](const css::StyleRule& rule, css::CascadeLayer* layer) {
        style_rules.push_back({&rule, layer, {}});
      });

  for (auto& style_rule : style_rules) {
    const auto serialized = SerializeStyleRule(*style_rule.rule);
    style_rule.text =
        serialized.empty()
            ? std::string()
            : AddLayerWrapper(serialized, style_rule.layer, context);
  }

  std::sort(style_rules.begin(), style_rules.end(),
            [](const SerializedStyleRule& lhs, const SerializedStyleRule& rhs) {
              if (lhs.rule->Position() != rhs.rule->Position()) {
                return lhs.rule->Position() < rhs.rule->Position();
              }
              return lhs.text < rhs.text;
            });

  for (const auto& style_rule : style_rules) {
    if (!context.emitted_style_rules.insert(style_rule.rule).second) {
      continue;
    }
    const auto token = style_rule.rule->Token();
    if (token == nullptr ||
        !context.emitted_tokens.insert(token.get()).second) {
      continue;
    }
    if (!style_rule.text.empty()) {
      output.emplace_back(style_rule.text);
    }
  }
}

std::string SerializeConditionRule(const css::ConditionRule& condition,
                                   CSSSerializationContext& context) {
  std::string prelude;
  if (condition.MediaQueries() != nullptr) {
    prelude = "@media " + condition.MediaQueries()->Serialize();
  } else if (condition.SupportsCondition() != nullptr) {
    prelude = "@supports " + condition.SupportsCondition()->Serialize();
  }
  if (prelude.empty()) {
    return {};
  }

  std::vector<std::string> child_rules;
  AppendRuleSetStyles(condition.GetRuleSet(), context, child_rules);
  return prelude + " {" +
         (child_rules.empty() ? std::string()
                              : " " + JoinCSSParts(child_rules, " ")) +
         " }";
}

void AppendLegacyCSSRules(SharedCSSFragment& fragment,
                          CSSSerializationContext& context) {
  std::vector<std::pair<std::string, const CSSParseToken*>> styles;
  for (const auto& entry : fragment.css()) {
    if (entry.second != nullptr) {
      styles.emplace_back(entry.first, entry.second.get());
    }
  }
  std::sort(styles.begin(), styles.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.first < rhs.first;
  });

  for (const auto& style : styles) {
    if (!context.emitted_tokens.insert(style.second).second) {
      continue;
    }
    const auto declarations = SerializeDeclarations(*style.second);
    context.rules.emplace_back(
        style.first + " {" +
        (declarations.empty() ? std::string() : " " + declarations) + " }");
  }
}

std::string SerializeKeyframeOffset(float offset) {
  if (offset == 0) {
    return "from";
  }
  if (offset == 1) {
    return "to";
  }
  return CSSDecoder::NumberToString(offset * 100) + "%";
}

void AppendKeyframes(SharedCSSFragment& fragment,
                     CSSSerializationContext& context) {
  std::vector<std::pair<std::string, const CSSKeyframesToken*>> keyframes;
  for (const auto& entry : fragment.GetKeyframesRuleMap()) {
    if (entry.second != nullptr) {
      keyframes.emplace_back(entry.first.str(), entry.second.get());
    }
  }
  std::sort(
      keyframes.begin(), keyframes.end(),
      [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

  for (const auto& keyframe : keyframes) {
    if (!context.emitted_keyframes.insert(keyframe.second).second) {
      continue;
    }

    auto* token = const_cast<CSSKeyframesToken*>(keyframe.second);
    const auto& content = token->GetKeyframesContent();
    const auto& custom_content = token->GetKeyframesCustomPropertyContent();
    std::vector<float> offsets;
    offsets.reserve(content.size() + custom_content.size());
    for (const auto& entry : content) {
      offsets.emplace_back(entry.first);
    }
    for (const auto& entry : custom_content) {
      if (std::find(offsets.begin(), offsets.end(), entry.first) ==
          offsets.end()) {
        offsets.emplace_back(entry.first);
      }
    }
    std::sort(offsets.begin(), offsets.end());

    std::vector<std::string> frames;
    for (float offset : offsets) {
      const auto style_it = content.find(offset);
      const auto custom_it = custom_content.find(offset);
      const StyleMap* styles =
          style_it == content.end() || style_it->second == nullptr
              ? nullptr
              : style_it->second.get();
      const CustomPropertiesMap* custom_properties =
          custom_it == custom_content.end() || custom_it->second == nullptr
              ? nullptr
              : custom_it->second.get();
      const auto declarations =
          SerializeDeclarations(styles, custom_properties);
      frames.emplace_back(
          SerializeKeyframeOffset(offset) + " {" +
          (declarations.empty() ? std::string() : " " + declarations) + " }");
    }
    context.rules.emplace_back(
        "@keyframes " + keyframe.first + " {" +
        (frames.empty() ? std::string() : " " + JoinCSSParts(frames, " ")) +
        " }");
  }
}

void AppendFontFaces(SharedCSSFragment& fragment,
                     CSSSerializationContext& context) {
  std::vector<std::pair<std::string,
                        const std::vector<std::shared_ptr<CSSFontFaceRule>>*>>
      font_faces;
  for (const auto& entry : fragment.GetFontFaceRuleMap()) {
    font_faces.emplace_back(entry.first, &entry.second);
  }
  std::sort(
      font_faces.begin(), font_faces.end(),
      [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

  for (const auto& family : font_faces) {
    for (const auto& face : *family.second) {
      if (face == nullptr ||
          !context.emitted_font_faces.insert(face.get()).second) {
        continue;
      }
      std::vector<CSSDeclaration> descriptors;
      for (const auto& descriptor : face->second) {
        AppendDeclaration(descriptors, descriptor.first,
                          descriptor.first == "font-family"
                              ? SerializeFontFamilyName(descriptor.second)
                              : descriptor.second);
      }
      if (std::none_of(descriptors.begin(), descriptors.end(),
                       [](const CSSDeclaration& descriptor) {
                         return descriptor.name == "font-family";
                       })) {
        AppendDeclaration(descriptors, "font-family",
                          SerializeFontFamilyName(face->first));
      }
      const auto declarations = SerializeDeclarations(descriptors);
      context.rules.emplace_back(
          "@font-face {" +
          (declarations.empty() ? std::string() : " " + declarations) + " }");
    }
  }
}

void AppendRuleSetAndConditions(SharedCSSFragment& fragment,
                                CSSSerializationContext& context) {
  auto* rule_set = fragment.rule_set();
  if (rule_set == nullptr) {
    return;
  }
  AppendRuleSetStyles(*rule_set, context, context.rules);
  rule_set->ForEachConditionRule([&context](const css::ConditionRule& rule) {
    if (!context.emitted_condition_rules.insert(&rule).second) {
      return;
    }
    const auto serialized = SerializeConditionRule(rule, context);
    if (!serialized.empty()) {
      context.rules.emplace_back(serialized);
    }
  });
}

std::string SerializeCSSText(const LynxTemplateBundle& template_bundle) {
  const auto& manager = template_bundle.GetCSSStyleManager();
  if (manager == nullptr) {
    return {};
  }
  const auto& fragment_map = manager->GetCSSFragmentMap();
  if (fragment_map == nullptr) {
    return {};
  }

  std::vector<SharedCSSFragment*> fragments;
  fragments.reserve(fragment_map->size());
  for (const auto& entry : *fragment_map) {
    if (entry.second != nullptr) {
      fragments.emplace_back(entry.second.get());
    }
  }
  std::sort(
      fragments.begin(), fragments.end(),
      [](const auto* lhs, const auto* rhs) { return lhs->id() < rhs->id(); });

  CSSSerializationContext context;
  for (const auto* fragment : fragments) {
    CollectLayerNames(fragment->root_layer(), "", context.layer_names);
  }
  for (auto* fragment : fragments) {
    AppendLegacyCSSRules(*fragment, context);
    AppendRuleSetAndConditions(*fragment, context);
    AppendKeyframes(*fragment, context);
    AppendFontFaces(*fragment, context);
  }
  return JoinCSSParts(context.rules, "\n");
}

}  // namespace

/// method to serialize compilerOptions in template_bundle into json string.
void SerializeCompilerOptions(const CompileOptions& compile_options,
                              rapidjson::Document& document) {
  auto& allocator = document.GetAllocator();
  rapidjson::Value compiler_options_json(rapidjson::kObjectType);

  compiler_options_json.AddMember("config_type", compile_options.config_type,
                                  allocator);

#define SERIALIZE_FIXED_LENGTH_FIELD(TYPE, NAME, ID)                         \
  if constexpr (std::is_same<decltype(compile_options.NAME), bool>::value) { \
    compiler_options_json.AddMember(                                         \
        #NAME, static_cast<bool>(compile_options.NAME), allocator);          \
  } else {                                                                   \
    compiler_options_json.AddMember(                                         \
        #NAME, static_cast<int64_t>(compile_options.NAME), allocator);       \
  }

  FOREACH_FIXED_LENGTH_FIELD(SERIALIZE_FIXED_LENGTH_FIELD);
#undef SERIALIZE_FIXED_LENGTH_FIELD

#define SERIALIZE_STRING_FIELD_IMPL(NAME, ID)                           \
  compiler_options_json.AddMember(                                      \
      #NAME, rapidjson::Value(compile_options.NAME.c_str(), allocator), \
      allocator);

  FOREACH_STRING_FIELD(SERIALIZE_STRING_FIELD_IMPL);
#undef SERIALIZE_STRING_FIELD_IMPL

  document.AddMember("compilerOptions", compiler_options_json, allocator);
}

/// method to serialize a template_bundle into json string,
/// be careful if you need to call this method.
void SerializeBTSBundle(const runtime::js::JsBundle& js_bundle,
                        rapidjson::Document& document) {
  auto js_files = js_bundle.GetAllJsFiles();
  auto& allocator = document.GetAllocator();
  std::for_each(
      js_files.begin(), js_files.end(),
      [&document,
       &allocator](const std::pair<std::string, runtime::js::JsContent>& pair) {
        auto js_content = pair.second;
        rapidjson::Document js_content_document(&allocator);
        js_content_document.SetObject();
        js_content_document.AddMember("path", pair.first, allocator);
        js_content_document.AddMember(
            "type",
            rapidjson::Value(js_content.IsByteCode() ? "bytecode" : "source",
                             allocator),
            allocator);
        std::string str(
            reinterpret_cast<const char*>(js_content.GetBuffer()->data()),
            js_content.GetBuffer()->size());
        js_content_document.AddMember(
            "content", rapidjson::Value(std::move(str), allocator), allocator);
        document.PushBack(rapidjson::Value(js_content_document, allocator),
                          allocator);
      });
}

void SerializeCustomSections(const lepus::Value& custom_sections,
                             rapidjson::Document& document) {
  auto& allocator = document.GetAllocator();
  ForEachLepusValue(
      custom_sections, [&allocator, &document](const lepus::Value& key,
                                               const lepus::Value& value) {
        auto key_rapid = rapidjson::Value(key.String().c_str(), allocator);
        if (value.IsString()) {
          auto value_str = value.String().c_str();
          document.AddMember(key_rapid, rapidjson::Value(value_str, allocator),
                             allocator);
        } else if (value.IsByteArray()) {
          auto byte_array = value.ByteArray();
          auto* ptr = byte_array->GetPtr();
          rapidjson::Value lepus_code(rapidjson::kArrayType);
          std::vector<uint8_t> binary =
              std::vector<uint8_t>(ptr, ptr + byte_array->GetLength());
          for (const auto& element : binary) {
            lepus_code.PushBack(element, allocator);
          }
          document.AddMember(key_rapid, lepus_code, allocator);
        }
      });
}

std::string
LynxTemplateBundleConverter::ConvertTemplateBundleToSerializedString(
    LynxTemplateBundle& template_bundle) {
  rapidjson::Document main_document;
  main_document.SetObject();
  rapidjson::Document::AllocatorType& allocator = main_document.GetAllocator();

  // put header info;
  main_document.AddMember("total-size", template_bundle.total_size_, allocator);
  main_document.AddMember("is-lepusng-binary",
                          template_bundle.is_lepusng_binary_, allocator);
  main_document.AddMember("context-type", template_bundle.context_type_,
                          allocator);
  main_document.AddMember("engine-version", template_bundle.target_sdk_version_,
                          allocator);
  main_document.AddMember("app-type", template_bundle.app_type_, allocator);
  main_document.AddMember("enable-css-variable",
                          template_bundle.enable_css_variable_, allocator);
  main_document.AddMember("enable-css-parser",
                          template_bundle.enable_css_parser_, allocator);

  // put compiler option;
  SerializeCompilerOptions(template_bundle.GetCompileOptions(), main_document);

  // put page config;
  auto page_config = template_bundle.GetPageConfig();
  if (page_config) {
    main_document.AddMember("page-config", page_config->GetOriginalConfig(),
                            allocator);
  } else {
    main_document.AddMember("page-config",
                            rapidjson::Value(rapidjson::kNullType), allocator);
  }

  // css
  rapidjson::Document css_document(&allocator);
  css_document.SetObject();
  css_document.AddMember(
      "text", rapidjson::Value(SerializeCSSText(template_bundle), allocator),
      allocator);
  main_document.AddMember("css", rapidjson::Value(css_document, allocator),
                          allocator);

  // main-thread-script
  rapidjson::Document mts_document(&allocator);
  mts_document.SetObject();
  SerializeMTSBundle(template_bundle.context_bundle_, mts_document);
  main_document.AddMember("main-thread-script",
                          rapidjson::Value(mts_document, allocator), allocator);

  // background-thread-script
  rapidjson::Document bts_document(&allocator);
  bts_document.SetArray();
  SerializeBTSBundle(template_bundle.GetJsBundle(), bts_document);
  main_document.AddMember("background-thread-script",
                          rapidjson::Value(bts_document, allocator), allocator);

  // custom-sections
  rapidjson::Document custom_sections(&allocator);
  custom_sections.SetObject();
  SerializeCustomSections(template_bundle.custom_sections_, custom_sections);
  main_document.AddMember("custom-sections",
                          rapidjson::Value(custom_sections, allocator),
                          allocator);

  // cast to string;
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  main_document.Accept(writer);
  return buffer.GetString();
}

}  // namespace tasm
}  // namespace lynx
