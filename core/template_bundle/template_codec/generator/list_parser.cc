// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/generator/list_parser.h"

#include <array>

namespace lynx {

namespace tasm {

namespace {
constexpr const auto kHeader =
    static_cast<size_t>(ListParser::ListComponentType::HEADER);
constexpr const auto kDefault =
    static_cast<size_t>(ListParser::ListComponentType::DEFAULT);
constexpr const auto kFooter =
    static_cast<size_t>(ListParser::ListComponentType::FOOTER);
constexpr const size_t kListParts[] = {kDefault, kHeader, kFooter};

template <typename Lhs, typename Rhs>
auto Append(Lhs &&lhs, Rhs &&rhs) {
  std::get<0>(lhs).append(std::get<0>(rhs));
  std::get<1>(lhs).append(std::get<1>(rhs));
  std::get<2>(lhs).append(std::get<2>(rhs));
}

template <typename Func>
auto ForEachChild(const rapidjson::Value &child, Func func) {
  auto &children_array = child["children"];
  for (auto i = decltype(children_array.Size()){}; i < children_array.Size();
       ++i) {
    func(children_array[i]);
  }
}

auto GetTagName(const rapidjson::Value &child) {
  auto &tag_object = child["tagName"];
  if (tag_object.IsObject()) {
    auto &tag_value = tag_object.GetObject()["value"];
    if (tag_value.IsString()) {
      return std::string{tag_value.GetString()};
    }
  }
  return std::string{};
}

auto WrapWithForLoop(const std::string &statement) {
  if (!statement.empty()) {
    return "for (let {index} = 0; {index} < $count; ++{index}) {" + statement +
           "}";
  }
  return std::string{};
}
}  // namespace

std::string ListParser::GenList(const rapidjson::Value &child) {
  auto res = Generate(child, ListComponentType::DEFAULT, 0);
  return res[kHeader] + res[kDefault] + res[kFooter];
}

ListParser::ListNodeType ListParser::GenerateIf(
    const rapidjson::Value &child, ListComponentType list_component_type,
    size_t depth) {
  auto &content = child;
  DCHECK(content.IsObject());
  auto &condition = content["condition"];
  auto &consequent = content["consequent"];
  auto &alternate = content["alternate"];
  DCHECK(consequent.IsArray() && consequent.Size() == 1);
  DCHECK(alternate.IsNull() || (alternate.IsArray() && alternate.Size() == 1) ||
         alternate.IsObject());
  const auto condition_statement = parser_.GenStatement(VALUE(condition));
  const auto consequent_statements =
      Generate(consequent[0], list_component_type, depth + 1);
  const auto alternate_statements = [&alternate, this, list_component_type,
                                     depth]() {
    if (alternate.IsArray()) {
      return Generate(alternate[0], list_component_type, depth + 1);
    } else {
      return Generate(alternate, list_component_type, depth + 1);
    }
  }();

  constexpr const char full_source[] =
      "if ({condition}) {"
      "{consequent}"
      "} else {"
      "{alternate}"
      "}";
  constexpr const char if_source[] =
      "if ({condition}) {"
      "{consequent}"
      "}";
  constexpr const char else_source[] =
      "if (!{condition}) {"
      "{alternate}"
      "}";

  auto rule = std::unordered_map<std::string, std::string>{
      {"condition", condition_statement}};

  auto gen_branch = [this, &full_source, &if_source, &else_source, &rule](
                        const std::string &consequent_statement,
                        const std::string &alternate_statement) {
    auto source = std::string{};
    if (consequent_statement.empty() && alternate_statement.empty()) {
      return std::string{};
    } else if (consequent_statement.empty()) {
      source = else_source;
    } else if (alternate_statement.empty()) {
      source = if_source;
    } else {
      source = full_source;
    }
    rule["consequent"] = consequent_statement;
    rule["alternate"] = alternate_statement;
    return parser_.FormatStringWithRule(source, rule);
  };

  ListNodeType node{};

  for (auto part : kListParts) {
    node[part] =
        gen_branch(consequent_statements[part], alternate_statements[part]);
  }

  return node;
}

ListParser::ListNodeType ListParser::GenerateList(
    const rapidjson::Value &child, ListComponentType list_component_type,
    size_t depth) {
  const auto type_name = std::string{child["type"].GetString()};
  const auto tag_name = GetTagName(child);

  auto &element = child;
  auto &classes = element["className"];
  auto &styles = element["style"];
  auto &attrs = element["attrs"];
  auto &dataset = element["dataset"];
  auto &events = element["events"];
  auto &id = element["elemId"];
  auto &count = element["count"];
  auto &gesture = element["gesture"];
  const auto index_statement = [&element] {
    if (element["attrs"]["index"].IsObject()) {
      return VALUE(element["attrs"]["index"]).GetString();
    } else {
      return "index";
    }
  }();

  // clang-format off
      auto list_statement = std::string{};

      if (depth == 0) {
        list_statement =
            "{"
              "let $count = {count};"
              "$child = _CreateVirtualListNode($kTemplateAssembler, {eid}, {count});"
              "{setId}"
              "{setAttributes}"
              "{setClasses}"
              "{setStyles}"
              "{setDataSet}"
              "{setEvents}"
              "{setGesture}"
              "{setChildrenHeader}"
              "{setChildren}"
              "{setChildrenFooter}"
              "_AppendChild($parent, $child);"
            "}";
      } else {
        list_statement =
            "{"
              "let $count = {count};"
              "{setChildrenHeader}"
              "{setChildren}"
              "{setChildrenFooter}"
            "}";
      };

      auto list_gen_rule = std::unordered_map<std::string, std::string>{};
      list_gen_rule["index"] = index_statement;
      list_gen_rule["setId"] = parser_.GenId(id);
      list_gen_rule["setAttributes"] = parser_.GenAttributes(attrs);
      list_gen_rule["setClasses"] = parser_.GenClasses(classes);
      list_gen_rule["setStyles"] = parser_.GenStyles(styles);
      list_gen_rule["setDataSet"] = parser_.GenDataSet(dataset);
      list_gen_rule["setEvents"] = parser_.GenEvents(events);
      list_gen_rule["setGesture"] = parser_.GenGestures(gesture);
      list_gen_rule["eid"] = std::to_string(eid_);

      auto hasCount = count.IsObject() && count.GetObject().MemberCount() > 0;
      if (hasCount) {
        list_gen_rule["count"] = parser_.GenStatement(VALUE(count));
      } else {
        list_gen_rule["count"] = std::to_string(1);
      }

      list_gen_rule["setChildren"] = "";
      list_gen_rule["setChildrenHeader"] = "";
      list_gen_rule["setChildrenFooter"] = "";

  // clang-format on
  ForEachChild(child, [&](const rapidjson::Value &child) {
    Append(std::tie(list_gen_rule["setChildren"],
                    list_gen_rule["setChildrenHeader"],
                    list_gen_rule["setChildrenFooter"]),
           Generate(child, list_component_type, depth + 1));
  });

  if (hasCount) {
    list_gen_rule["setChildren"] =
        WrapWithForLoop(list_gen_rule["setChildren"]);
    list_gen_rule["setChildren"] = parser_.FormatStringWithRule(
        list_gen_rule["setChildren"], list_gen_rule);
  }

  ListNodeType node{};
  node[kDefault] = parser_.FormatStringWithRule(list_statement, list_gen_rule);
  return node;
}

ListParser::ListNodeType ListParser::GenerateRepeat(
    const rapidjson::Value &child, ListComponentType list_component_type,
    size_t depth) {
  auto &element = child;
  DCHECK(element.IsObject());
  auto &object = VALUE(element["for"]);
  auto &variable = VALUE(element["item"]);
  auto &index = VALUE(element["index"]);
  auto &block = element["body"];

  auto object_statement = parser_.GenStatement(object);
  auto index_statement = index.GetString();
  auto item_statement = variable.GetString();

  auto res = Generate(block, list_component_type, depth + 1);

  auto gen_branch = [&obj = object_statement, &item = item_statement,
                     &index = index_statement,
                     this](const std::string &statement) {
    auto source =
        "{"
        "let $object = {obj};"
        "for (let {index} = 0; {index} < _GetLength($object); ++{index}) {"
        "let {item} = _IndexOf($object, {index});"
        "{statements}"  // statements
        "}"
        "};";
    if (statement.empty()) {
      source = "";
    }
    auto rule =
        std::unordered_map<std::string, std::string>{{"obj", obj},
                                                     {"index", index},
                                                     {"item", item},
                                                     {"statements", statement}};
    return parser_.FormatStringWithRule(source, rule);
  };

  ListNodeType node{};

  for (auto part : kListParts) {
    node[part] = gen_branch(res[part]);
  }

  return node;
}

ListParser::ListNodeType ListParser::GenerateNode(
    const rapidjson::Value &child, ListComponentType list_component_type,
    size_t depth) {
  const auto tag_name = GetTagName(child);
  if (tag_name == "block") {
    return GenerateNodeBlock(child, list_component_type, depth);
  } else if (tag_name == "header") {
    return GenerateNodeHeader(child, list_component_type, depth);
  } else if (tag_name == "footer") {
    return GenerateNodeFooter(child, list_component_type, depth);
  } else if (tag_name == "list-row") {
    return GenerateNodeListRow(child, list_component_type, depth);
  } else if (!tag_name.empty()) {
    return GenerateComponent(child, list_component_type, depth);
  }
  return {};
}

ListParser::ListNodeType ListParser::GenerateNodeBlock(
    const rapidjson::Value &child, ListComponentType list_component_type,
    size_t depth) {
  ListNodeType node{};
  ForEachChild(child, [this, list_component_type, depth,
                       &node](const rapidjson::Value &child) {
    auto res = Generate(child, list_component_type, depth + 1);
    Append(node, res);
  });
  return node;
}

ListParser::ListNodeType ListParser::GenerateNodeHeader(
    const rapidjson::Value &child, ListComponentType list_component_type,
    size_t depth) {
  if (list_component_type == ListComponentType::DEFAULT) {
    list_component_type = ListComponentType::HEADER;
  }
  auto node = GenerateNodeBlock(child, list_component_type, depth);
  node[kHeader].append(node[kDefault]).append(node[kFooter]);
  node[kDefault].clear();
  node[kFooter].clear();
  return node;
}

ListParser::ListNodeType ListParser::GenerateNodeFooter(
    const rapidjson::Value &child, ListComponentType list_component_type,
    size_t depth) {
  if (list_component_type == ListComponentType::DEFAULT) {
    list_component_type = ListComponentType::FOOTER;
  }
  auto node = GenerateNodeBlock(child, list_component_type, depth);
  node[kFooter] = node[kHeader].append(node[kDefault]).append(node[kFooter]);
  node[kHeader].clear();
  node[kDefault].clear();
  return node;
}

ListParser::ListNodeType ListParser::GenerateNodeListRow(
    const rapidjson::Value &child, ListComponentType list_component_type,
    size_t depth) {
  if (list_component_type == ListComponentType::DEFAULT) {
    list_component_type = ListComponentType::LIST_ROW;
  }
  auto node = GenerateNodeBlock(child, list_component_type, depth);
  return node;
}

ListParser::ListNodeType ListParser::GenerateComponent(
    const rapidjson::Value &child, ListComponentType list_component_type,
    size_t depth) {
  std::string component_source =
      // clang-format off
        "{"
        "let $name = {name_statement};"
        "let $props = {props_statement};"
        "let $ids = {ids_statement};"
        "let $style = {style_statement};"
        "let $class = {class_statement};"
        "let $event = {event_statement};"
        "let $dataset = {dataset_statement};"
        "let $comptype = {component_type};"
        "if ($name) {"
        "{func_name}($child, $name, null, $props, $ids, $style, "
        "$class, $event, $dataset, $comptype);"
        "}"
        "}";
  // clang-format on
  auto &tagName = child["tagName"];
  auto &is = child["is"];
  auto &props = child["attrs"];
  auto &ids = child["elemId"];
  auto &style = child["style"];
  auto &class_st = child["className"];
  std::unordered_map<std::string, std::string> template_rule = {
      {"name_statement", ""}, {"func_name", ""}};
  static const std::string comp_types[] = {"default", "header", "footer",
                                           "list-row"};
  // template_rule["comptype_statement"] = GenStatement(comp_types[compType]);
  if (is.IsObject() && VALUE(is).IsString() &&
      strlen(VALUE(is).GetString()) != 0) {
    template_rule["name_statement"] = parser_.GenStatement(VALUE(is));
  } else {
    template_rule["name_statement"] = parser_.GenStatement(VALUE(tagName));
  }

  auto update_list_statement = [&template_rule, this](const auto &state,
                                                      const auto &prop) {
    if (prop.IsString()) {
      template_rule[state] = parser_.GenStatement(prop);
    } else {
      template_rule[state] = parser_.GenStatement(VALUE(prop));
    }
  };

  update_list_statement("ids_statement", ids);
  update_list_statement("style_statement", style);
  update_list_statement("class_statement", class_st);

  // issue: #1601
  // generate nothing if 'template_rule["name_statement"]' is null.
  ListNodeType node{};
  if (template_rule["name_statement"] != "null") {
    template_rule["func_name"] = "_AppendListComponentInfo";
    template_rule["props_statement"] = parser_.GenComponentProps(props);
    template_rule["dataset_statement"] =
        parser_.GenComponentProps(child["dataset"]);
    template_rule["event_statement"] =
        parser_.GenComponentEvent(child["events"]);
    template_rule["component_type"] = parser_.GenStatement(
        comp_types[static_cast<size_t>(list_component_type)]);
    node[kDefault] =
        parser_.FormatStringWithRule(component_source, template_rule);
  }

  return node;
}

ListParser::ListNodeType ListParser::Generate(
    const rapidjson::Value &child, ListComponentType list_component_type,
    size_t depth) {
  if (!child.IsObject()) {
    return {};
  }

  const auto type_name = std::string{child["type"].GetString()};

  if (type_name == "list") {
    return GenerateList(child, list_component_type, depth);
  } else if (type_name == "if") {
    return GenerateIf(child, list_component_type, depth);
  } else if (type_name == "repeat") {
    return GenerateRepeat(child, list_component_type, depth);
  } else if (type_name == "node") {
    return GenerateNode(child, list_component_type, depth);
  } else if (type_name == "component") {
    return GenerateComponent(child, list_component_type, depth);
  }
  return {};
}

}  // namespace tasm
}  // namespace lynx
