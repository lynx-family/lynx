// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/generator/template_parser.h"

#include <assert.h>

#include <list>
#include <map>
#include <set>
#include <sstream>

#include "base/include/debug/lynx_assert.h"
#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "core/parser/input_stream.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/bindings/lepus/renderer.h"
#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/template_bundle/template_codec/generator/list_parser.h"
#include "core/template_bundle/template_codec/generator/template_scope.h"
#include "core/template_bundle/template_codec/version.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {

TemplateParser::TemplateParser(const EncoderOptions &encoder_options)
    : SourceGenerator(encoder_options.generator_options_.source_content_str_,
                      encoder_options.generator_options_.worklet_,
                      encoder_options.generator_options_.script_map_,
                      encoder_options.generator_options_.packed_script_,
                      encoder_options.generator_options_.instance_type_,
                      encoder_options.generator_options_.instance_dsl_,
                      encoder_options.compile_options_,
                      encoder_options.generator_options_.trial_options_,
                      encoder_options.source_generator_options_, true,
                      encoder_options.generator_options_.bundle_module_mode_,
                      encoder_options.generator_options_.lepus_code_) {}

TemplateParser::~TemplateParser() = default;

std::string TemplateParser::AddAttributes(std::string &source, std::string key,
                                          std::string value) {
  std::stringstream ss;

  static LepusGenRuleMap rule = {{"key", ""}, {"value", ""}};
  static std::string source2 = "_SetAttributeTo($child, \"{key}\", {value});";
  rule["key"] = key;
  rule["value"] = GenStatement(value);
  ss << FormatStringWithRule(source2, rule);
  ss << source;
  return ss.str();
}

TemplateRenderMap TemplateParser::GenNecessaryRenders(Component *component) {
  // Declare neccesary component & templates renderer
  std::stringstream components_render_declare;
  std::stringstream components_data_processor_declare;
  std::stringstream templates_declare;
  // Define component & template renderer
  std::stringstream component_renderers;
  std::stringstream components_data_processors;
  std::stringstream template_renderers;

  // Find necessary component and template for page
  std::set<Component *> necessary_components;
  std::set<Fragment *> necessary_fragments;
  std::set<Template *> necessary_templates;

  Component *cur_component = component;

  FindNecessaryComponentInComponent(package_instance_.get(), cur_component,
                                    necessary_components);
  // clang-format off
  std::string source = "let $renderComponent{ID} = null;";
  // clang-format on
  for (auto it = necessary_components.begin(); it != necessary_components.end();
       ++it) {
    if ((*it)->IsPage()) {
      continue;
    }
    std::stringstream id;
    id << (*it)->id();
    LepusGenRuleMap rule = {{"ID", id.str()}};
    components_render_declare << FormatStringWithRule(source, rule);
    component_renderers << GenComponentRenderer((*it));
  }

  FindNecessaryInComponent(necessary_components, necessary_fragments,
                           necessary_templates);

  for (auto it = necessary_templates.begin(); it != necessary_templates.end();
       ++it) {
    std::stringstream id;
    id << (*it)->id();
    LepusGenRuleMap rule = {{"ID", id.str()}};
    // clang-format off
    static std::string source =
        "let $renderTemplate{ID} = null;";
    // clang-format on
    templates_declare << FormatStringWithRule(source, rule);
    template_renderers << (*it)->codes()[0];
  }

  // Declare dynamic template render
  for (auto it = necessary_fragments.begin(); it != necessary_fragments.end();
       ++it) {
    if (!(*it)->has_dynamic_template()) {
      continue;
    }
    std::stringstream id;
    id << (*it)->uid();
    LepusGenRuleMap rule = {{"ID", id.str()}};
    // clang-format off
    static std::string source =
        "let $dynamicRenderTemplateInFragment{ID} = null;";
    // clang-format on
    templates_declare << FormatStringWithRule(source, rule);
    template_renderers << GenTemplateDynamicRendererInFragment((*it));
  }

  TemplateRenderMap sourceRenders;
  sourceRenders["component"] = {components_render_declare.str(),
                                component_renderers.str()};
  sourceRenders["template"] = {templates_declare.str(),
                               template_renderers.str()};

  return sourceRenders;
}

std::string TemplateParser::GenDependentComponentInfoMapDefinition(
    Component *component) {
  bool is_below_version_1_1 = !Config::IsHigherOrEqual(
      compile_options_.target_sdk_version_, FEATURE_CONTROL_VERSION);
  bool is_ge_version_1_6 =
      Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                              FEATURE_DYNAMIC_COMPONENT_VERSION) &&
      compile_options_.enable_dynamic_component_;

  if (is_below_version_1_1 && component->templates().empty()) {
    return "";
  }
  std::stringstream add_components;
  if (is_ge_version_1_6) {
    // see issue:#3019, update dynamic component info to the ComponentInfoMap,
    // if id = -1, this component is dynamic compoennt, else, the component is
    // static component
    auto &dependent_dynamic_components =
        component->dependent_dynamic_components();
    for (auto it = dependent_dynamic_components.begin();
         it != dependent_dynamic_components.end(); ++it) {
      LepusGenRuleMap rule = {{"name", it->first}, {"path", it->second}};
      static std::string source;
      // clang-format off
      source = "_UpdateComponentInfo($parent, \"{name}\", [-1, -1, \"{path}\"], \"{path}\");";
      // clang-format on
      add_components << FormatStringWithRule(source, rule);
    }
  }
  auto &dependent_components = component->dependent_components();
  for (auto it = dependent_components.begin(); it != dependent_components.end();
       ++it) {
    auto comp = package_instance_->GetComponent(it->second);
    std::stringstream id;
    id << comp->id();

    LepusGenRuleMap rule = {
        {"name", it->first}, {"id", id.str()}, {"path", comp->path()}};
    static std::string source;
    // clang-format off
    if (!is_below_version_1_1) {
      source =
        "_UpdateComponentInfo($parent, \"{name}\", [{id}, $renderComponent{id}, \"{path}\"], \"{path}\");";
    } else {
      source =
        "_SetValueToMap($componentInfoMap, \"{name}\", [{id}, $renderComponent{id}]);";
    }
    // clang-format on
    add_components << FormatStringWithRule(source, rule);
  }
  if (!is_below_version_1_1) {
    add_components << "let $componentInfoMap = _GetComponentInfo($parent);";
    return add_components.str();
  } else {
    LepusGenRuleMap rule = {{"addComponents", add_components.str()}};
    // clang-format off
    static std::string source =
        "let $componentInfoMap = {};"
        "{addComponents}";
    // clang-format on
    return FormatStringWithRule(source, rule);
  }
}

std::string TemplateParser::GenInstruction(const rapidjson::Value &instruction,
                                           const TemplateMap *const templates) {
  if (instruction.IsNull()) {
    return "";
  }
  DCHECK(instruction.IsObject());
  const rapidjson::Value &type = instruction["type"];
  if (type.IsNull()) {
  } else if (strcmp(type.GetString(), "node") == 0) {
    return GenElement(instruction);
  } else if (strcmp(type.GetString(), "if") == 0) {
    return GenIf(instruction);
  } else if (strcmp(type.GetString(), "repeat") == 0) {
    return GenRepeat(instruction);
  } else if (strcmp(type.GetString(), "include") == 0) {
    return GenInclude(instruction);
  } else if (strcmp(type.GetString(), "template") == 0) {
    return GenTemplate(instruction, templates != nullptr);
  } else if (strcmp(type.GetString(), "import") == 0) {
    return GenImport(instruction, templates != nullptr);
  } else if (strcmp(type.GetString(), "template_node") == 0) {
    return GenTemplateNode(instruction, templates);
  }

  if (Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                              LYNX_VERSION_1_1)) {
    // version >= 1.1
    if (strcmp(type.GetString(), "list") == 0) {
      return GenList(instruction);
    } else if (strcmp(type.GetString(), "component") == 0) {
      return GenElement(instruction);
    }
  }

  LOGE(
      "TemplateAssembler Error: ParseInstructionType error with "
      "undefined type "
      << type.GetString());
  return "";
}

std::string TemplateParser::GenIf(const rapidjson::Value &content) {
  DCHECK(content.IsObject());
  auto &condition = VALUE(content["condition"]);
  auto &consequent = content["consequent"];
  auto &alternate = content["alternate"];

  DCHECK(consequent.IsArray() && consequent.Size() == 1);
  DCHECK(alternate.IsNull() || (alternate.IsArray() && alternate.Size() == 1) ||
         alternate.IsObject());

  std::string condition_str = GenStatement(condition);
  rapid_value true_branch_chain(rapidjson::kArrayType);
  rapid_value false_branch_chain(rapidjson::kArrayType);
  if (content.HasMember("condition-branch") &&
      content["condition-branch"].IsArray()) {
    const auto &branch_chain = content["condition-branch"];
    true_branch_chain.CopyFrom(branch_chain, document.GetAllocator());
    false_branch_chain.CopyFrom(branch_chain, document.GetAllocator());
  }

  rapid_value branch_true(rapidjson::kObjectType);
  branch_true.AddMember("condition", condition_str, document.GetAllocator());
  branch_true.AddMember("value", true, document.GetAllocator());
  true_branch_chain.GetArray().PushBack(branch_true, document.GetAllocator());
  const_cast<rapidjson::Value &>(consequent[0])
      .AddMember("condition-branch", true_branch_chain,
                 document.GetAllocator());

  rapid_value branch_false(rapidjson::kObjectType);
  branch_false.AddMember("condition", condition_str, document.GetAllocator());
  branch_false.AddMember("value", false, document.GetAllocator());
  if (!alternate.IsNull()) {
    false_branch_chain.GetArray().PushBack(branch_false,
                                           document.GetAllocator());
    if (alternate.IsArray()) {
      const_cast<rapidjson::Value &>(alternate[0])
          .AddMember("condition-branch", false_branch_chain,
                     document.GetAllocator());
    } else {
      DCHECK(alternate.IsObject());
      const_cast<rapidjson::Value &>(alternate).AddMember(
          "condition-branch", false_branch_chain, document.GetAllocator());
    }
  }

  // alternate
  std::stringstream alternate_content;
  if (alternate.IsArray()) {
    alternate_content << GenInstruction(alternate[0]);
  } else {
    alternate_content << GenInstruction(alternate);
  }
  // clang-format off
  LepusGenRuleMap rule = {
      {"condition", condition_str},
      {"consequent", GenInstruction(consequent[0])},
      {"alternate", alternate_content.str()}
  };

  static std::string source =
      "if ({condition}) {"
          "{consequent}"
      "} else {"
          "{alternate}"
      "}";
  // clang-format on
  // return FormatStringWithRule(source, rule);
  std::string if_content = FormatStringWithRule(source, rule);

  if (IsSlot(content)) {
    return GenPlugNode(content, if_content, false);
  }
  return if_content;
}

std::string TemplateParser::GenRepeat(const rapidjson::Value &content) {
  DCHECK(content.IsObject());
  auto &object = VALUE(content["for"]);
  auto &variable = VALUE(content["item"]);
  auto &index = VALUE(content["index"]);
  auto &block = content["body"];

  // clang-format off
  LepusGenRuleMap rule = {
      {"obj", GenStatement(object)},
      {"index", index.GetString()},
      {"item", variable.GetString()},
      {"statements", GenInstruction(block)}
  };
  // clang-format on

  if (block.HasMember("attrs") && block["attrs"].IsObject() &&
      block["attrs"].HasMember(":lazy-load-count") &&
      block["attrs"][":lazy-load-count"].IsString()) {
    auto &lazy_load_count = block["attrs"][":lazy-load-count"];

    const char *c_lazy_load_count = lazy_load_count.GetString();
    for (int i = 0; i < lazy_load_count.GetStringLength(); i++) {
      if (!(c_lazy_load_count[i] >= '0' && c_lazy_load_count[i] <= '9')) {
        THROW_ERROR_MSG_WITH_LOC(
            "lazy-load-count should be const number like "
            "lazy-load-count=\"3\".\n",
            LOC((content["for"])));
        DCHECK(false);
      }
    }
    rule.insert({"lazyLoadCount", c_lazy_load_count});
    // use body eid as unique identification
    std::stringstream eid;
    eid << sElementIdGenerator + 1;
    rule.insert({"eid", eid.str()});
    rule.insert({"component",
                 current_component_ == current_page_ ? "$page" : "$component"});

    // clang-format off
    static std::string source_lazy =
        "{"
            "let $object = {obj};"
            "let $length = _GetLazyLoadCount($kTemplateAssembler, {component}, {eid}, _GetLength($object), {lazyLoadCount});"
            "for (let {index} = 0; {index} < $length; ++{index}) {"
                "let {item} = _IndexOf($object, {index});"
                "{statements}" // statements
            "}"
        "}";
    // clang-format on
    return FormatStringWithRule(source_lazy, rule);
  }

  // clang-format off
   std::string source;
   if (generator_options_.enable_tt_for_full_version && compile_options_.enable_lepus_ng_)
   {
     generator_options_.has_tt_for_command = true;
       source =
      "{"
          "let $object = {obj};"
          "$ttForRenderList($object ,function($itr_index, $key, $val) {"
                "let {index} = $key;"
                "let {item} = $val;"
                "{statements}"
          "});"
      "}";
   }else{
        source =
        "{"
          "let $object = {obj};"
          "for (let {index} = 0; {index} < _GetLength($object); ++{index}) {"
              "let {item} = _IndexOf($object, {index});"
              "{statements}" // statements
          "}"
      "}";
   }
  // clang-format on
  return FormatStringWithRule(source, rule);
}

std::string TemplateParser::GenElement(const rapidjson::Value &element) {
  auto &tag_name = element["tagName"]["value"];
  // replace <p> to <text>
  if (strcmp(tag_name.GetString(), "p") == 0) {
    const_cast<rapidjson::Value *>(&tag_name)->SetString("text");
  }

  bool is_text = (strcmp(tag_name.GetString(), "text") == 0 ||
                  strcmp(tag_name.GetString(), "inline-text") == 0) &&
                 !IsComponent(tag_name.GetString());
  bool is_image = strcmp(tag_name.GetString(), "image") == 0;
  bool is_view = strcmp(tag_name.GetString(), "view") == 0;

  // if <text> wrap in <p> <text>, it should be map to <inline-text>
  if (is_text && text_count_ > 0) {
    const_cast<rapidjson::Value *>(&tag_name)->SetString("inline-text");
  }
  // if <image> wrap in <p> <text>, it should be map to <inline-image>
  if (is_image && text_count_ > 0) {
    const_cast<rapidjson::Value *>(&tag_name)->SetString("inline-image");
  }

  int saved_text_count = text_count_;
  if (is_text) {
    text_count_++;
  } else if (is_view) {
    text_count_ = 0;
  }

  auto result = GenRawElement(element);
  text_count_ = saved_text_count;
  return result;
}

std::string TemplateParser::GenRawElement(const rapidjson::Value &element) {
  DCHECK(element.IsObject());
  auto &tag = VALUE(element["tagName"]);
  DCHECK(tag.IsString());
  std::string tag_name = tag.GetString();

  // Check if element is a static or dynamic component that render in a template
  // renderer function
  if (is_in_template_render_ &&
      (template_helper_->MaybeKindOfComponent(current_template_, tag_name) ||
       IsComponentIs(element))) {
    template_helper_->RecordTemplateHasComponentTag(current_template_);
    if (IsSlot(element)) {
      return GenComponentPlugInTemplate(element);
    } else {
      return GenComponentNodeInTemplate(element);
    }
  }

  // Check if element is a static component
  if (IsComponent(tag_name) || IsComponentIs(element)) {
    if (IsSlot(element)) {
      return GenComponentPlug(element);
    } else {
      return GenComponentNode(element);
    }
  }

  // If engineVersion >= 1.6. Gen dynamic compnent parser.
  if (IsDynamicComponent(tag_name)) {
    if (Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                LYNX_VERSION_1_6) &&
        compile_options_.enable_dynamic_component_) {
      if (IsSlot(element)) {
        return GenDynamicComponentPlug(element);
      } else {
        return GenDynamicComponentNode(element);
      }
    } else {
      LOGW(
          "[Warning]: use dynamic component when engine version "
          "< 1.6 or compile option enableDynamicComponent disable");
      return "";
    }
  }

  // Check if is we can generate slot content for component
  if (IsSlot(element)) {
    return GenElementPlug(element);
  }

  return GenElementNode(element);
}

std::string TemplateParser::GenElementSlot(const rapidjson::Value &slot) {
  DCHECK(current_template_);
  auto &attrs = slot["attrs"];
  std::string slot_name =
      attrs["name"].IsObject() ? VALUE(attrs["name"]).GetString() : "";
  if (slot_name.empty()) {
    slot_name = defaultSlotName;
  }

  SlotConditionChainVec chain;
  if (slot.HasMember("condition-branch") &&
      slot["condition-branch"].IsArray()) {
    auto &chain_array = slot["condition-branch"];
    for (auto it = chain_array.Begin(); it != chain_array.End(); ++it) {
      std::string name = std::string((*it)["condition"].GetString());
      bool value = (*it)["value"].GetBool();
      chain.push_back(std::make_pair(name, value));
    }
  }

  if (current_template_->HasSlotInHistory(slot_name, chain)) {
    std::stringstream error;
    error << "slot name: " << slot_name << " is duplicate!";
    THROW_ERROR_MSG(error.str().c_str());
  }

  LepusGenRuleMap rule = {{"slotId", GenStatement(slot_name)}};
  static std::string create_slot =
      "$child = _CreateVirtualSlot({slotId});"
      "_AppendChild($parent, $child);";

  current_template_->AddSlotToHistory(slot_name, chain);

  return FormatStringWithRule(create_slot, rule);
}

std::string TemplateParser::GenElementPlug(const rapidjson::Value &element) {
  auto content =
      element.IsObject() ? GenElementNode(element) : GenRawText(element);
  return GenPlugNode(element, content, is_in_template_render_);
}

std::string TemplateParser::GenList(const rapidjson::Value &element) {
  return ListParser{*this, sElementIdGenerator++}.GenList(element);
}

std::string TemplateParser::GenComponentProps(const rapidjson::Value &element) {
  std::stringstream ss;
  ss << "{";
  for (auto iter = element.GetObject().MemberBegin();
       iter != element.GetObject().MemberEnd(); ++iter) {
    ss << "\"" << iter->name.GetString() << "\""
       << ":" << GenStatement(VALUE(iter->value).GetString()) << ",";
  }
  ss << "}";
  return ss.str();
}

std::string TemplateParser::GenComponentEvent(const rapidjson::Value &element) {
  if (element.IsObject() && element.GetObject().MemberCount() == 0) {
    return "null";
  }
  if (element.IsArray() && element.GetArray().Size() == 0) {
    return "null";
  }

  rapid_value events(rapidjson::kArrayType);
  if (element.IsArray()) {
    for (rapidjson::Value::ConstValueIterator itr = element.Begin();
         itr != element.End(); ++itr) {  // Ok`
      rapid_value anEvent(rapidjson::kObjectType);
      anEvent.AddMember("type", std::string((*itr)["type"].GetString()),
                        document.GetAllocator());
      anEvent.AddMember("name", std::string((*itr)["name"].GetString()),
                        document.GetAllocator());
      auto &value = VALUE((*itr)["value"]);
      const auto &value_str = std::string(value.GetString());
      if (IsVariableString(value.GetString())) {
        std::string script =
            "{{" + value_str.substr(2, value_str.find('.') - 2) + "}}";
        anEvent.AddMember("script", script, document.GetAllocator());
      }
      anEvent.AddMember("value", value_str, document.GetAllocator());
      events.GetArray().PushBack(anEvent, document.GetAllocator());
    }
  }

  return GenStatement(events);
}

std::string TemplateParser::GenElementNode(const rapidjson::Value &element,
                                           bool should_gen_children) {
  // Create virtual node
  auto &tag_name = element["tagName"]["value"];
  // Check if element is slot
  std::string tag_name_str = tag_name.GetString();
  if ("slot" == tag_name_str) {
    return GenElementSlot(element);
  }
  auto &classes = element["className"];
  auto &styles = element["style"];
  auto &attrs = element["attrs"];
  auto &dataset = element["dataset"];
  auto &events = element["events"];
  auto &children = element["children"];
  auto &id = element["elemId"];
  auto &gesture = element["gesture"];
  bool is_block_element = strcmp(tag_name.GetString(), "block") == 0;
  // fallback node in dynamic component will be considered as blocks
  if (need_handle_fallback_ && IsSlot(element) &&
      strcmp(tag_name.GetString(), "fallback") == 0) {
    is_block_element = true;
  }

  // clang-format off
  LepusGenRuleMap rule = {
      {"createNode", ""},
      {"setId", ""},
      {"setAttributes", ""},
      {"setClasses", ""},
      {"setStyles", ""},
      {"setDataSet",""},
      {"setEvents", ""},
      {"setGesture", ""},
      {"setChildren", ""},
  };
  // clang-format on
  // Block element is a element that do not exist in the dom structure.
  if (!is_block_element) {
    // clang-format off
    static LepusGenRuleMap create_rule = {
        {"tagName", ""},
    };
    static std::string create_source =
        "$child = _CreateVirtualNode(\"{tagName}\", {eid});"
        "_AppendChild($parent, $child);";
    create_rule["tagName"] = tag_name.GetString();
    std::stringstream eid;
    eid << sElementIdGenerator++;
    create_rule["eid"] = eid.str();
    // clang-format on
    rule["createNode"] = FormatStringWithRule(create_source, create_rule);
    rule["setId"] = GenId(id);
    rule["setAttributes"] = GenAttributes(attrs);
    rule["setClasses"] = GenClasses(classes);
    rule["setStyles"] = GenStyles(styles);
    rule["setDataSet"] = GenDataSet(dataset);
    rule["setEvents"] = GenEvents(events);
    rule["setGesture"] = GenGestures(gesture);
  }

  // Append children
  if (should_gen_children && children.IsArray() && children.Size() > 0) {
    if (element.HasMember("condition-branch") &&
        element["condition-branch"].IsArray()) {
      rapid_value &children_no_const = const_cast<rapid_value &>(children);
      for (auto it = children_no_const.Begin(); it != children_no_const.End();
           ++it) {
        if (it->IsObject()) {
          rapid_value new_chain(rapidjson::kArrayType);
          new_chain.CopyFrom(element["condition-branch"],
                             document.GetAllocator());
          it->AddMember("condition-branch", new_chain, document.GetAllocator());
        }
      }
    }

    // clang-format off
    static LepusGenRuleMap children_rule = {
        {"childrenContent", ""},
    };
    static std::string children_source =
        "{"
        "let $parent = $child;"
        "{childrenContent}"
        "}";

    static std::string block_children_source =
        "{childrenContent}";
    children_rule["tagName"] = tag_name.GetString();
    // clang-format on
    children_rule["childrenContent"] = GenChildrenInElement(children);

    rule["setChildren"] = FormatStringWithRule(
        is_block_element ? block_children_source : children_source,
        children_rule);
  }

  // clang-format off
  static std::string source =
      "{"
          "{createNode}"
          "{setId}"
          "{setAttributes}"
          "{setClasses}"
          "{setStyles}"
          "{setDataSet}"
          "{setEvents}"
          "{setGesture}"
          "{setChildren}"
      "}";
  // clang-format on

  return FormatStringWithRule(source, rule);
}

std::string TemplateParser::GenClasses(const rapidjson::Value &classes_obj) {
  if (!classes_obj.IsObject()) {
    return "";
  }

  const rapidjson::Value &classes = VALUE(classes_obj);
  std::stringstream ss;
  if (classes.IsString() && IsAvailableString(classes)) {
    const char *input = classes.GetString();
    size_t input_length = classes.GetStringLength();
    std::string maybe_class;
    static std::string source1 = "_SetClassTo($child, {class});";
    static std::string source2 = "_SetStaticClassTo($child, {class});";

    static LepusGenRuleMap rule = {
        {"class", ""},
    };
    // Split by space
    std::list<std::string> maybe_classes;
    bool inside_statement = false;
    // The count exclude the {{}} for statement
    size_t left_bracket_count = 0;
    size_t maybe_class_start_index = 0;
    for (int i = 0; i < input_length; ++i) {
      char c = input[i];
      switch (c) {
        case ' ':
          if (!inside_statement && i != maybe_class_start_index) {
            maybe_classes.push_back(std::string(input + maybe_class_start_index,
                                                i - maybe_class_start_index));
            maybe_class_start_index = i + 1;
          }
          break;
        case '{': {
          if (i + 1 >= input_length) DCHECK(false);
          if (inside_statement) DCHECK(false);
          if (!inside_statement && input[i + 1] == '{') {
            inside_statement = true;
            i++;
          } else {
            left_bracket_count++;
          }
          break;
        }
        case '}': {
          if (left_bracket_count > 0) {
            left_bracket_count--;
          } else if (inside_statement && i + 1 < input_length &&
                     input[i + 1] == '}') {
            i++;
            inside_statement = false;
          } else {
            DCHECK(false);
          }
          break;
        }
        default:
          break;
      }
    }
    if (maybe_class_start_index <= input_length - 1) {
      maybe_classes.push_back(
          std::string(input, maybe_class_start_index, input_length));
    }
    for (const std::string &maybe_class : maybe_classes) {
      if (maybe_class.empty()) continue;
      rule["class"] = GenStatement(maybe_class);
      // Treat external classes as variable classes.
      if (IsVariableString(maybe_class) ||
          JsonArrayContains(current_component_->external_classes(),
                            maybe_class)) {
        ss << FormatStringWithRule(source1, rule);
      } else {
        ss << FormatStringWithRule(source2, rule);
      }
    }
  } else {
    THROW_ERROR_MSG_WITH_LOC("class name is not valid string",
                             LOC(classes_obj));
  }
  return ss.str();
}

std::string TemplateParser::GenStyles(const rapidjson::Value &styles_obj) {
  if (!styles_obj.IsObject()) {
    return "";
  }
  auto &styles = VALUE(styles_obj);

  if (!styles.IsString()) {
    THROW_ERROR_MSG_WITH_LOC("invalid styles", LOC(styles_obj));
  }
  if (!IsAvailableString(styles)) {
    return "";
  }
  std::stringstream ss;

  const std::string &style_str = styles.GetString();
  if (IsVariableString(style_str)) {
    return "_SetDynamicStyleTo($child, " + GenStatement(style_str) + ");";
  }
  // clang-format off
  static std::string source1 =
      "_SetStyleTo($child, {key}, {value});";
  static std::string source2 =
      "_SetStaticStyleTo($child, {key}, {value});";
  static std::string source2_fiber =
      "_SetStaticStyleToByFiber($child, {key}, {pattern}, {value});";
  // clang-format on

  static LepusGenRuleMap rule = {
      {"key", ""},
      {"value", ""},
  };

  static LepusGenRuleMap static_style_rule_fiber = {
      {"key", ""},
      {"pattern", ""},
      {"value", ""},
  };

  // A small optimize:
  //    style: "font-size: {{aa}}; font-size: 100;"
  // will be optimized to
  //    style: "font-size: 100;"
  // optimization is invalid when
  //    style: "font-size: 100; font-size: {{aa}};"
  std::map<std::string, std::string> static_map;
  std::map<std::string, std::string> dynamic_map;
  auto splits = base::SplitStringByCharsOrderly<':', ';'>(styles.GetString());
  for (int i = 0; i + 1 < splits.size(); i = i + 2) {
    std::string key = base::TrimString(splits[i]);
    std::string value = base::TrimString(splits[i + 1]);
    if (IsVariableString(key) || IsVariableString(value)) {
      dynamic_map.insert({key, value});
    } else {
      static_map.insert({key, value});
      dynamic_map.erase(key);
    }
  }
  auto configs = tasm::CSSParserConfigs::GetCSSParserConfigsByComplierOptions(
      compile_options_);
  for (auto it = static_map.begin(); it != static_map.end(); ++it) {
    if (Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                FEATURE_CSS_VALUE_VERSION) &&
        compile_options_.enable_css_parser_) {
      auto tm = it->second;
      CSSPropertyID property_id =
          (CSSPropertyID)atoi(FormatCSSPropertyID(it->first).c_str());
      StyleMap css_values;

      tasm::UnitHandler::Process(property_id, lepus::Value(tm.c_str()),
                                 css_values, configs);
      for (auto css_value : css_values) {
        static_style_rule_fiber["key"] = std::to_string((int)css_value.first);
        static_style_rule_fiber["pattern"] =
            std::to_string((int)css_value.second.GetPattern());
        static_style_rule_fiber["value"] =
            lepus::lepusValueToString(css_value.second.GetValue());
        ss << FormatStringWithRule(source2_fiber, static_style_rule_fiber);
      }
    } else {
      std::string tm = it->first;
      rule["key"] = FormatCSSPropertyID(tm);
      tm = it->second;
      rule["value"] = GenStatement(tm);
      ss << FormatStringWithRule(source2, rule);
    }
  }
  for (auto it = dynamic_map.begin(); it != dynamic_map.end(); ++it) {
    rule["key"] = FormatCSSPropertyID(it->first);
    rule["value"] = GenStatement(it->second);
    ss << FormatStringWithRule(source1, rule);
  }
  return ss.str();
}

std::string TemplateParser::GenId(const rapidjson::Value &id_obj) {
  if (!id_obj.IsObject()) {
    return "";
  }

  const rapidjson::Value &id = VALUE(id_obj);
  if (id.IsString() && IsAvailableString(id)) {
    // clang-format off
    static LepusGenRuleMap rule = {
        {"id", ""},
    };
    static std::string source =
        "_SetId($child, {id});";
    // clang-format on

    rule["id"] = GenStatement(id);
    return FormatStringWithRule(source, rule);
  } else {
    THROW_ERROR_MSG_WITH_LOC("invalid id", LOC(id_obj));
  }
  return "";
}

std::string TemplateParser::GenAttributes(const rapidjson::Value &attrs) {
  std::stringstream ss;
  if (attrs.IsObject()) {
    // clang-format off
    static LepusGenRuleMap rule = {
        {"key", ""},
        {"value", ""}
    };
    static std::string source1 =
        "_SetStaticAttributeTo($child, \"{key}\", {value});";
    static std::string source2 =
        "_SetAttributeTo($child, \"{key}\", {value});";
    // clang-format on
    for (auto it = attrs.MemberBegin(); it != attrs.MemberEnd(); ++it) {
      DCHECK(it->name.IsString());
      if (strcmp(it->name.GetString(), ":lazy-load-count") == 0) {
        continue;
      }

      const auto &key = std::string(it->name.GetString());
      const auto &value = VALUE(it->value);
      rule["key"] = key;
      rule["value"] = GenStatement(value);
      if (compile_options_.enable_lepus_ng_) {
        lynx::base::ReplaceEscapeCharacterWithLiteralString(rule["value"]);
      }

      if (value.IsString() && IsVariableString(value.GetString())) {
        ss << FormatStringWithRule(source2, rule);
      } else {
        ss << FormatStringWithRule(source1, rule);
      }
    }
  }
  return ss.str();
}

std::string TemplateParser::GenDataSet(const rapidjson::Value &attrs) {
  std::stringstream ss;
  if (attrs.IsObject()) {
    // clang-format off
    static LepusGenRuleMap rule = {
        {"key", ""},
        {"value", ""}
    };
    static std::string source =
        "_SetDataSetTo($child, \"{key}\", {value});";
    // clang-format on

    for (auto it = attrs.MemberBegin(); it != attrs.MemberEnd(); ++it) {
      auto &value = VALUE(it->value);
      DCHECK(it->name.IsString());
      DCHECK(value.IsString() || value.IsBool());
      rule["key"] = it->name.GetString();
      rule["value"] = GenStatement(value);
      ss << FormatStringWithRule(source, rule);
    }

    if (generator_options_.enable_dataset_attrs_ &&
        attrs.MemberBegin() != attrs.MemberEnd()) {
      static size_t sDataSetIdGenerator = 0;
      std::string source_dataset =
          "{"
          "let $dataSet{id} = {};"
          "{val_code}"
          "_SetAttributeTo($child, \"dataset\", $dataSet{id});"
          "}";
      std::stringstream val_ss;
      for (auto it = attrs.MemberBegin(); it != attrs.MemberEnd(); ++it) {
        auto &value = VALUE(it->value);
        DCHECK(it->name.IsString());
        DCHECK(value.IsString() || value.IsBool());
        static std::string val_source = "$dataSet{id}.{key} = {value};";

        LepusGenRuleMap val_rule = {
            {"key", it->name.GetString()},
            {"value", GenStatement(value)},
            {"id", std::to_string(sDataSetIdGenerator)}};
        val_ss << FormatStringWithRule(val_source, val_rule);
      }
      LepusGenRuleMap rule_data_set = {
          {"id", std::to_string(sDataSetIdGenerator)},
          {"val_code", val_ss.str()},
      };
      ss << FormatStringWithRule(source_dataset, rule_data_set);
      sDataSetIdGenerator++;
    }
  }
  return ss.str();
}

std::string TemplateParser::GenEvents(const rapidjson::Value &events) {
  std::stringstream ss;

  if (events.IsArray()) {
    // clang-format off
    static LepusGenRuleMap rule = {
        {"type", ""},
        {"name", ""},
        {"value", ""}
    };
    static std::string kSourceEvent =
        "_SetStaticEventTo($child, \"{type}\", \"{name}\",{value});";
    // clang-format on
    constexpr const static char *kScriptEvent =
        "_SetScriptEventTo($child, \"{type}\", \"{name}\", {script}, "
        "{value});";

    size_t size = events.Size();
    for (int i = 0; i < size; ++i) {
      auto &event = events[i];
      if (event.IsObject()) {
        rule["type"] = event["type"].GetString();
        rule["name"] = event["name"].GetString();
        auto &value = VALUE(event["value"]);
        const auto &value_str = GenStatement(value);
        rule["value"] = value_str;

        if (compile_options_.enable_lepus_ng_ &&
            Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                    FEATURE_TEMPLATE_SCRIPT) &&
            IsVariableString(value.GetString())) {
          rule["script"] = value_str.substr(1, value_str.find('.') - 1);
          ss << FormatStringWithRule(kScriptEvent, rule);
        } else {
          ss << FormatStringWithRule(kSourceEvent, rule);
        }
      }
    }
  }
  return ss.str();
}

std::string TemplateParser::GenGestures(const rapidjson::Value &gesture) {
  std::stringstream ss;

  if (gesture.IsObject()) {
    static LepusGenRuleMap rule = {{"value", ""}, {"gesture", ""}};

    static std::string kSourceGesture = "__processGesture($child, {value});";
    auto &value = VALUE(gesture["value"]);
    const auto &value_str = GenStatement(value);
    rule["value"] = value_str;

    if (IsVariableString(value.GetString())) {
      rule["gesture"] = value_str.substr(1, value_str.find('.') - 1);
      ss << FormatStringWithRule(kSourceGesture, rule);
    }
  }
  return ss.str();
}

std::string TemplateParser::GenChildrenInElement(
    const rapidjson::Value &children) {
  DCHECK(children.IsArray());
  if (children.IsArray() && children.Size() > 0) {
    std::stringstream ss;

    // clang-format off
    LepusGenRuleMap rule = {
        {"children", ""},
    };
    static std::string source =
        "{children}";
    // clang-format on

    size_t size = children.Size();
    for (int i = 0; i < size; ++i) {
      auto &child = children[i];
      if (child.IsObject()) {
        ss << GenInstruction(child);
      } else if (child.IsString()) {
        ss << GenRawText(child);
      }
    }
    rule["children"] = ss.str();
    return FormatStringWithRule(source, rule);
  }
  return "";
}

std::string TemplateParser::GenChildrenInComponentElement(
    const rapidjson::Value &children, bool in_dynamic_component) {
  DCHECK(children.IsArray());
  if (!children.IsArray()) {
    LOGE("Empty children in component " << GetCurrentPath());
  }
  if (children.IsArray() && children.Size() > 0) {
    const bool current_state = need_handle_fallback_;
    need_handle_fallback_ =
        in_dynamic_component &&
        Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                FEATURE_DYNAMIC_COMPONENT_FALLBACK);

    std::stringstream ss;

    // clang-format off
    LepusGenRuleMap rule = {
        {"children", ""},
    };
    static std::string source =
        "{children}";

    // most count of default slot is 1
    size_t children_count = children.Size();

    for (int i = 0; i < children_count; ++i) {
      auto &child = children[i];
      if (child.IsObject() && (!child.HasMember("slot") || (child["slot"].IsString() && child["slot"].GetStringLength() == 0)) ) {
        rapidjson::Value& child_slot = const_cast<rapidjson::Value&>(child);
        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
        rapidjson::Value slot(rapidjson::kObjectType);
        rapidjson::Value value(defaultSlotName, allocator);
        rapidjson::Value loc(rapidjson::kObjectType);
        rapidjson::Value row(1);
        rapidjson::Value column(1);
        loc.AddMember(rapidjson::Value("row"), row, allocator);
        loc.AddMember(rapidjson::Value("column"), column, allocator);

        slot.AddMember(rapidjson::Value("value"), value, allocator);
        slot.AddMember(rapidjson::Value("loc"), loc, allocator);

        if (child_slot.HasMember("slot")) {
          child_slot.RemoveMember(rapidjson::Value("slot"));
        }
        child_slot.AddMember(rapidjson::Value("slot"), slot, allocator);
      }
      if (child.IsObject() && child["slot"].IsObject() && VALUE(child["slot"]).GetStringLength() == 0){
        rapidjson::Value& child_slot = const_cast<rapidjson::Value&>(child);
        VALUE(child_slot["slot"]) = rapidjson::Value::StringRefType(defaultSlotName);
      }
    }

    for (int i = 0; i < children_count; ++i) {
      auto &child = children[i];
      if (child.IsObject()) {
        ss << GenInstruction(child);
      }
      if (child.IsString()) {
        ss << GenElementPlug(child);
      }
    }

    rule["children"] = ss.str();

    need_handle_fallback_ = current_state;
    return FormatStringWithRule(source, rule);
  }
    return "";
}

std::string TemplateParser::GenRawText(const rapidjson::Value &value) {
  DCHECK(value.IsString());
  // clang-format off
  static LepusGenRuleMap rule = {
      {"value", ""},
  };
  // Static attribute
  static std::string source1 =
      "{"
          "$child = _CreateVirtualNode(\"raw-text\", {eid});"
          "_SetStaticAttributeTo($child, \"text\", {value});"
          "_AppendChild($parent, $child);"
      "}";
  // Dynamic attribute
  static std::string source2 =
      "{"
          "$child = _CreateVirtualNode(\"raw-text\", {eid});"
          "_SetAttributeTo($child, \"text\", {value});"
          "_AppendChild($parent, $child);"
      "}";
  // clang-format on
  rule["value"] = GenStatement(value);
  if (compile_options_.enable_lepus_ng_) {
    lynx::base::ReplaceEscapeCharacterWithLiteralString(rule["value"]);
  }
  std::stringstream eid;
  eid << sElementIdGenerator++;
  rule["eid"] = eid.str();
  if (IsVariableString(value.GetString())) {
    return FormatStringWithRule(source2, rule);
  } else {
    return FormatStringWithRule(source1, rule);
  }
}

std::string TemplateParser::GenTemplate(const rapidjson::Value &tem,
                                        bool is_include) {
  if (is_include) {
    return "";
  }
  DCHECK(tem.IsObject());
  auto &name = tem["name"]["value"];
  DCHECK(name.IsString());
  auto &children = tem["children"];
  DCHECK(children.IsArray());
  if (tem.HasMember("condition-branch") && tem["condition-branch"].IsArray() &&
      children.IsArray() && children.Size() > 0) {
    rapid_value &children_no_const = const_cast<rapid_value &>(children);
    for (auto it = children_no_const.Begin(); it != children_no_const.End();
         ++it) {
      if (it->IsObject()) {
        rapid_value new_chain(rapidjson::kArrayType);
        new_chain.CopyFrom(tem["condition-branch"], document.GetAllocator());
        it->AddMember("condition-branch", new_chain, document.GetAllocator());
      }
    }
  }
  auto unique_tem_ptr = std::make_shared<Template>(
      name.GetString(), const_cast<rapidjson::Value *>(&children));

  // Template is defined in fragment, so add it to current fragment.
  auto tem_obj = unique_tem_ptr.get();
  current_fragment_->AddTemplate(unique_tem_ptr);
  current_fragment_->AddLocalTemplate(unique_tem_ptr);

  // Record available components for template
  template_helper_->RecordAvailableInfo(tem_obj, current_component_);

  return "";
}

std::string TemplateParser::GenTemplateDynamicRendererInFragment(
    Fragment *fragment) {
  std::stringstream block;
  auto dependent_templates = fragment->templates();
  for (auto it : fragment->dependent_fragments()) {
    for (auto iit : it.second->templates()) {
      dependent_templates[iit.first] = iit.second;
    }
  }
  int i = 0;
  for (auto it : dependent_templates) {
    auto &dt = it.second;

    // id
    std::stringstream id;
    id << dt->id();

    LepusGenRuleMap rule = {{"ID", id.str()}, {"name", dt->path()}};
    if (i == 0) {
      static std::string source =
          "if ($is == '{name}') {"
          "$renderTemplate{ID}($parent, $componentInfoMap, $data, "
          "$recursive);"
          "}";
      block << FormatStringWithRule(source, rule);
    } else {
      static std::string source =
          "else if ($is == '{name}') {"
          "$renderTemplate{ID}($parent, $componentInfoMap, $data, "
          "$recursive);"
          "}";
      block << FormatStringWithRule(source, rule);
    }
    i++;
  }

  // id
  std::stringstream id;
  id << fragment->uid();

  // clang-format off
  LepusGenRuleMap rule = {
      {"ID", id.str()},
      {"block", block.str()}
  };
  static std::string source =
      "$dynamicRenderTemplateInFragment{ID} = function($is, $parent, $componentInfoMap, $data, $recursive) {"
          "{block}"
      "};";
  // clang-format on
  return FormatStringWithRule(source, rule);
}

std::string TemplateParser::GenTemplateRenderer(Template *templ) {
  // Enter template scope
  TemplateScope template_scope(this, templ);
  is_in_template_render_ = true;

  // Id
  std::stringstream id;
  id << templ->id();

  // Generate instruction for content
  std::stringstream content;
  auto &ttml = templ->ttml();
  if (ttml.IsArray()) {
    for (int i = 0; i < ttml.Size(); ++i) {
      content << GenInstruction(ttml[i]);
    }
  } else {
    LOGE("Empty ttml in template " << templ->path());
  }

  // Defines data
  std::stringstream data_defines;
  for (auto var_name : templ->variables_in_use()) {
    // clang-format off
      LepusGenRuleMap rule = {
          {"varName", var_name.str()},
      };
      static std::string source =
          "let {varName} = $data.{varName};";
    // clang-format on
    data_defines << FormatStringWithRule(source, rule);
  }

  // clang-format off
  LepusGenRuleMap rule = {
      {"ID", id.str()},
      {"createNode", content.str()},
      {"dataDefine", data_defines.str()},
      {"defineComponentInfo", template_helper_->HasComponentTag(templ) ? "let $componentInfo = null;" : ""}
  };
  static std::string source =
      "$renderTemplate{ID} = function($parent, $componentInfoMap, $data, $recursive) {"
          "{dataDefine}"
          "{defineComponentInfo}"
          "let $child = null;"
          "{createNode}"
      "};";
  // clang-format on
  is_in_template_render_ = false;
  return FormatStringWithRule(source, rule);
}

std::string TemplateParser::GenTemplateNode(
    const rapidjson::Value &template_node, const TemplateMap *const templates) {
  // TODO: template node has data scope
  DCHECK(template_node.IsObject());
  std::stringstream ss;
  auto &name = VALUE(template_node["is"]);
  bool is_dynamic = IsVariableString(name.GetString());
  DCHECK(name.IsString());
  // data
  auto &data = VALUE(template_node["data"]);
  std::stringstream param;
  if (data.IsString()) {
    std::string data_str = base::TrimString(data.GetString());
    static const char *kErrorMsg =
        "TTML Parse Error: data on <template> now support {{...let}} or "
        "{{key:value}}!\n";
    if (!IsVariableString(data_str)) {
      THROW_ERROR_MSG(kErrorMsg);
      DCHECK(false);
    }
    auto index = data_str.find("...");
    if (index != std::string::npos) {
      // Extract var from {{...var}}
      param << data_str.substr(index + 3, data_str.length() - index - 5);
    } else if (data_str.find(":") != std::string::npos) {
      // Extract {key:value} from {{key:value}}
      auto temp = data_str.substr(1, data_str.length() - 2);
      // Fill key with "" (quotation mark) if needed
      bool need_lqm = true;
      bool need_rqm = false;
      for (char &c : temp) {
        switch (c) {
          case '"':
          case '\'':
            if (need_lqm)
              need_lqm = false;
            else if (need_rqm)
              need_rqm = false;
            break;
          case '{':
          case ',':
            need_lqm = true;
            need_rqm = true;
            break;
          case ' ':
            if (!need_lqm && need_rqm) {
              param << '"';
              need_rqm = false;
            }
            break;
          case '}':
          case ':':
            if (need_lqm) {
              param << '"';
              need_lqm = false;
            } else if (need_rqm) {
              param << '"';
              need_rqm = false;
            }
            break;
          default:
            if (need_lqm) {
              param << '"';
              need_lqm = false;
            }
            break;
        }
        param << c;
      }
    } else {
      std::string var(data_str, 2, data_str.size() - 4);
      param << "{'" << var << "':" << var << "}";
    }

    // For mark variable in use
    std::stringstream mark_variable_test;
    mark_variable_test << "{{" << param.str() << "}}";
    GenStatement(mark_variable_test.str());
  }

  // Find template in page scope, otherwise find in app scope
  if (!is_dynamic) {
    Template *tem = nullptr;
    // if this template_node is in included file,search included fragment's
    // templates, not current_fragment's templates
    if (templates != nullptr) {
      auto tem_finder = templates->find(name.GetString());
      if (tem_finder != templates->end()) {
        tem = tem_finder->second.get();
        std::shared_ptr<Template> shared_tem_ptr = tem_finder->second;
        current_fragment_->AddIncludeTemplate(shared_tem_ptr);
        template_helper_->RecordAvailableInfo(tem, current_component_);
      }
    } else {
      if (current_fragment_->HasTemplate(name.GetString())) {
        tem = current_fragment_->GetTemplate(name.GetString());
      }
    }
    DCHECK(tem != nullptr);
    if (tem == nullptr) {
      LOGW("Cannot find template " << name.GetString() << " in "
                                   << current_fragment_->path());
      return "";
    }
    // ID
    std::stringstream id;
    id << tem->id();

    // clang-format off
    LepusGenRuleMap rule = {
        {"ID", id.str()},
        {"param", param.str().empty() ? "null" : param.str()}
    };
    static std::string source =
        "$renderTemplate{ID}($parent, $componentInfoMap, {param}, $recursive);";
    // clang-format on
    return FormatStringWithRule(source, rule);
  } else {
    current_fragment_->set_has_dynamic_template(true);
    // ID
    std::stringstream current_fragment_id;
    current_fragment_id << current_fragment_->uid();
    // clang-format off
    LepusGenRuleMap rule = {
        {"ID", current_fragment_id.str()},
        {"param", param.str().empty() ? "null" : param.str()},
        {"is", GenStatement(name.GetString())}
    };
    static std::string source =
        "$dynamicRenderTemplateInFragment{ID}({is}, $parent, $componentInfoMap, {param}, $recursive);";
    // clang-format on
    return FormatStringWithRule(source, rule);
  }
}

void TemplateParser::GenFragment(const rapidjson::Value &frag) {
  DCHECK(frag.IsObject());
  auto &path = frag["path"];
  DCHECK(path.IsString());
  if (package_instance_->HasFragment(path.GetString())) {
    return;
  }
  if (opening_files_.find(path.GetString()) != opening_files_.end()) {
    LOGW(path.GetString() << " is being opening in a loop, will be stopped.");
    return;
  }
  opening_files_.insert(path.GetString());
  auto &ttml = package_instance_->GetTTMLHolder(path.GetString());
  // Create fragment
  auto fragment = std::make_shared<Fragment>(
      path.GetString(), const_cast<rapidjson::Value *>(&ttml));
  FragmentScope fragment_scope(this, fragment.get());
  // TODO(xiaopeng): no need to generate every instruction for fragment
  if (!ttml.IsArray()) {
    LOGE("Can't not find path: " << path.GetString());
    return;
  }
  for (int i = 0; i < ttml.Size(); ++i) {
    GenInstruction(ttml[i]);
  }
  // Register fragment to app
  package_instance_->RegisterFragment(fragment);
  opening_files_.erase(path.GetString());
}

std::string TemplateParser::GenImport(const rapidjson::Value &import,
                                      bool is_include) {
  if (is_include) {
    return "";
  }
  GenFragment(import);
  auto &path = import["path"];
  DCHECK(path.IsString());
  if (!package_instance_->HasFragment(path.GetString())) {
    LOGW(path.GetString() << " is not available! ");
    return "";
  }
  auto fragment = package_instance_->GetFragment(path.GetString());
  auto &local_templates = fragment->local_templates();
  for (auto it = local_templates.begin(); it != local_templates.end(); ++it) {
    auto tem = it->second;
    current_fragment_->AddTemplate(tem);
  }
  return "";
}

std::string TemplateParser::GenInclude(const rapidjson::Value &include) {
  GenFragment(include);
  auto &path = include["path"];
  DCHECK(path.IsString());
  if (!package_instance_->HasFragment(path.GetString())) {
    LOGW(path.GetString() << " is not available! ");
    return "";
  }
  if (including_chain_.find(path.GetString()) != including_chain_.end()) {
    LOGW(path.GetString() << " is being included in a loop, will be stopped.");
    return "";
  }
  including_chain_.insert(path.GetString());
  auto fragment = package_instance_->GetFragment(path.GetString());
  std::stringstream ss;
  auto &ttml = package_instance_->GetTTMLHolder(path.GetString());
  if (!ttml.IsArray()) {
    LOGE(path.GetString() << " can not be included.");
  }
  auto &templates = fragment->templates();
  for (int i = 0; i < ttml.Size(); ++i) {
    ss << GenInstruction(ttml[i], &templates);
  }
  including_chain_.erase(path.GetString());
  return ss.str();
}

std::string TemplateParser::GenComponentPlug(const rapidjson::Value &element) {
  return GenPlugNode(element, GenComponentNode(element), false);
}

std::string TemplateParser::GenComponentNode(
    const rapidjson::Value &json_component) {
  DCHECK(json_component.IsObject());
  DCHECK(json_component["tagName"].IsObject());
  auto &tag_name = VALUE(json_component["tagName"]);
  bool component_is = false;

  std::stringstream id;
  std::string component_name = tag_name.GetString();
  std::string render_command;
  std::string create_command;
  std::string data_processor_command;

  std::stringstream component_instance_id;
  component_instance_id << sComponentInstanceIdGenerator++;

  Component *component = nullptr;
  if (IsComponentIs(json_component)) {
    component_is = true;
    component_name = GenStatement(VALUE(json_component["is"]).GetString());
    static std::string create_source =
        "_CreateVirtualComponent($componentInfo[0], $kTemplateAssembler, "
        "{componentName}, $componentInfo[2], {componentInstanceId});";
    LepusGenRuleMap rule = {
        {"componentName", component_name},
        {"componentInstanceId", component_instance_id.str()},
    };
    create_command = FormatStringWithRule(create_source, rule);
    if (package_instance_->dsl() == PackageInstanceDSL::REACT) {
      static std::string data_processor_source = "_ProcessData($child);";
      data_processor_command =
          FormatStringWithRule(data_processor_source, rule);
    }
    static std::string render_source =
        "_MarkComponentHasRenderer($child);"
        "$componentInfo[1]($child, _GetComponentData($child), "
        "_GetComponentProps($child), $recursive);";
    render_command = FormatStringWithRule(render_source, rule);
  } else {
    std::stringstream id;
    // Get component
    std::string component_path = GetComponentPath(component_name);
    DCHECK(!component_path.empty());
    component = package_instance_->GetComponent(component_path);
    DCHECK(component);
    id << component->id();
    static std::string create_source =
        "_CreateVirtualComponent({ID}, $kTemplateAssembler, "
        "\"{componentName}\", \"{componentPath}\", {componentInstanceId});";
    LepusGenRuleMap rule = {
        {"ID", id.str()},
        {"componentName", component_name},
        {"componentPath", component_path},
        {"componentInstanceId", component_instance_id.str()},
    };
    create_command = FormatStringWithRule(create_source, rule);
    static std::string render_source =
        "_MarkComponentHasRenderer($child);"
        "$renderComponent{ID}($child, _GetComponentData($child), "
        "_GetComponentProps($child), $recursive);";
    render_command = FormatStringWithRule(render_source, rule);

    if (package_instance_->dsl() == PackageInstanceDSL::REACT) {
      static std::string data_processor_source = "_ProcessData($child);";
      data_processor_command =
          FormatStringWithRule(data_processor_source, rule);
    }
  }

  // Slot content
  const rapidjson::Value &slot_content = json_component["children"];
  auto slot_content_str = GenChildrenInComponentElement(slot_content, false);

  // Set props
  std::stringstream set_props_content;
  auto &props = json_component["attrs"];

  // Attributes recording, we need to remove those props in attributes
  auto attrs = SegregateAttrsFromPropsForComponent(props, set_props_content,
                                                   component_is, component);

  auto &classes = json_component["className"];
  auto &styles = json_component["style"];
  auto &dataset = json_component["dataset"];
  auto &events = json_component["events"];
  auto &elem_id = json_component["elemId"];

  LepusGenRuleMap rule = {
      {"genSlotContents", slot_content_str},
      {"setProps", set_props_content.str()},
      {"setId", GenId(elem_id)},
      {"setClasses", GenClasses(classes)},
      {"setStyles", GenStyles(styles)},
      {"setDataSet", GenDataSet(dataset)},
      {"setEvents", GenEvents(events)},
      {"componentName", component_name},
      {"createCommand", create_command},
      {"renderCommand", render_command},
      {"setAttributes", GenAttributes(attrs)},
      {"data_processor_command", data_processor_command},
  };
  if (component_is &&
      Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                              LYNX_VERSION_1_6) &&
      compile_options_.enable_dynamic_component_) {
    rule["genDynamicComponent"] =
        GenDynamicComponentNode(json_component, slot_content);
  } else {
    rule["genDynamicComponent"] = "// TODO \n";
  }
  // clang-format off
  static std::string component_source =
      "{"
          "let $child = {createCommand}"
          "let $childComponent = $child;"
          "{setProps}"
          "{setId}"
          "{setClasses}"
          "{setStyles}"
          "{setDataSet}"
          "{setEvents}"
          "{setAttributes}"
          "_AppendChild($parent, $child);"
          "{data_processor_command}"
          "if ($recursive) {"
              "{renderCommand}"
          "}"
          "{genSlotContents}"
      "}";
  static std::string component_is_source =
      "{"
          "let $componentInfo = $componentInfoMap[{componentName}];"
          "let $childComponent = null;"
          "if ($componentInfo && $componentInfo[0] >= 0) {"
              "// Child is a static component \n"
              + component_source +
          "} else {"
              "// Child is a dynamic component \n"
              "{genDynamicComponent}"
          "}"
      "}";
  // clang-format on
  return FormatStringWithRule(
      component_is ? component_is_source : component_source, rule);
}

std::string TemplateParser::GenDynamicComponentPlug(
    const rapidjson::Value &component) {
  return GenPlugNode(component, GenDynamicComponentNode(component), false);
}

std::string TemplateParser::GenDynamicComponentNode(
    const rapidjson::Value &json_component,
    const rapidjson::Value &slot_content) {
  DCHECK(json_component.IsObject());
  DCHECK(VALUE(json_component["tagName"]).IsString());
  std::string tag_name = VALUE(json_component["tagName"]).GetString();
  std::string entry_name;

  if (IsComponentIs(json_component)) {
    DCHECK(json_component.HasMember("is"));
    DCHECK(VALUE(json_component["is"]).IsString());
    entry_name = GenStatement(VALUE(json_component["is"]).GetString());
  } else {
    entry_name = GenStatement(VALUE(json_component["tagName"]).GetString());
  }

  std::stringstream id;
  id << sComponentIdGenerator++;

  // Slot content
  auto slot_content_str = GenChildrenInComponentElement(
      slot_content.IsArray() ? slot_content : json_component["children"], true);

  // Set props
  std::stringstream set_props_content;
  auto &props = json_component["attrs"];
  // segregate attributes from props
  auto attrs = SegregateAttrsFromPropsForComponent(props, set_props_content);

  auto &classes = json_component["className"];
  auto &styles = json_component["style"];
  auto &dataset = json_component["dataset"];
  auto &events = json_component["events"];
  auto &elem_id = json_component["elemId"];
  std::stringstream component_instance_id;
  component_instance_id << sComponentInstanceIdGenerator++;

  LepusGenRuleMap rule = {
      {"ID", id.str()},
      {"genSlotContents", std::move(slot_content_str)},
      {"setProps", set_props_content.str()},
      {"setAttributes", GenAttributes(attrs)},
      {"setId", GenId(elem_id)},
      {"setClasses", GenClasses(classes)},
      {"setStyles", GenStyles(styles)},
      {"setDataSet", GenDataSet(dataset)},
      {"setEvents", GenEvents(events)},
      {"entryName", entry_name},
      {"componentInstanceId", component_instance_id.str()},
  };
  // clang-format off
  static std::string source =
      "{"
          "let $child = _CreateDynamicVirtualComponent({ID}, $kTemplateAssembler, {entryName}, {componentInstanceId});"
          "if ($child) {"
              "let $childComponent = $child;"
              "{setProps}"
              "{setId}"
              "{setClasses}"
              "{setStyles}"
              "{setDataSet}"
              "{setEvents}"
              "{setAttributes}"
              "_AppendChild($parent, $child);"
              "if ($recursive) {"
                  "_MarkComponentHasRenderer($child);"
                  "_RenderDynamicComponent({entryName}, $kTemplateAssembler, $child, _GetComponentData($child), _GetComponentProps($child), $recursive);"
              "}"
              "{genSlotContents}"
          "}"
      "}";
  // clang-format on
  return FormatStringWithRule(source, rule);
}

std::string TemplateParser::GenComponentPlugInTemplate(
    const rapidjson::Value &element) {
  return GenPlugNode(element, GenComponentNodeInTemplate(element), true);
}

std::string TemplateParser::GenComponentNodeInTemplate(
    const rapidjson::Value &component_instance) {
  DCHECK(component_instance.IsObject());
  DCHECK(component_instance["tagName"].IsObject());
  std::string component_name =
      GenStatement(VALUE(component_instance["tagName"]).GetString());
  bool component_is = false;
  if (IsComponentIs(component_instance)) {
    component_is = true;
    component_name = GenStatement(VALUE(component_instance["is"]).GetString());
  }
  // Slot content
  const rapidjson::Value &slot_content = component_instance["children"];
  auto component_slot_content =
      GenChildrenInComponentElement(slot_content, false);
  auto dynamic_component_slot_content =
      Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                              FEATURE_DYNAMIC_COMPONENT_FALLBACK)
          ? GenChildrenInComponentElement(slot_content, true)
          : component_slot_content;

  // Set props
  std::stringstream set_props_content;
  auto &props = component_instance["attrs"];

  // Attributes recording.
  auto attrs = SegregateAttrsFromPropsForComponent(props, set_props_content);

  // Gen node
  std::stringstream gen_node;
  gen_node << GenElementNode(component_instance, false);

  auto &classes = component_instance["className"];
  auto &styles = component_instance["style"];
  auto &dataset = component_instance["dataset"];
  auto &events = component_instance["events"];
  auto &elem_id = component_instance["elemId"];

  std::stringstream id;
  id << sComponentIdGenerator++;

  std::stringstream component_instance_id;
  component_instance_id << sComponentInstanceIdGenerator++;

  LepusGenRuleMap rule = {
      {"ID", id.str()},
      {"genSlotContents", std::move(component_slot_content)},
      {"genDynamicComponentSlotContents",
       std::move(dynamic_component_slot_content)},
      {"setProps", set_props_content.str()},
      {"setAttributes", GenAttributes(attrs)},
      {"setId", GenId(elem_id)},
      {"setClasses", GenClasses(classes)},
      {"setStyles", GenStyles(styles)},
      {"setDataSet", GenDataSet(dataset)},
      {"setEvents", GenEvents(events)},
      {"componentName", component_name},
      {"genNode", gen_node.str()},
      {"cid", component_instance_id.str()},
  };
  // clang-format off
  static std::string static_source =
      "// Child is a static component \n"
      "$child = _CreateVirtualComponent($componentInfo[0], $kTemplateAssembler, {componentName}, $componentInfo[2], {cid});"
      "let $childComponent = $child;"
      "{setProps}"
      "{setId}"
      "{setClasses}"
      "{setStyles}"
      "{setDataSet}"
      "{setEvents}"
      "{setAttributes}"
      "_AppendChild($parent, $child);"
      "if ($recursive) {"
          "_MarkComponentHasRenderer($child);"
          "$componentInfo[1]($child, _GetComponentData($child), _GetComponentProps($child), $recursive);"
      "}"
      "{genSlotContents}";

  static std::string dynamic_source =
      "// Child is a dynamic component \n"
      "{"
          "$child = _CreateDynamicVirtualComponent({ID}, $kTemplateAssembler, {componentName}, {cid});"
          "if ($child) {"
              "let $childComponent = $child;"
              "{setProps}"
              "{setId}"
              "{setClasses}"
              "{setStyles}"
              "{setDataSet}"
              "{setEvents}"
              "{setAttributes}"
              "_AppendChild($parent, $child);"
              "if ($recursive) {"
                  "_MarkComponentHasRenderer($child);"
                  "_RenderDynamicComponent({componentName}, $kTemplateAssembler, $child, _GetComponentData($child), _GetComponentProps($child), $recursive);"
              "}"
              "{genDynamicComponentSlotContents}"
        "}";

  bool is_ge_16 =
      Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                              FEATURE_DYNAMIC_COMPONENT_VERSION) &&
      compile_options_.enable_dynamic_component_;

  std::string source =
      "{"
          "$componentInfo = $componentInfoMap[{componentName}];"
          "if ($componentInfo && $componentInfo[0] >= 0) {"
           + static_source +
          "}" + (is_ge_16 || !component_is ? " else {" : "")
            + (is_ge_16 ? dynamic_source : "")
            + (!component_is ? "// Child is a node \n {genNode}" : "")
            + (is_ge_16 ? "}" : "")
          + (is_ge_16 || !component_is ? "}" : "") +
      "}";

  // clang-format on
  return FormatStringWithRule(source, rule);
}

std::string TemplateParser::GenComponentRenderer(Component *component) {
  DCHECK(component);
  // Enter component scope
  ComponentScope component_scope(this, component);

  // Gen Instructions
  std::stringstream id;
  id << component->id();
  std::stringstream content;
  auto &ttml = component->ttml();
  DCHECK(ttml.IsArray());
  if (!ttml.IsArray()) {
    LOGE("Empty ttml in page " << current_component_->path());
  }
  for (int i = 0; i < ttml.Size(); ++i) {
    content << GenInstruction(ttml[i]);
  }

  for (auto &iter : current_fragment_->templates()) {
    auto tem = iter.second;
    // Register and Trigger
    // TODO(zhoupeng): seem to be no need to add template to current_component_
    current_component_->AddTemplate(tem);
    template_helper_->RecordAvailableInfo(tem.get(), current_component_);
    if (tem->codes().empty()) {
      tem->set_codes({GenTemplateRenderer(tem.get())});
    }
  }

  for (auto &iter : current_fragment_->include_templates()) {
    auto tem = iter.second;
    // Register and Trigger
    template_helper_->RecordAvailableInfo(tem.get(), current_component_);
    if (tem->codes().empty()) {
      tem->set_codes({GenTemplateRenderer(tem.get())});
    }
  }

  // Gen component mould
  // Needs to be executed after GenInstruction, because only used variables in
  // data and properties can be encoded.
  GenComponentMould(component);

  // Template Script
  std::string script_defines = GenScriptMapDefines(component, "$component");

  // Template API
  std::string templateFunctions = GenTemplateFunctions(component);

  std::unordered_set<base::String> variable_defines;
  // Props define
  std::stringstream data_defines;
  std::stringstream props_defines;
  auto &props = component->props();
  if (props.IsObject()) {
    for (auto it = props.GetObject().begin(); it != props.GetObject().end();
         ++it) {
      base::String var_name(it->name.GetString());
      if (component->IsVariableInUse(it->name.GetString()) &&
          variable_defines.find(var_name) == variable_defines.end()) {
        // clang-format off
        LepusGenRuleMap rule = {
            {"varName", var_name.str()},
        };
        static std::string source =
            "let {varName} = $props.{varName};";
        // clang-format on
        props_defines << FormatStringWithRule(source, rule);
        variable_defines.insert(var_name);
      }
    }
  }
  // Data define
  bool is_above_version_1_4 = Config::IsHigherOrEqual(
      compile_options_.target_sdk_version_, LYNX_VERSION_1_4);
  bool has_get_props_map = false;
  for (auto var_name : component->variables_in_use()) {
    if (variable_defines.find(var_name) == variable_defines.end()) {
      if (is_above_version_1_4 && !has_get_props_map) {
        static std::string props_map_str =
            "let $props_map = Object.keys($props);";
        data_defines << props_map_str;
        has_get_props_map = true;
      }
      static std::string source_with_object_keys =
          "let {varName} = $props_map.includes(\"{varName}\") ? "
          "$props.{varName} : $data.{varName};";
      static std::string source_without_object_keys =
          "let $prop_name_{varName} = $props.{varName};"
          "let {varName} = ($prop_name_{varName} != null && "
          "$prop_name_{varName} "
          "!= undefined )? "
          "$prop_name_{varName} : $data.{varName};";
      // clang-format off
        LepusGenRuleMap rule = {
            {"varName", var_name.str()},
        };
      // clang-format on

      const std::string &source = is_above_version_1_4
                                      ? source_with_object_keys
                                      : source_without_object_keys;
      data_defines << FormatStringWithRule(source, rule);
      variable_defines.insert(var_name);
    }
  }

  LepusGenRuleMap rule = {
      {"ID", id.str()},
      {"createNode", content.str()},
      {"dataDefine", data_defines.str()},
      {"propsDefine", props_defines.str()},
      {"componentInfoMapDefinition",
       GenDependentComponentInfoMapDefinition(current_component_)},
      {"templateFunctions", templateFunctions},
      {"scriptDefine", script_defines}};
  // clang-format off

  std::string source =
      "$renderComponent{ID} = function($component, $data, $props, $recursive) {"
          "{templateFunctions}"
          "{dataDefine}"
          "{propsDefine}"
          "{scriptDefine}"
          "let $parent = $component;"
          "{componentInfoMapDefinition}"
          "let $child = null;"
          "{createNode}"
      "};";
  // clang-format on
  return FormatStringWithRule(source, rule);
}

void TemplateParser::GenComponentMouldForCompilerNG(Component *component) {
  std::set<Component *> necessary_components;
  FindNecessaryComponentInComponent(package_instance_.get(), component,
                                    necessary_components);
  for (auto it = necessary_components.begin(); it != necessary_components.end();
       ++it) {
    if ((*it)->IsPage()) {
      continue;
    }
    ComponentScope component_scope(this, *it);
    auto &ttml = (*it)->ttml();
    for (auto i = 0; i < ttml.Size(); ++i) {
      GenInstruction(ttml[i]);
    }
    GenComponentMould(*it);
  }
}

rapidjson::Value TemplateParser::SegregateAttrsFromPropsForComponent(
    const rapidjson::Value &props, std::stringstream &set_props_content,
    bool component_is, Component *component) {
  auto attrs = rapidjson::Value(rapidjson::kObjectType);
  if (props.IsObject()) {
    for (auto it = props.GetObject().begin(); it != props.GetObject().end();
         ++it) {
      // Judgment method to determine whether prop is attr:
      // -static:
      //   1: in attribute allow list
      //   2: not in component's properties list
      // - not static (component is or dynamic component) :
      //   1: not in component's properties list
      if (IsComponentAttrs(it->name.GetString()) &&
          (component_is || !component || !component->props().IsObject() ||
           component->props()[it->name.GetString()].IsNull())) {
        rapidjson::Value key(it->name, document.GetAllocator());
        rapidjson::Value value(it->value, document.GetAllocator());
        attrs.AddMember(key, value, document.GetAllocator());
      } else {
        // In order to reuse and compatible with orgin process, we check value
        // type and transform boolean value to corresponding string "true" when
        // type is "true type". Then we support "prop" and "prop=true" in
        // ".ttml"
        std::string value = "true";
        auto &value_obj = VALUE(it->value);
        if (value_obj.GetType() != rapidjson::Type::kTrueType ||
            !value_obj.GetBool()) {
          value = value_obj.GetString();
        }
        LepusGenRuleMap rule = {
            {"propName", FormatValue(it->name.GetString())},
            {"propValue", GenStatement(value)},
        };
        static std::string source =
            "_SetProp($child, {propName}, {propValue});";
        set_props_content << FormatStringWithRule(source, rule);
      }
    }
  }
  return attrs;
}

std::string TemplateParser::GetCurrentPath() {
  if (current_page_) {
    return current_page_->path();
  }
  if (current_dynamic_component_) {
    return current_dynamic_component_->path();
  }
  return "";
}

std::string TemplateParser::GetPlugName(const rapidjson::Value &node) {
  if (node.IsObject()) {
    if (need_handle_fallback_ &&
        strcmp(node["tagName"]["value"].GetString(), "fallback") == 0) {
      return FormatValue(kFallbackName);
    }

    const auto &slot_name = VALUE(node["slot"]);
    if (slot_name.IsString()) {
      return GenStatement(slot_name.GetString());
    }
  }

  if (node.IsString()) {
    return FormatValue(defaultSlotName);
  }

  return FormatValue("");
}

std::string TemplateParser::GenPlugNode(const rapidjson::Value &node,
                                        const std::string &content,
                                        const bool is_component_in_template) {
  return GenPlugNode(GetPlugName(node), content, is_component_in_template);
}

std::string TemplateParser::GenPlugNode(const std::string &plug_name,
                                        const std::string &content,
                                        const bool is_component_in_template) {
  // clang-format off
  auto construct_plug_source = [](const std::string &source) {
    return "{"
             "let $parent = _CreateVirtualPlug({plugName});"
             "let $child = null;"
             "{genSlotContent}" 
             + source +
           "}";
  };
  auto construct_source_in_template = [](const std::string &source) {
    return "{"
             "if ($componentInfo) " 
             + source + 
           "}";
  };
  // clang-format on

  static std::string plug_source =
      construct_plug_source("_AddPlugToComponent($childComponent, $parent);");
  static std::string plug_source_in_template =
      construct_source_in_template(plug_source);
  static std::string fallback_source = construct_plug_source(
      "_AddFallbackToDynamicComponent($childComponent, $parent);");
  static std::string fallback_source_in_template =
      construct_source_in_template(fallback_source);

  static std::string fallback_name = FormatValue(kFallbackName);

  bool is_fallback = plug_name == fallback_name;

  LepusGenRuleMap rule = {
      {"plugName", plug_name},
      {"genSlotContent", content},
  };

  return FormatStringWithRule(
      is_fallback
          ? (is_component_in_template ? fallback_source_in_template
                                      : fallback_source)
          : (is_component_in_template ? plug_source_in_template : plug_source),
      rule);
}

}  // namespace tasm
}  // namespace lynx
