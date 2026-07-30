// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"

#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "core/public/pipeline_option.h"
#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/css_fragment.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/dynamic_css_styles_manager.h"
#include "core/renderer/css/ng/parser/media_query_parser.h"
#include "core/renderer/css/ng/parser/supports_condition_parser.h"
#include "core/renderer/css/ng/style/condition_rule.h"
#include "core/renderer/css/ng/style/rule_set.h"
#include "core/renderer/css/ng/supports/supports_evaluator.h"
#include "core/renderer/dom/element_manager.h"
#include "core/services/replay/replay_controller.h"
#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/element/element_helper.h"
#include "devtool/lynx_devtool/element/helper_util.h"
#include "third_party/modp_b64/modp_b64.h"
#include "third_party/zlib/zlib.h"

namespace lynx {
namespace devtool {

#define HANDLE_WHITE_BOARD_METHOD(method, cur_func_name)                     \
  do {                                                                       \
    CHECK_NULL_AND_LOG_RETURN(white_board_inspector_delegate_,               \
                              "InspectorTasmExecutor::" #cur_func_name       \
                              ", white_board_inspector_delegate_ is null");  \
    std::string response = white_board_inspector_delegate_->method(message); \
    if (!response.empty()) {                                                 \
      sender->SendMessage("CDP", response);                                  \
    }                                                                        \
  } while (0)

namespace {

std::string TrimConditionText(const std::string& text) {
  const size_t start = text.find_first_not_of(" \t\n\r\f\v");
  if (start == std::string::npos) {
    return "";
  }
  const size_t end = text.find_last_not_of(" \t\n\r\f\v");
  return text.substr(start, end - start + 1);
}

std::string NormalizeConditionTextForParser(const std::string& text,
                                            const std::string& at_rule) {
  std::string normalized = TrimConditionText(text);
  if (normalized.rfind(at_rule, 0) == 0) {
    normalized = TrimConditionText(normalized.substr(at_rule.length()));
  }
  return normalized;
}

struct ConditionTextEditRequest {
  std::string style_sheet_id;
  int condition_index = -1;
  std::string text;
  bool has_valid_range = false;
};

ConditionTextEditRequest ParseConditionTextEditRequest(
    const Json::Value& message, const std::string& at_rule) {
  const Json::Value& params = message["params"];
  const Json::Value range =
      params.get("range", Json::Value(Json::ValueType::objectValue));
  return {params["styleSheetId"].asString(), range["startLine"].asInt(),
          NormalizeConditionTextForParser(params["text"].asString(), at_rule),
          range.isObject() && range.isMember("startLine")};
}

void SendConditionTextEditError(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message, int code, const char* error_message) {
  Json::Value response(Json::ValueType::objectValue);
  response["error"]["code"] = code;
  response["error"]["message"] = error_message;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void SendConditionTextEditResult(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message, const char* result_key,
    const Json::Value& condition) {
  Json::Value response(Json::ValueType::objectValue);
  response["result"][result_key] = condition;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// TTSS source ranges are not available after binary decoding. DevTool uses this
// synthetic range as a stable key: startLine/endLine store the condition-rule
// index within the stylesheet, and endColumn stores the editable text length.
Json::Value BuildSyntheticConditionRange(int condition_index,
                                         const std::string& condition_text) {
  Json::Value range(Json::ValueType::objectValue);
  range["startLine"] = condition_index;
  range["startColumn"] = 0;
  range["endLine"] = condition_index;
  range["endColumn"] = static_cast<int>(condition_text.length());
  return range;
}

Json::Value BuildMediaObject(const std::string& media_text,
                             const std::string& style_sheet_id,
                             int media_index) {
  Json::Value media(Json::ValueType::objectValue);
  media["text"] = media_text;
  media["source"] = "mediaRule";
  media["range"] = BuildSyntheticConditionRange(media_index, media_text);
  if (!style_sheet_id.empty()) {
    media["styleSheetId"] = style_sheet_id;
  }
  return media;
}

Json::Value BuildSupportsObject(const std::string& supports_text,
                                const std::string& style_sheet_id,
                                int supports_index, bool active) {
  Json::Value supports(Json::ValueType::objectValue);
  supports["text"] = supports_text;
  supports["active"] = active;
  supports["range"] =
      BuildSyntheticConditionRange(supports_index, supports_text);
  if (!style_sheet_id.empty()) {
    supports["styleSheetId"] = style_sheet_id;
  }
  return supports;
}

void SyncCachedMediaInfo(lynx::tasm::Element* style_root,
                         const std::string& style_sheet_id, int media_index,
                         const std::string& media_text) {
  if (!style_root || style_sheet_id.empty() || media_index < 0) {
    return;
  }
  Range media_range{media_index, media_index, 0,
                    static_cast<int>(media_text.length())};
  auto& style_sheet_map = ElementInspector::GetStyleSheetMap(style_root);
  for (auto& [_, style_sheet] : style_sheet_map) {
    if (style_sheet.style_sheet_id_ == style_sheet_id &&
        !style_sheet.media_text_.empty() &&
        style_sheet.media_range_.start_line_ == media_index) {
      style_sheet.media_text_ = media_text;
      style_sheet.media_range_ = media_range;
    }
  }
}

void SyncCachedSupportsInfo(lynx::tasm::Element* style_root,
                            const std::string& style_sheet_id,
                            int supports_index,
                            const std::string& supports_text) {
  if (!style_root || style_sheet_id.empty() || supports_index < 0) {
    return;
  }
  Range supports_range{supports_index, supports_index, 0,
                       static_cast<int>(supports_text.length())};
  auto& style_sheet_map = ElementInspector::GetStyleSheetMap(style_root);
  for (auto& [_, style_sheet] : style_sheet_map) {
    if (style_sheet.style_sheet_id_ == style_sheet_id &&
        !style_sheet.supports_text_.empty() &&
        style_sheet.supports_range_.start_line_ == supports_index) {
      style_sheet.supports_text_ = supports_text;
      style_sheet.supports_range_ = supports_range;
    }
  }
}

constexpr size_t kBoxModelSize = 34;
constexpr size_t kInvalidBoxModelIndex = static_cast<size_t>(-1);

using ComputedStyleMap = std::unordered_map<std::string, std::string>;
using BoxModelNodePath = std::vector<Json::ArrayIndex>;

struct LayerBoxModelInfo {
  Json::ArrayIndex layer_index = 0;
  size_t box_model_index = kInvalidBoxModelIndex;
  size_t parent_box_model_index = kInvalidBoxModelIndex;
};

InspectorLayoutObjectInfo BuildLayoutObjectInfo(tasm::Element* element) {
  if (element == nullptr) {
    return {};
  }
  return lynx::devtool::BuildLayoutObjectInfo(element->impl_id(),
                                              element->GetLayoutObject());
}

bool IsVirtualOrWrapper(tasm::Element* element) {
  return element != nullptr && (element->is_virtual() || element->is_wrapper());
}

Json::Value ConvertBoxModelToJson(const std::vector<double>& box_model,
                                  double screen_scale_factor,
                                  float layouts_unit_per_px) {
  if (box_model.size() != kBoxModelSize) {
    return Json::Value();
  }
  Json::Value model(Json::ValueType::objectValue);
  model["width"] = box_model[0] / layouts_unit_per_px * screen_scale_factor;
  model["height"] = box_model[1] / layouts_unit_per_px * screen_scale_factor;
  model["content"] = Json::Value(Json::ValueType::arrayValue);
  for (int i = 2; i <= 9; ++i) {
    model["content"].append(box_model[i] / layouts_unit_per_px *
                            screen_scale_factor);
  }
  model["padding"] = Json::Value(Json::ValueType::arrayValue);
  for (int i = 10; i <= 17; ++i) {
    model["padding"].append(box_model[i] / layouts_unit_per_px *
                            screen_scale_factor);
  }
  model["border"] = Json::Value(Json::ValueType::arrayValue);
  for (int i = 18; i <= 25; ++i) {
    model["border"].append(box_model[i] / layouts_unit_per_px *
                           screen_scale_factor);
  }
  model["margin"] = Json::Value(Json::ValueType::arrayValue);
  for (int i = 26; i <= 33; ++i) {
    model["margin"].append(box_model[i] / layouts_unit_per_px *
                           screen_scale_factor);
  }
  return model;
}

Json::Value BuildBoxModelError() {
  Json::Value res(Json::ValueType::objectValue);
  Json::Value error = Json::Value(Json::ValueType::objectValue);
  error["code"] = Json::Value(-32000);
  error["message"] = Json::Value("Could not compute box model.");
  res["error"] = error;
  return res;
}

void ApplyRootBoxModelOffset(std::vector<double>& box_model,
                             const std::vector<double>& root_box_model) {
  if (box_model.size() != kBoxModelSize ||
      root_box_model.size() != kBoxModelSize) {
    return;
  }
  double origin_x = root_box_model[18];
  double origin_y = root_box_model[19];
  for (int i = 1; i <= 16; i++) {
    box_model[2 * i] -= origin_x;
    box_model[2 * i + 1] -= origin_y;
  }
}

Json::Value BuildBoxModelResult(std::vector<double> box_model,
                                double screen_scale_factor,
                                float layouts_unit_per_px,
                                const std::vector<double>& root_box_model) {
  if (box_model.size() != kBoxModelSize) {
    return BuildBoxModelError();
  }
  ApplyRootBoxModelOffset(box_model, root_box_model);

  Json::Value content(Json::ValueType::objectValue);
  content["model"] = ConvertBoxModelToJson(box_model, screen_scale_factor,
                                           layouts_unit_per_px);
  return content;
}

// When an already-built subtree is wrapped by an extra document/component node,
// its recorded box model paths need to descend through the wrapper's child
// slot.
void InsertChildIndexInPaths(std::vector<BoxModelNodePath>& paths, size_t begin,
                             size_t end, size_t parent_path_size,
                             Json::ArrayIndex child_index) {
  end = std::min(end, paths.size());
  for (size_t i = begin; i < end; ++i) {
    if (paths[i].size() >= parent_path_size) {
      paths[i].insert(paths[i].begin() + parent_path_size, child_index);
    }
  }
}

Json::Value* GetNodeByPath(Json::Value& root, const BoxModelNodePath& path) {
  Json::Value* node = &root;
  for (Json::ArrayIndex child_index : path) {
    if (!node->isObject() || !node->isMember("children") ||
        !(*node)["children"].isArray() ||
        child_index >= (*node)["children"].size()) {
      return nullptr;
    }
    node = &(*node)["children"][child_index];
  }
  return node;
}

void ApplyBoxModelsToDocument(
    Json::Value& root, const std::vector<BoxModelNodePath>& box_model_paths,
    const std::vector<std::vector<double>>& box_models,
    double screen_scale_factor, float layouts_unit_per_px,
    const std::vector<double>& root_box_model) {
  for (size_t i = 0; i < box_model_paths.size(); ++i) {
    Json::Value* node = GetNodeByPath(root, box_model_paths[i]);
    if (node == nullptr) {
      continue;
    }
    if (i < box_models.size() && box_models[i].size() == kBoxModelSize) {
      std::vector<double> box_model = box_models[i];
      ApplyRootBoxModelOffset(box_model, root_box_model);
      (*node)["box_model"] = ConvertBoxModelToJson(
          box_model, screen_scale_factor, layouts_unit_per_px);
    } else {
      (*node)["box_model"] = Json::Value();
    }
  }
}

bool IsValidBoxModelIndex(size_t index,
                          const std::vector<std::vector<double>>& box_models) {
  return index < box_models.size() && box_models[index].size() == kBoxModelSize;
}

void ApplyBoxModelsToLayers(
    Json::Value& layers, const std::vector<LayerBoxModelInfo>& box_model_infos,
    const std::vector<std::vector<double>>& box_models) {
  if (!layers.isArray()) {
    return;
  }
  for (const auto& info : box_model_infos) {
    if (info.layer_index >= layers.size() ||
        !IsValidBoxModelIndex(info.box_model_index, box_models)) {
      continue;
    }
    const std::vector<double>& box_model = box_models[info.box_model_index];
    Json::Value& layer = layers[info.layer_index];
    layer["width"] = box_model[28] - box_model[26];
    layer["height"] = box_model[31] - box_model[29];
    if (IsValidBoxModelIndex(info.parent_box_model_index, box_models)) {
      const std::vector<double>& parent_box_model =
          box_models[info.parent_box_model_index];
      layer["offsetX"] = box_model[26] - parent_box_model[26];
      layer["offsetY"] = box_model[27] - parent_box_model[27];
    } else {
      layer["offsetX"] = box_model[26];
      layer["offsetY"] = box_model[27];
    }
  }
}

void ApplyBoxModelToComputedStyle(ComputedStyleMap& dict,
                                  const std::vector<double>& box_info,
                                  float layouts_unit_per_px) {
  if (box_info.size() != kBoxModelSize) {
    return;
  }
  dict["width"] =
      lynx::tasm::CSSDecoder::ToPxValue(box_info[0] / layouts_unit_per_px);
  dict["height"] =
      lynx::tasm::CSSDecoder::ToPxValue(box_info[1] / layouts_unit_per_px);

  // margin 26-33 border 18-25 padding 10-17 content 2-9
  dict["margin-left"] = lynx::tasm::CSSDecoder::ToPxValue(
      (box_info[18] - box_info[26]) / layouts_unit_per_px);
  dict["margin-top"] = lynx::tasm::CSSDecoder::ToPxValue(
      (box_info[19] - box_info[27]) / layouts_unit_per_px);
  dict["margin-right"] = lynx::tasm::CSSDecoder::ToPxValue(
      (box_info[28] - box_info[20]) / layouts_unit_per_px);
  dict["margin-bottom"] = lynx::tasm::CSSDecoder::ToPxValue(
      (box_info[33] - box_info[25]) / layouts_unit_per_px);
  if (dict["margin-left"] == dict["margin-right"] &&
      dict["margin-left"] == dict["margin-top"] &&
      dict["margin-left"] == dict["margin-bottom"]) {
    dict["margin"] = dict["margin-left"];
  } else {
    std::ostringstream margin_str;
    margin_str << dict["margin-top"] << " " << dict["margin-right"] << " "
               << dict["margin-bottom"] << " " << dict["margin-left"];
    dict["margin"] = margin_str.str();
  }

  dict["border-left-width"] = lynx::tasm::CSSDecoder::ToPxValue(
      (box_info[10] - box_info[18]) / layouts_unit_per_px);
  dict["border-right-width"] = lynx::tasm::CSSDecoder::ToPxValue(
      (box_info[20] - box_info[12]) / layouts_unit_per_px);
  dict["border-top-width"] = lynx::tasm::CSSDecoder::ToPxValue(
      (box_info[11] - box_info[19]) / layouts_unit_per_px);
  dict["border-bottom-width"] = lynx::tasm::CSSDecoder::ToPxValue(
      (box_info[25] - box_info[17]) / layouts_unit_per_px);

  if (dict["border-left-width"] == dict["border-right-width"] &&
      dict["border-left-width"] == dict["border-top-width"] &&
      dict["border-left-width"] == dict["border-bottom-width"]) {
    dict["border"] = dict["border-left-width"];
  } else {
    std::ostringstream border_str;
    border_str << dict["border-top-width"] << " " << dict["border-right-width"]
               << " " << dict["border-bottom-width"] << " "
               << dict["border-left-width"];
    dict["border"] = border_str.str();
  }

  dict["padding-left"] = lynx::tasm::CSSDecoder::ToPxValue(
      (box_info[2] - box_info[10]) / layouts_unit_per_px);
  dict["padding-top"] = lynx::tasm::CSSDecoder::ToPxValue(
      (box_info[3] - box_info[11]) / layouts_unit_per_px);
  dict["padding-right"] = lynx::tasm::CSSDecoder::ToPxValue(
      (box_info[12] - box_info[4]) / layouts_unit_per_px);
  dict["padding-bottom"] = lynx::tasm::CSSDecoder::ToPxValue(
      (box_info[17] - box_info[9]) / layouts_unit_per_px);

  if (dict["padding-left"] == dict["padding-right"] &&
      dict["padding-left"] == dict["padding-top"] &&
      dict["padding-left"] == dict["padding-bottom"]) {
    dict["padding"] = dict["padding-left"];
  } else {
    std::ostringstream padding_str;
    padding_str << dict["padding-top"] << " " << dict["padding-right"] << " "
                << dict["padding-bottom"] << " " << dict["padding-left"];
    dict["padding"] = padding_str.str();
  }
}

Json::Value ConvertComputedStyleToJson(const ComputedStyleMap& dict) {
  Json::Value res = Json::Value(Json::ValueType::arrayValue);
  ComputedStyleMap remaining = dict;
  auto append_style = [&res](const std::string& name,
                             const std::string& value) {
    if (name.empty()) {
      return;
    }
    Json::Value temp = Json::Value(Json::ValueType::objectValue);
    temp["name"] = name;
    if (name.find("color") != std::string::npos &&
        name != "-x-animation-color-interpolation" && name != "border-color") {
      temp["value"] = lynx::tasm::CSSDecoder::ToRgbaFromColorValue(value);
    } else {
      temp["value"] = value;
    }
    res.append(temp);
  };

  for (int id = lynx::tasm::CSSPropertyID::kPropertyStart + 1;
       id < lynx::tasm::CSSPropertyID::kPropertyEnd; ++id) {
    const char* property_name = lynx::tasm::CSSProperty::GetPropertyNameCStr(
        static_cast<lynx::tasm::CSSPropertyID>(id));
    auto it = remaining.find(property_name);
    if (it != remaining.end()) {
      append_style(it->first, it->second);
      remaining.erase(it);
    }
  }

  std::vector<std::string> remaining_names;
  remaining_names.reserve(remaining.size());
  for (const auto& pair : remaining) {
    if (!pair.first.empty()) {
      remaining_names.push_back(pair.first);
    }
  }
  std::sort(remaining_names.begin(), remaining_names.end());
  for (const auto& name : remaining_names) {
    append_style(name, remaining.at(name));
  }
  return res;
}

ComputedStyleMap BuildComputedStyleMap(tasm::Element* ptr) {
  ComputedStyleMap dict;
  if (ptr == nullptr || !ElementInspector::HasDataModel(ptr)) {
    return dict;
  }
  dict = ElementInspector::GetDefaultCss();
  if (ElementInspector::IsEnableCSSSelector(ptr)) {
    const std::vector<InspectorStyleSheet>& match_rules =
        ElementInspector::GetMatchedStyleSheet(ptr);
    for (const InspectorStyleSheet& match : match_rules) {
      ReplaceDefaultComputedStyle(
          dict, ElementInspector::ResolveStyleSheetForComputedStyle(ptr, match)
                    .css_properties_);
    }
  } else {
    auto replace_rule = [ptr, &dict](const InspectorStyleSheet& style_sheet) {
      ReplaceDefaultComputedStyle(
          dict,
          ElementInspector::ResolveStyleSheetForComputedStyle(ptr, style_sheet)
              .css_properties_);
    };
    replace_rule(ElementInspector::GetStyleSheetByName(ptr, "*"));
    replace_rule(ElementInspector::GetStyleSheetByName(ptr, "body *"));
    for (size_t i = 0; i < ElementInspector::ClassOrder(ptr).size(); ++i) {
      replace_rule(ElementInspector::GetStyleSheetByName(
          ptr, ElementInspector::ClassOrder(ptr)[i]));
    }
    replace_rule(ElementInspector::GetStyleSheetByName(
        ptr, ElementInspector::SelectorId(ptr)));
    replace_rule(ElementInspector::GetStyleSheetByName(
        ptr, ElementInspector::SelectorTag(ptr)));
  }
  ReplaceDefaultComputedStyle(
      dict, ElementInspector::GetInlineStyleSheet(ptr).css_properties_);
  return dict;
}

Json::Value BuildLayerContentJson(lynx::tasm::Element* element) {
  Json::Value layer(Json::ValueType::objectValue);
  if (element == nullptr) {
    return layer;
  }
  layer["layerId"] = std::to_string(ElementInspector::NodeId(element));
  layer["backendNodeId"] = ElementInspector::NodeId(element);
  if (element->parent() != nullptr) {
    layer["parentLayerId"] =
        std::to_string(ElementInspector::NodeId(element->parent()));
  }
  layer["paintCount"] = 1;
  layer["drawsContent"] = true;
  layer["invisible"] = true;
  layer["name"] = ElementInspector::LocalName(element);
  layer["offsetX"] = Json::Value();
  layer["offsetY"] = Json::Value();
  layer["width"] = Json::Value();
  layer["height"] = Json::Value();
  return layer;
}

bool GetOrAppendBoxModelQuery(
    InspectorTasmExecutor* executor, lynx::tasm::Element* element,
    std::vector<InspectorBoxModelQuery>& queries,
    std::unordered_map<int32_t, size_t>& box_model_query_indexes,
    size_t& index) {
  if (executor == nullptr || element == nullptr) {
    return false;
  }
  InspectorBoxModelQuery query;
  if (!executor->BuildBoxModelQuery(element, query)) {
    return false;
  }
  auto iter = box_model_query_indexes.find(query.layout_object.id);
  if (iter != box_model_query_indexes.end()) {
    index = iter->second;
    return true;
  }
  index = queries.size();
  box_model_query_indexes[query.layout_object.id] = index;
  queries.push_back(std::move(query));
  return true;
}

void AppendLayerBoxModelInfo(
    InspectorTasmExecutor* executor, lynx::tasm::Element* element,
    Json::ArrayIndex layer_index, std::vector<InspectorBoxModelQuery>& queries,
    std::unordered_map<int32_t, size_t>& box_model_query_indexes,
    std::vector<LayerBoxModelInfo>& box_model_infos) {
  LayerBoxModelInfo info;
  info.layer_index = layer_index;
  if (!GetOrAppendBoxModelQuery(executor, element, queries,
                                box_model_query_indexes,
                                info.box_model_index)) {
    return;
  }
  // Parent box model is used to convert the layer's absolute position into a
  // parent-relative offset.
  GetOrAppendBoxModelQuery(executor, element->parent(), queries,
                           box_model_query_indexes,
                           info.parent_box_model_index);
  box_model_infos.push_back(info);
}

Json::Value BuildLayerTreeJson(
    InspectorTasmExecutor* executor, lynx::tasm::Element* root_element,
    std::vector<InspectorBoxModelQuery>& queries,
    std::vector<LayerBoxModelInfo>& box_model_infos) {
  Json::Value layers(Json::ValueType::arrayValue);
  CHECK_NULL_AND_LOG_RETURN_VALUE(root_element, "root_element is null", layers);
  std::unordered_map<int32_t, size_t> box_model_query_indexes;
  std::queue<lynx::tasm::Element*> element_queue;
  element_queue.push(root_element);
  while (!element_queue.empty()) {
    lynx::tasm::Element* element = element_queue.front();
    element_queue.pop();
    Json::ArrayIndex layer_index = layers.size();
    layers.append(BuildLayerContentJson(element));
    AppendLayerBoxModelInfo(executor, element, layer_index, queries,
                            box_model_query_indexes, box_model_infos);
    for (auto& child : element->GetChildren()) {
      element_queue.push(child);
    }
  }
  return layers;
}

using ConditionRuleMatcher = bool (css::ConditionRule::*)() const;

struct ConditionRuleTarget {
  css::ConditionRule* rule = nullptr;
  Element* style_root = nullptr;

  explicit operator bool() const { return rule != nullptr; }
};

ConditionRuleTarget FindConditionRuleByIndex(
    Element* root, const std::string& target_style_sheet_id,
    int target_condition_index, ConditionRuleMatcher matcher) {
  ConditionRuleTarget target;
  if (!root || target_style_sheet_id.empty() || target_condition_index < 0 ||
      !matcher) {
    return target;
  }

  int condition_index = 0;
  std::unordered_set<const css::RuleSet*> visited_rule_sets;
  std::unordered_set<const css::ConditionRule*> visited_condition_rules;
  std::queue<Element*> inspect_node_queue;
  inspect_node_queue.push(root);
  while (!inspect_node_queue.empty() && !target) {
    Element* element = inspect_node_queue.front();
    inspect_node_queue.pop();
    if (!element) continue;
    for (Element* child : element->GetChildren()) {
      inspect_node_queue.push(child);
    }

    tasm::CSSFragment* fragment = element->GetRelatedCSSFragment();
    if (!fragment) continue;
    Element* style_value = ElementInspector::StyleValueElement(element);
    std::string style_sheet_id;
    if (style_value) {
      style_sheet_id = std::to_string(ElementInspector::NodeId(style_value));
    }
    if (style_sheet_id != target_style_sheet_id) {
      continue;
    }

    struct Ctx {
      int target_condition_index;
      int* condition_index;
      std::unordered_set<const css::RuleSet*>* visited_rule_sets;
      std::unordered_set<const css::ConditionRule*>* visited_condition_rules;
      ConditionRuleMatcher matcher;
      css::ConditionRule** target_rule;
    };
    Ctx ctx{target_condition_index,   &condition_index, &visited_rule_sets,
            &visited_condition_rules, matcher,          &target.rule};
    fragment->ForEachRuleSet(
        [](css::RuleSet* rs, void* cb_data) {
          auto* c = static_cast<Ctx*>(cb_data);
          if (*c->target_rule || !rs) return;
          if (!c->visited_rule_sets->insert(rs).second) return;
          auto condition_rule_visitor = [c](const auto& condition_rule) {
            if (*c->target_rule || !(condition_rule.*(c->matcher))()) {
              return;
            }
            if (!c->visited_condition_rules->insert(&condition_rule).second) {
              return;
            }
            if ((*c->condition_index)++ != c->target_condition_index) {
              return;
            }
            *c->target_rule = const_cast<css::ConditionRule*>(&condition_rule);
          };
          rs->ForEachConditionRule(condition_rule_visitor);
        },
        &ctx);
    if (target.rule) {
      target.style_root = style_value;
    }
  }
  return target;
}

void RequestStyleResolveAfterConditionEdit(Element* root) {
  if (auto* element_manager = root->element_manager()) {
    auto options = std::make_shared<lynx::tasm::PipelineOptions>();
    element_manager->RequestResolve(options);
  }
}

void SendStyleSheetChanged(InspectorTasmExecutor* executor,
                           const std::string& style_sheet_id) {
  if (executor && !style_sheet_id.empty()) {
    executor->SendCSSEventMsg(
        InspectorTasmExecutor::CssCdpEvent::STYLE_SHEET_CHANGED, style_sheet_id,
        nullptr);
  }
}

}  // namespace

InspectorTasmExecutor::InspectorTasmExecutor(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator, int view_id)
    : dom_use_compression_(false),
      dom_compression_threshold_(10240),
      element_root_(nullptr),
      tasm_(),
      devtool_mediator_wp_(devtool_mediator),
      view_id_(view_id) {}
InspectorTasmExecutor::InspectorTasmExecutor(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator,
    tasm::TemplateAssembler* tasm, int view_id)
    : dom_use_compression_(false),
      dom_compression_threshold_(10240),
      element_root_(nullptr),
      tasm_(tasm),
      devtool_mediator_wp_(devtool_mediator),
      view_id_(view_id) {}

void InspectorTasmExecutor::SetDevToolPlatformFacade(
    const std::shared_ptr<DevToolPlatformFacade>& devtool_platform_facade) {
  devtool_platform_facade_ = devtool_platform_facade;
}

const std::shared_ptr<WhiteBoardInspectorTasmDelegate>&
InspectorTasmExecutor::GetWhiteBoardInspectorDelegate() {
  if (white_board_inspector_delegate_ == nullptr) {
    white_board_inspector_delegate_ =
        std::make_shared<WhiteBoardInspectorTasmDelegate>(shared_from_this(),
                                                          view_id_);
  }
  return white_board_inspector_delegate_;
}

void InspectorTasmExecutor::SendDOMEventMsg(const DomCdpEvent& event_name,
                                            int nodeId, const std::string& name,
                                            int parentNodeId) {
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");

  Json::Value msg(Json::ValueType::objectValue);
  msg["params"] = Json::ValueType::objectValue;
  if (event_name == DomCdpEvent::DOCUMENT_UPDATED) {
    msg["method"] = "DOM.documentUpdated";
  } else if (event_name == DomCdpEvent::ATTRIBUTE_REMOVED) {
    msg["method"] = "DOM.attributeRemoved";
    msg["params"]["nodeId"] = nodeId;
    msg["params"]["name"] = name;
  } else if (event_name == DomCdpEvent::ATTRIBUTE_MODIFIED) {
    msg["method"] = "DOM.attributeModified";
    msg["params"]["nodeId"] = nodeId;
    msg["params"]["name"] = name;
    Element* ptr = GetElementById(nodeId);

    // At this point, if the element_root_
    // has not yet been set in the OnElementNodeAdded callback,
    // the DOM.attributeModified event will be dropped.
    if (ptr == nullptr) {
      return;
    }
    msg["params"]["value"] =
        ElementHelper::GetAttributesAsTextOfNode(ptr, name);
  } else if (event_name == DomCdpEvent::CHILD_NODE_REMOVED) {
    msg["method"] = "DOM.childNodeRemoved";
    msg["params"]["parentNodeId"] = parentNodeId;
    msg["params"]["nodeId"] = nodeId;
  } else {
    msg["method"] = "ERROR method";
  }

  devtool_mediator->RunOnDevToolThread(
      [devtool_mediator, msg]() mutable {
        devtool_mediator->SendCDPEvent(msg);
      },
      true);
}

void InspectorTasmExecutor::OnDocumentUpdated() {
  Json::Value msg(Json::ValueType::objectValue);
  SendDOMEventMsg(DomCdpEvent::DOCUMENT_UPDATED, -1, "", -1);
}

void InspectorTasmExecutor::OnElementNodeAdded(lynx::tasm::Element* ptr) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");

  if (ElementInspector::SelectorTag(ptr) == "page") {
    element_root_ = ptr;
#if LYNX_ENABLE_TRACING
    lynx::base::tracing::InstanceCounterTraceImpl::InitNodeCounter();
#endif
  } else {
    // For Radon diff test case as follows:
    // class Condition extends Component<{ condition: boolean,
    // removeComponentElement: true }> {
    //      render() {
    //        const { condition} = this.props;
    //        if (typeof condition !== 'boolean') {
    //          return null;
    //        }
    //        return condition ? <Loading1 /> : <Loading2 />;
    //      }
    //    }
    // when Loading1 is removed and Loading2 is added, Loading1 won't be
    // correctly removed from dom tree because it can't find
    // parentComponentElement which has  been moved to new RadonComponent.
    // Given Loading1 and Loading2 has the same parentComponentElement, and when
    // Loading1 is removed, it is actually remove parentComponentElement from
    // dom tree,so the effect of removing loading1 and loading2 is the same, and
    // even though loading1 doesn't exist,It doesn't matter if you send one more
    // message.
    // as above, before loading2 is added, remove it first.
    if (ElementInspector::GetParentComponentElementFromDataModel(ptr) &&
        ElementInspector::IsNeedEraseId(
            ElementInspector::GetParentComponentElementFromDataModel(ptr))) {
      OnElementNodeRemoved(ptr);
    }

    Element* parentNode = ptr->parent();
    if (parentNode != nullptr) {
      Json::Value msg(Json::ValueType::objectValue);
      msg["method"] = "DOM.childNodeInserted";
      msg["params"] = Json::ValueType::objectValue;
      msg["params"]["parentNodeId"] = ElementInspector::NodeId(parentNode);
      // TODO: tanxuelian.rovic
      // optimize data processing uniformly later, which can be done in the
      // following way

      // Json::Value msg(Json:ValueType:objectValue);
      // msg["method"] = "DOM.childNodeInserted";
      // Json::Value params(Json:ValueType:objectValue);
      // params["parentNodeId"] = ElementInspector::NodeId(parent);
      // msg["params"] = std::move(params);

      Element* previous_node = ElementHelper::GetPreviousNode(ptr);
      if (!previous_node) {
        msg["params"]["previousNodeId"] = 0;
      } else {
        Element* parent =
            ElementInspector::GetParentComponentElementFromDataModel(
                previous_node);
        while (parent && ElementInspector::IsNeedEraseId(parent)) {
          previous_node = parent;
          parent = ElementInspector::GetParentComponentElementFromDataModel(
              previous_node);
        }
        msg["params"]["previousNodeId"] =
            ElementInspector::NodeId(previous_node);
      }

      msg["params"]["node"] = ElementHelper::GetDocumentBodyFromNode(ptr);
      msg["compress"] = false;

      devtool_mediator->RunOnDevToolThread(
          [devtool_mediator, self = shared_from_this(), msg]() mutable {
            std::string params_str = msg["params"].toStyledString();
            if (self->dom_use_compression_ &&
                params_str.size() >
                    static_cast<size_t>(self->dom_compression_threshold_)) {
              InspectorUtil::CompressData("childNodeInserted",
                                          msg["params"].toStyledString(), msg,
                                          "params");
            }
            devtool_mediator->SendCDPEvent(msg);
          },
          true);
    }
  }
#if LYNX_ENABLE_TRACING
  lynx::base::tracing::InstanceCounterTraceImpl::IncrementNodeCounter(ptr);
#endif
}

void InspectorTasmExecutor::OnElementNodeRemoved(Element* ptr) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  Element* parent = ptr->parent();
  if (parent) {
    Element* remove_element = ptr;
    while (ElementInspector::GetParentComponentElementFromDataModel(
               remove_element) &&
           ElementInspector::IsNeedEraseId(
               ElementInspector::GetParentComponentElementFromDataModel(
                   remove_element))) {
      remove_element = ElementInspector::GetParentComponentElementFromDataModel(
          remove_element);
    }

    SendDOMEventMsg(DomCdpEvent::CHILD_NODE_REMOVED,
                    ElementInspector::NodeId(remove_element), "",
                    ElementInspector::NodeId(parent));
  }

#if LYNX_ENABLE_TRACING
  lynx::base::tracing::InstanceCounterTraceImpl::DecrementNodeCounter(ptr);
#endif
}

// not used yet
void InspectorTasmExecutor::OnCharacterDataModified(lynx::tasm::Element* ptr) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");

  Json::Value msg(Json::ValueType::objectValue);
  Json::Value res(Json::ValueType::objectValue);
  msg["method"] = "DOM.characterDataModified";
  msg["params"]["nodeId"] = ElementInspector::NodeId(ptr);
  msg["params"]["characterData"] = ElementHelper::GetStyleNodeText(ptr);
  devtool_mediator->SendCDPEvent(msg);
}

void InspectorTasmExecutor::OnElementDataModelSet(lynx::tasm::Element* ptr) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  DiffID(ptr);
  DiffAttr(ptr);
  DiffClass(ptr);
  DiffStyle(ptr);
}

void InspectorTasmExecutor::OnElementManagerWillDestroy() {
  tasm_ = nullptr;
  element_root_ = nullptr;
}

void InspectorTasmExecutor::DiffID(lynx::tasm::Element* ptr) {
  std::string old_id = ElementInspector::SelectorId(ptr);
  std::string new_id = ElementInspector::GetSelectorIDFromAttributeHolder(ptr);
  ElementInspector::SetSelectorId(ptr, new_id);
  if (!old_id.empty()) {
    ElementInspector::DeleteAttr(ptr, "id");
    SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_REMOVED,
                    ElementInspector::NodeId(ptr), "id", -1);
  }
  if (!new_id.empty()) {
    ElementInspector::UpdateAttr(ptr, "id", new_id);
    SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                    ElementInspector::NodeId(ptr), "id", -1);
  }
}

void InspectorTasmExecutor::DiffAttr(lynx::tasm::Element* ptr) {
  const auto old_attr = ElementInspector::AttrMap(ptr);
  const auto& new_attr =
      ElementInspector::GetAttrFromAttributeHolder(ptr).second;

  // Events are also a type of attribute, so when DiffAttr is performed, events
  // are also diffed.
  const auto old_event_attr = ElementInspector::EventMap(ptr);
  const auto& new_event_attr =
      ElementInspector::GetEventMapFromAttributeHolder(ptr).second;

  const auto old_data_attr = ElementInspector::DataMap(ptr);
  const auto& new_data_attr =
      ElementInspector::GetDataSetFromAttributeHolder(ptr).second;

  const auto& diff_attr_map_function = [this, ptr](const auto& new_attr,
                                                   const auto& old_attr) {
    for (const auto& pair : new_attr) {
      auto iter = old_attr.find(pair.first);
      if (iter == old_attr.end() || iter->second != pair.second) {
        ElementInspector::UpdateAttr(ptr, pair.first, pair.second);
        SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                        ElementInspector::NodeId(ptr), pair.first, -1);
      }
    }
    for (const auto& pair : old_attr) {
      if (new_attr.find(pair.first) == new_attr.end()) {
        ElementInspector::DeleteAttr(ptr, pair.first);
        SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_REMOVED,
                        ElementInspector::NodeId(ptr), pair.first, -1);
      }
    }
  };

  diff_attr_map_function(new_attr, old_attr);
  diff_attr_map_function(new_event_attr, old_event_attr);
  diff_attr_map_function(new_data_attr, old_data_attr);
}

void InspectorTasmExecutor::DiffClass(lynx::tasm::Element* ptr) {
  std::vector<std::string> old_class = ElementInspector::ClassOrder(ptr);
  std::vector<std::string> new_class =
      ElementInspector::GetClassOrderFromAttributeHolder(ptr);
  if (old_class != new_class) {
    ElementInspector::DeleteClasses(ptr);
    {
      SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_REMOVED,
                      ElementInspector::NodeId(ptr), "class", -1);
    }

    ElementInspector::UpdateClasses(ptr, new_class);
    {
      SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                      ElementInspector::NodeId(ptr), "class", -1);
    }
  }
}

void InspectorTasmExecutor::DiffStyle(lynx::tasm::Element* ptr) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  auto* inspector_attribute = ptr->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");

  std::unordered_map<std::string, std::string> old_style;
  for (auto& iter : inspector_attribute->inline_style_sheet_.css_properties_) {
    old_style[iter.first] = iter.second.value_;
  }
  auto new_style = ElementInspector::GetInlineStylesFromAttributeHolder(ptr);
  for (const auto& pair : new_style) {
    ElementInspector::UpdateStyle(ptr, pair.first, pair.second);
    {
      SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                      ElementInspector::NodeId(ptr), "style", -1);
    }
    for (const auto& pair : old_style) {
      if (new_style.find(pair.first) == new_style.end()) {
        ElementInspector::DeleteStyle(ptr, pair.first);
        {
          if (ElementInspector::GetInlineStyleSheet(ptr)
                  .css_properties_.empty()) {
            SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_REMOVED,
                            ElementInspector::NodeId(ptr), "style", -1);
          } else {
            SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                            ElementInspector::NodeId(ptr), "style", -1);
          }
        }
      }
    }
  }
}

void InspectorTasmExecutor::OnSetNativeProps(lynx::tasm::Element* ptr,
                                             const std::string& name,
                                             const std::string& value,
                                             bool is_style) {
  if (!ptr) {
    return;
  }
  if (is_style) {
    ElementInspector::UpdateStyle(ptr, name, value);
    {
      SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                      ElementInspector::NodeId(ptr), "style", -1);
    }
  } else {
    ElementInspector::UpdateAttr(ptr, name, value);
    {
      SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED,
                      ElementInspector::NodeId(ptr), name, -1);
    }
  }
}

lynx::tasm::Element* InspectorTasmExecutor::GetElementById(int node_id) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element_root_, "element_root_ is null",
                                  nullptr);
  auto* element_manager = element_root_->element_manager();
  CHECK_NULL_AND_LOG_RETURN_VALUE(element_manager, "element_manager is null",
                                  nullptr);
  auto* node_manager = element_manager->node_manager();
  CHECK_NULL_AND_LOG_RETURN_VALUE(node_manager, "node_manager is null",
                                  nullptr);
  return node_manager->Get(node_id);
}

lynx::tasm::Element* InspectorTasmExecutor::GetElementRoot() {
  return element_root_;
}

void InspectorTasmExecutor::QuerySelector(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string selector = params["selector"].asString();
  Element* start_node;
  if (params.isMember("nodeId")) {
    int node_id = params["nodeId"].asInt();
    start_node = GetElementById(node_id);
  } else {
    start_node = element_root_;
  }
  content["nodeId"] =
      start_node ? ElementHelper::QuerySelector(start_node, selector) : -1;

  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetAttributes(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  int node_id = params["nodeId"].asInt();
  Element* ptr = GetElementById(node_id);
  if (ptr) {
    content["attributes"] = ElementHelper::GetAttributesImpl(ptr);
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::InnerText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  int nodeId = params["nodeId"].asInt();
  Element* element = GetElementById(nodeId);
  Json::Value raw_text_value_array(Json::ValueType::arrayValue);
  // find all raw-text on text element
  if (element && !element->IsDetached() &&
      !ElementInspector::LocalName(element).compare("text")) {
    for (Element* raw_text_child : element->GetChildren()) {
      if (ElementInspector::LocalName(raw_text_child).compare("raw-text")) {
        continue;
      }
      auto itr = ElementInspector::AttrMap(raw_text_child).find("text");
      if (itr != ElementInspector::AttrMap(element).end()) {
        Json::Value attr_text_value(Json::ValueType::objectValue);
        attr_text_value["nodeId"] = ElementInspector::NodeId(raw_text_child);
        attr_text_value["text"] = itr->second;
        raw_text_value_array.append(attr_text_value);
      }
    }
  }
  content["nodeId"] = static_cast<int>(nodeId);
  content["rawTextValues"] = raw_text_value_array;
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::QuerySelectorAll(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string selector = params["selector"].asString();
  Element* start_node;
  if (params.isMember("nodeId")) {
    int node_id = params["nodeId"].asInt();
    start_node = GetElementById(node_id);
  } else {
    start_node = element_root_;
  }
  content["nodeIds"] =
      start_node ? ElementHelper::QuerySelectorAll(start_node, selector)
                 : Json::Value(Json::ValueType::arrayValue);

  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

const std::map<DevToolFunction, std::function<void(const lynx::base::any&)>>&
InspectorTasmExecutor::GetFunctionForElementMap() {
  static lynx::base::NoDestructor<
      std::map<DevToolFunction, std::function<void(const lynx::base::any&)>>>
      function_map{
          {{DevToolFunction::InitForInspector,
            &ElementInspector::InitForInspector},
           {DevToolFunction::InitPlugForInspector,
            &ElementInspector::InitPlugForInspector},
           {DevToolFunction::InitStyleValueElement,
            &ElementInspector::InitStyleValueElement},
           {DevToolFunction::InitStyleRoot, &ElementInspector::InitStyleRoot},
           {DevToolFunction::SetDocElement, &ElementInspector::SetDocElement},
           {DevToolFunction::SetStyleValueElement,
            &ElementInspector::SetStyleValueElement},
           {DevToolFunction::SetStyleRoot, &ElementInspector::SetStyleRoot}}};
  return *function_map;
}

void InspectorTasmExecutor::DOM_Enable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value params = message["params"];
  if (params.isMember("useCompression")) {
    dom_use_compression_ = params["useCompression"].asBool();
  }
  if (params.isMember("compressionThreshold")) {
    dom_compression_threshold_ = params["compressionThreshold"].asInt();
  }

  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::DOM_Disable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetDocument(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  int depth = -1;
  if (params.isMember("depth")) {
    depth = params["depth"].asInt() < -1 ? -1 : params["depth"].asInt();
  }
  if (element_root_ == nullptr) {
    response["result"] = content;
    response["id"] = message["id"].asInt64();
    sender->SendMessage("CDP", response);
    return;
  }

  content["root"] =
      ElementHelper::GetDocumentBodyFromNode(element_root_, depth);
  content["compress"] = false;

  response["result"] = content;
  response["id"] = message["id"].asInt64();

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->RunOnDevToolThread(
      [sender, self = shared_from_this(), content, response]() mutable {
        std::string root_str = content["root"].toStyledString();
        if (self->dom_use_compression_ &&
            root_str.size() >
                static_cast<size_t>(self->dom_compression_threshold_)) {
          InspectorUtil::CompressData("getDocument", root_str, content, "root");
        }
        response["result"] = content;
        sender->SendMessage("CDP", response);
      },
      true);
}

void InspectorTasmExecutor::DescribeNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  const Json::Value params = message["params"];
  int depth = 1;
  if (params.isMember("depth")) {
    depth = params["depth"].asInt() < -1 ? 1 : params["depth"].asInt();
  }

  Element* target = nullptr;
  if (params.isMember("nodeId")) {
    target = GetElementById(params["nodeId"].asInt());
  } else if (params.isMember("backendNodeId")) {
    target = GetElementById(params["backendNodeId"].asInt());
  }

  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  content["compress"] = false;
  response["id"] = message["id"].asInt64();
  if (target == nullptr) {
    response["result"] = content;
    sender->SendMessage("CDP", response);
    return;
  }

  content["node"] = ElementHelper::GetDocumentBodyFromNode(target, depth);

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->RunOnDevToolThread(
      [sender, self = shared_from_this(), content, response]() mutable {
        std::string node_str = content["node"].toStyledString();
        if (self->dom_use_compression_ &&
            node_str.size() >
                static_cast<size_t>(self->dom_compression_threshold_)) {
          InspectorUtil::CompressData("describeNode", node_str, content,
                                      "node");
        }
        response["result"] = content;
        sender->SendMessage("CDP", response);
      },
      true);
}

void InspectorTasmExecutor::GetDocumentWithBoxModel(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  tasm::Element* root = GetElementRoot();
  CHECK_NULL_AND_LOG_RETURN(root, "root is null");

  content["compress"] = false;
  response["id"] = message["id"].asInt64();

  auto send_response = [sender, response, self = shared_from_this()](
                           Json::Value content) mutable {
    auto devtool_mediator = self->devtool_mediator_wp_.lock();
    CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
    devtool_mediator->RunOnDevToolThread(
        [sender, self, content = std::move(content), response]() mutable {
          std::string root_str = content["root"].toStyledString();
          if (self->dom_use_compression_ &&
              root_str.size() >
                  static_cast<size_t>(self->dom_compression_threshold_)) {
            InspectorUtil::CompressData("getDocumentWithBoxModel", root_str,
                                        content, "root");
          }
          response["result"] = content;
          sender->SendMessage("CDP", response);
        },
        true);
  };

  float layouts_unit_per_px = 1.0f;
  auto* element_manager = root->element_manager();
  if (element_manager != nullptr) {
    layouts_unit_per_px =
        element_manager->GetLynxEnvConfig().LayoutsUnitPerPx();
  }
  const double screen_scale_factor = 1.0f;

  std::vector<InspectorBoxModelQuery> queries;
  std::vector<BoxModelNodePath> box_model_paths;
  content["root"] =
      GetDocumentBodyFromNodeWithBoxModel(root, queries, box_model_paths, {});

  size_t root_box_model_index = queries.size();
  std::string screen_shot_mode = DevToolStatus::GetInstance().GetStatus(
      DevToolStatus::kDevToolStatusKeyScreenShotMode,
      DevToolStatus::SCREENSHOT_MODE_FULLSCREEN);
  if (screen_shot_mode == DevToolStatus::SCREENSHOT_MODE_LYNXVIEW) {
    InspectorBoxModelQuery root_query;
    if (BuildBoxModelQuery(root, root_query)) {
      queries.push_back(std::move(root_query));
    }
  }

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  if (queries.empty()) {
    send_response(std::move(content));
    return;
  }
  devtool_mediator->GetBoxModels(
      queries, [content = std::move(content), send_response,
                box_model_paths = std::move(box_model_paths),
                root_box_model_index, screen_scale_factor, layouts_unit_per_px](
                   std::vector<std::vector<double>> box_models) mutable {
        std::vector<double> root_box_model;
        if (root_box_model_index < box_models.size()) {
          root_box_model = box_models[root_box_model_index];
        }
        ApplyBoxModelsToDocument(content["root"], box_model_paths, box_models,
                                 screen_scale_factor, layouts_unit_per_px,
                                 root_box_model);
        send_response(std::move(content));
      });
}

void InspectorTasmExecutor::RequestChildNodes(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  int node_id = params["nodeId"].asInt();
  [[maybe_unused]] int depth = 1;
  if (params.isMember("depth")) {
    depth = params["depth"].asInt();
  }
  Json::Value nodes(Json::ValueType::arrayValue);
  Element* cur_node = GetElementById(node_id);
  if (cur_node != nullptr) {
    for (Element* child : cur_node->GetChildren()) {
      Json::Value node_info(Json::ValueType::objectValue);
      node_info["parentId"] = ElementInspector::NodeId(child->parent());
      node_info["backendNodeId"] = 0;
      node_info["childNodeCount"] =
          static_cast<int>(child->GetChildren().size());
      node_info["localName"] = ElementInspector::LocalName(child);
      node_info["nodeId"] = ElementInspector::NodeId(child);
      node_info["nodeName"] = ElementInspector::NodeName(child);
      node_info["nodeType"] = ElementInspector::NodeType(child);
      node_info["nodeValue"] = ElementInspector::NodeValue(child);
      node_info["attributes"] = ElementHelper::GetAttributesImpl(child);
      ;
      nodes.append(node_info);
    }
  }

  content["parentId"] = node_id;
  content["nodes"] = nodes;
  response["result"] = content;
  response["id"] = message["id"].asInt64();

  // call method
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::DOM_GetBoxModel(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  int index = params["nodeId"].asInt();
  tasm::Element* ptr = GetElementById(index);
  double screen_scale_factor = 1.0f;
  response["id"] = message["id"].asInt64();

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");

  auto send_response = [sender, response, self = shared_from_this(),
                        devtool_mediator](Json::Value content) mutable {
    if (content.empty()) {
      content = BuildBoxModelError();
    }
    devtool_mediator->RunOnDevToolThread(
        [sender, self, content = std::move(content), response]() mutable {
          Json::Value result_response = response;
          result_response["result"] = content;
          sender->SendMessage("CDP", result_response);
        },
        true);
  };

  if (ptr == nullptr) {
    send_response(BuildBoxModelError());
    return;
  }

  if (ptr->IsDetached() || !ElementInspector::HasDataModel(ptr)) {
    send_response(BuildBoxModelError());
    return;
  }

  auto* element_manager = ptr->element_manager();
  if (element_manager == nullptr) {
    send_response(BuildBoxModelError());
    return;
  }
  const float layouts_unit_per_px =
      element_manager->GetLynxEnvConfig().LayoutsUnitPerPx();
  std::string screen_shot_mode = DevToolStatus::GetInstance().GetStatus(
      DevToolStatus::kDevToolStatusKeyScreenShotMode);
  InspectorBoxModelQuery query;
  if (!BuildBoxModelQuery(ptr, query)) {
    send_response(BuildBoxModelError());
    return;
  }

  bool need_root_box_model =
      screen_shot_mode == DevToolStatus::SCREENSHOT_MODE_LYNXVIEW &&
      GetElementRoot() != nullptr;
  InspectorBoxModelQuery root_query;
  if (need_root_box_model) {
    need_root_box_model = BuildBoxModelQuery(GetElementRoot(), root_query);
  }

  std::vector<InspectorBoxModelQuery> queries;
  queries.push_back(std::move(query));
  size_t root_box_model_index = queries.size();
  if (need_root_box_model) {
    queries.push_back(std::move(root_query));
  }
  devtool_mediator->GetBoxModels(
      queries, [send_response, screen_scale_factor, layouts_unit_per_px,
                root_box_model_index](
                   std::vector<std::vector<double>> box_models) mutable {
        std::vector<double> root_box_model;
        if (root_box_model_index < box_models.size()) {
          root_box_model = box_models[root_box_model_index];
        }
        if (box_models.empty()) {
          send_response(BuildBoxModelError());
          return;
        }
        send_response(BuildBoxModelResult(box_models[0], screen_scale_factor,
                                          layouts_unit_per_px, root_box_model));
      });
}

bool InspectorTasmExecutor::BuildBoxModelQuery(tasm::Element* element,
                                               InspectorBoxModelQuery& query) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", false);
  tasm::Element* target = element;
  while (IsVirtualOrWrapper(target)) {
    target = target->parent();
  }
  CHECK_NULL_AND_LOG_RETURN_VALUE(target, "target is null", false);

  query = InspectorBoxModelQuery();
  query.layout_object = BuildLayoutObjectInfo(target);
  query.transform_node = query.layout_object;
  std::string tag = target->GetTag().str();
  query.is_overlay = tag == "x-overlay-ng" || tag == "overlay";
  if (query.is_overlay) {
    query.overlay_box_model = ElementInspector::GetOverlayNGBoxModel(target);
  }
  query.has_ui_primitive = target->HasUIPrimitive();

  if (!query.has_ui_primitive) {
    tasm::Element* current = target;
    while (current != nullptr && !current->HasUIPrimitive()) {
      query.layout_only_nodes.push_back(BuildLayoutObjectInfo(current));
      do {
        current = current->parent();
      } while (current != nullptr && current->is_wrapper());
    }
    if (current != nullptr) {
      query.transform_node = BuildLayoutObjectInfo(current);
    } else {
      query.transform_node = InspectorLayoutObjectInfo();
    }
  }
  return query.layout_object.id != 0 && query.transform_node.id != 0;
}

void InspectorTasmExecutor::SetAttributesAsText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::vector<Json::Value> msg_v;
  int index = params["nodeId"].asInt();
  std::string name = params["name"].asString();
  std::string text = params["text"].asString();
  Element* ptr = GetElementById(index);
  if (ptr != nullptr && !ptr->IsDetached()) {
    msg_v = ElementHelper::SetAttributesAsText(ptr, name, text);
  }

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  for (const Json::Value& msg : msg_v) {
    devtool_mediator->SendCDPEvent(msg);
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// This protocol has not been implemented yet
void InspectorTasmExecutor::MarkUndoableState(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::PushNodesByBackendIdsToFrontend(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value nodeIds(Json::ValueType::arrayValue);
  Json::Value params = message["params"];
  nodeIds = params["backendNodeIds"];
  content["nodeIds"] = nodeIds;
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// This protocol has not been implemented yet
void InspectorTasmExecutor::RemoveNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value content(Json::ValueType::objectValue);
}

// This protocol has not been implemented yet
void InspectorTasmExecutor::MoveTo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value content(Json::ValueType::objectValue);
}

// This protocol has not been implemented yet
void InspectorTasmExecutor::CopyTo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value content(Json::ValueType::objectValue);
}

void InspectorTasmExecutor::GetOuterHTML(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  Element* ptr = GetElementById(static_cast<int>(index));
  if (ptr != nullptr) {
    content["outerHTML"] = ElementHelper::GetElementContent(ptr, 0);
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// This protocol has not been implemented yet
void InspectorTasmExecutor::SetOuterHTML(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value content(Json::ValueType::objectValue);
}

void InspectorTasmExecutor::SetInspectedNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::PerformSearch(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string query = params["query"].asString();
  std::string searchId = std::to_string(lynx::base::CurrentTimeMilliseconds());
  std::vector<int> searchResults;
  ElementHelper::PerformSearchFromNode(element_root_, query, searchResults);
  search_results_[searchId] = searchResults;
  content["searchId"] = searchId;
  content["resultCount"] = static_cast<int>(searchResults.size());
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetSearchResults(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string searchId = params["searchId"].asString();
  int fromIndex = params["fromIndex"].asInt();
  int toIndex = params["toIndex"].asInt();
  Json::Value nodeIds(Json::ValueType::arrayValue);
  auto iter = search_results_.find(searchId);
  if (iter != search_results_.end()) {
    std::vector<int> searchResults = iter->second;
    for (int index = fromIndex; index < toIndex; ++index) {
      if (index >= static_cast<int>(searchResults.size())) {
        break;
      }
      nodeIds.append(Json::Value(searchResults[index]));
    }
    content["nodeIds"] = nodeIds;
    response["result"] = content;
  } else {
    Json::Value error(Json::ValueType::objectValue);
    error["code"] = 32000;
    error["message"] = "SearchId not found.";
    response["error"] = error;
  }
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::DiscardSearchResults(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string searchId = params["searchId"].asString();
  if (search_results_.find(searchId) != search_results_.end()) {
    search_results_.erase(searchId);
    response["result"] = content;
  } else {
    Json::Value error(Json::ValueType::objectValue);
    error["code"] = 32000;
    error["message"] = "SearchId not found.";
    response["error"] = error;
  }
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetOriginalNodeIndex(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);

  Json::Value params = message["params"];
  size_t node_id = static_cast<size_t>(params["nodeId"].asInt64());

  Element* element = GetElementById(static_cast<int>(node_id));
  if (element != nullptr) {
    uint32_t node_index = element->NodeIndex();
    content["nodeIndex"] = node_index;
  }
  response["id"] = message["id"].asInt64();
  response["result"] = content;
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::ScrollIntoViewIfNeeded(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);

  Json::Value params = message["params"];
  size_t node_id = static_cast<size_t>(params["nodeId"].asInt64());
  Element* current_element = GetElementById(static_cast<int>(node_id));
  while (current_element != nullptr && (current_element->is_virtual() ||
                                        current_element->CanBeLayoutOnly())) {
    current_element = current_element->parent();
  }
  if (current_element == nullptr) {
    Json::Value error(Json::ValueType::objectValue);
    error["code"] = Json::Value(-32000);
    ;
    error["message"] = Json::Value("Element not found.");
    response["error"] = error;
    response["id"] = message["id"].asInt64();
    sender->SendMessage("CDP", response);
    return;
  }
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");

  devtool_mediator->ScrollIntoView(ElementInspector::NodeId(current_element));

  response["id"] = message["id"].asInt64();
  response["result"] = content;
  sender->SendMessage("CDP", response.toStyledString());
}

void InspectorTasmExecutor::DOM_Focus(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["id"] = message["id"].asInt64();

  const Json::Value& params = message["params"];
  if (!params.isObject() || !params.isMember("nodeId") ||
      (!params["nodeId"].isInt() && !params["nodeId"].isUInt())) {
    Json::Value error(Json::ValueType::objectValue);
    error["code"] = kInvalidParams;
    error["message"] = "DOM.focus requires a numeric nodeId parameter.";
    response["error"] = error;
    sender->SendMessage("CDP", response);
    return;
  }

  const Json::Value& node_id_value = params["nodeId"];
  Element* current_element = GetElementById(node_id_value.asInt());
  while (current_element != nullptr && (current_element->is_virtual() ||
                                        current_element->CanBeLayoutOnly())) {
    current_element = current_element->parent();
  }
  if (current_element == nullptr) {
    Json::Value error(Json::ValueType::objectValue);
    error["code"] = kServerError;
    error["message"] = "Element not found.";
    response["error"] = error;
    sender->SendMessage("CDP", response);
    return;
  }

  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");

  devtool_mediator->Focus(ElementInspector::NodeId(current_element));

  response["result"] = content;
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::WhiteBoardEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  HANDLE_WHITE_BOARD_METHOD(Enable, WhiteBoardEnable);
}

void InspectorTasmExecutor::WhiteBoardDisable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  HANDLE_WHITE_BOARD_METHOD(Disable, WhiteBoardDisable);
}

void InspectorTasmExecutor::WhiteBoardSetSharedData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  HANDLE_WHITE_BOARD_METHOD(SetSharedData, WhiteBoardSetSharedData);
}

void InspectorTasmExecutor::WhiteBoardGetSharedData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  HANDLE_WHITE_BOARD_METHOD(GetSharedData, WhiteBoardGetSharedData);
}

void InspectorTasmExecutor::WhiteBoardRemoveSharedData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  HANDLE_WHITE_BOARD_METHOD(RemoveSharedData, WhiteBoardRemoveSharedData);
}

void InspectorTasmExecutor::WhiteBoardClear(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  HANDLE_WHITE_BOARD_METHOD(Clear, WhiteBoardClear);
}

void InspectorTasmExecutor::DOMEnableDomTree(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv(
      lynx::tasm::LynxEnv::kLynxEnableDomTree, true);
  Json::Value params = message["params"];
  bool ignore_cache = false;
  if (!params.empty()) {
    ignore_cache = params["ignoreCache"].asBool();
  }
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->PageReload(ignore_cache);
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::DOMDisableDomTree(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv(
      lynx::tasm::LynxEnv::kLynxEnableDomTree, false);
  Json::Value params = message["params"];
  bool ignore_cache = false;
  if (!params.empty()) {
    ignore_cache = params["ignoreCache"].asBool();
  }
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->PageReload(ignore_cache);
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// CSS protocol
void GetElementByType(InspectorElementType type, std::vector<Element*>& res,
                      Element* root) {
  CHECK_NULL_AND_LOG_RETURN(root, "root is null");
  if (ElementInspector::Type(root) == type) {
    res.push_back(root);
    return;
  } else if (ElementInspector::Type(root) == InspectorElementType::COMPONENT) {
    Element* style_value = ElementInspector::StyleValueElement(root);
    GetElementByType(type, res, style_value);
  }

  Element* comp_ptr =
      ElementInspector::GetParentComponentElementFromDataModel(root);
  while (comp_ptr && ElementInspector::IsNeedEraseId(comp_ptr)) {
    Element* style_value = ElementInspector::StyleValueElement(comp_ptr);
    GetElementByType(type, res, style_value);

    comp_ptr =
        ElementInspector::GetParentComponentElementFromDataModel(comp_ptr);
  }

  if (!root->GetChildren().empty()) {
    for (Element* child : root->GetChildren()) {
      GetElementByType(type, res, child);
    }
  }
}

Json::Value InspectorTasmExecutor::GetUsageItem(
    const std::string& stylesheet_id, const std::string& content,
    const std::string& selector) {
  Json::Value usage_item(Json::ValueType::objectValue);
  usage_item["styleSheetId"] = stylesheet_id;

  // find the start index and end index of the selector in 'content'
  auto start_offset =
      content.find(selector + lynx::devtool::kPaddingCurlyBrackets);
  usage_item["startOffset"] = static_cast<Json::Int64>(start_offset);
  usage_item["endOffset"] =
      static_cast<Json::Int64>(content.find('\n', start_offset)) + 1;
  usage_item["used"] = true;
  return usage_item;
}

void InspectorTasmExecutor::SendCSSEventMsg(const CssCdpEvent& event_name,
                                            const std::string& style_sheet_id,
                                            lynx::tasm::Element* ptr) {
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  Json::Value msg(Json::ValueType::objectValue);
  msg["params"] = Json::Value(Json::ValueType::objectValue);
  if (event_name == CssCdpEvent::STYLE_SHEET_ADDED) {
    msg["method"] = "CSS.styleSheetAdded";
    if (ptr) {
      msg["params"]["header"] = ElementHelper::GetStyleSheetHeader(ptr);
    }
  } else if (event_name == CssCdpEvent::STYLE_SHEET_REMOVED) {
    // not called ever
    msg["method"] = "CSS.styleSheetRemoved";
    msg["params"]["styleSheetId"] = style_sheet_id;
  } else if (event_name == CssCdpEvent::STYLE_SHEET_CHANGED) {
    msg["method"] = "CSS.styleSheetChanged";
    msg["params"]["styleSheetId"] = style_sheet_id;
  } else if (event_name == CssCdpEvent::MEDIA_QUERY_RESULT_CHANGED) {
    msg["method"] = "CSS.mediaQueryResultChanged";
  } else {
    msg["method"] = "ERROR method";
  }
  devtool_mediator->SendCDPEvent(msg);
}

void InspectorTasmExecutor::OnCSSStyleSheetAdded(lynx::tasm::Element* ptr) {
  SendCSSEventMsg(CssCdpEvent::STYLE_SHEET_ADDED, "", ptr);
}

void InspectorTasmExecutor::OnCSSMediaQueryResultChanged() {
  SendCSSEventMsg(CssCdpEvent::MEDIA_QUERY_RESULT_CHANGED, "", nullptr);
}

void InspectorTasmExecutor::SendWhiteBoardEvent(const Json::Value& msg) {
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(
      devtool_mediator,
      "InspectorTasmExecutor::SendWhiteBoardEvent, devtool_mediator is null");
  devtool_mediator->SendCDPEvent(msg);
}

bool InspectorTasmExecutor::IsWhiteBoardEnabled() {
  if (white_board_inspector_delegate_ != nullptr) {
    return white_board_inspector_delegate_->IsEnabled();
  }
  return false;
}

void InspectorTasmExecutor::SetWhiteBoardEnabled(bool enable) {
  if (white_board_inspector_delegate_ != nullptr) {
    white_board_inspector_delegate_->SetEnabled(enable);
  }
}

// Enable CSS debugging, get all style sheet of current page
void InspectorTasmExecutor::CSS_Enable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);

  // then send styleSheetAdded event
  std::vector<Element*> style_values;
  GetElementByType(InspectorElementType::STYLEVALUE, style_values,
                   element_root_);
  for (Element* ptr : style_values) {
    if (ptr != nullptr &&
        ElementInspector::Type(ptr) == InspectorElementType::STYLEVALUE) {
      SendCSSEventMsg(CssCdpEvent::STYLE_SHEET_ADDED, "", ptr);
    }
  }
}

// This protocol has not been implemented yet
void InspectorTasmExecutor::CSS_Disable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetMatchedStylesForNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  Element* ptr = GetElementById(static_cast<int>(index));
  if (ptr != nullptr && !ptr->IsDetached()) {
    content = ElementHelper::GetMatchedStylesForNode(ptr);
  } else {
    Json::Value error = Json::Value(Json::ValueType::objectValue);
    error["code"] = Json::Value(-32000);
    error["message"] = Json::Value("Node is not an Element");
    content["error"] = error;
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetComputedStyleForNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  Element* ptr = GetElementById(static_cast<int>(index));
  if (ptr != nullptr && !ptr->IsDetached()) {
    ComputedStyleMap dict = BuildComputedStyleMap(ptr);
    if (ElementInspector::HasDataModel(ptr)) {
      auto* element_manager = ptr->element_manager();
      if (element_manager != nullptr) {
        const float layouts_unit_per_px =
            element_manager->GetLynxEnvConfig().LayoutsUnitPerPx();
        dict["font-size"] = lynx::tasm::CSSDecoder::ToPxValue(
            ptr->GetFontSize() / layouts_unit_per_px);
        InspectorBoxModelQuery query;
        if (BuildBoxModelQuery(ptr, query)) {
          response["id"] = message["id"].asInt64();
          auto devtool_mediator = devtool_mediator_wp_.lock();
          if (devtool_mediator) {
            devtool_mediator->GetBoxModel(
                query, [sender, self = shared_from_this(), devtool_mediator,
                        response, dict = std::move(dict), layouts_unit_per_px](
                           std::vector<double> box_info) mutable {
                  Json::Value content(Json::ValueType::objectValue);
                  ApplyBoxModelToComputedStyle(dict, box_info,
                                               layouts_unit_per_px);
                  content["computedStyle"] = ConvertComputedStyleToJson(dict);
                  devtool_mediator->RunOnDevToolThread(
                      [sender, self = std::move(self), response,
                       content = std::move(content)]() mutable {
                        response["result"] = content;
                        sender->SendMessage("CDP", response);
                      },
                      true);
                });
            return;
          }
        }
      }
    }
    content["computedStyle"] = ConvertComputedStyleToJson(dict);
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::GetInlineStylesForNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  Element* ptr = GetElementById(static_cast<int>(index));
  if (ptr != nullptr && !ptr->IsDetached()) {
    content["inlineStyle"] = ElementHelper::GetInlineStyleOfNode(ptr);
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::SetStyleTexts(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  Json::Value edits =
      params.get("edits", Json::Value(Json::ValueType::arrayValue));
  for (const Json::Value& edit : edits) {
    std::string style_sheet_id = edit["styleSheetId"].asString();
    int index =
        atoi(style_sheet_id.substr(style_sheet_id.find('.') + 1).c_str());
    Json::Value range_json =
        edit.get("range", Json::Value(Json::ValueType::objectValue));
    Range range;
    range.start_line_ = range_json["startLine"].asInt();
    range.start_column_ = range_json["startColumn"].asInt();
    range.end_line_ = range_json["endLine"].asInt();
    range.end_column_ = range_json["endColumn"].asInt();
    std::string text = edit["text"].asString();
    Element* ptr = GetElementById(static_cast<int>(index));
    if (ptr != nullptr) {
      ElementHelper::SetStyleTexts(element_root_, ptr, text, range);
      content =
          ElementHelper::GetStyleSheetAsTextOfNode(ptr, style_sheet_id, range);
      std::string res = content.toStyledString();
      Json::Value msg(Json::ValueType::objectValue);
      if (ElementInspector::Type(ptr) != InspectorElementType::STYLEVALUE ||
          ElementInspector::Type(ptr) != InspectorElementType::DOCUMENT) {
        int nodeid =
            atoi(style_sheet_id.substr(style_sheet_id.find('.') + 1).c_str());
        SendDOMEventMsg(DomCdpEvent::ATTRIBUTE_MODIFIED, nodeid, "style", -1);
      }
      SendCSSEventMsg(CssCdpEvent::STYLE_SHEET_CHANGED, style_sheet_id, ptr);
    }
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// Get the content of the specified style sheet ( Unsure of usage scenario)
void InspectorTasmExecutor::GetStyleSheetText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string style_sheet_id = params["styleSheetId"].asString();
  int index = atoi(style_sheet_id.substr(style_sheet_id.find('.') + 1).c_str());
  Element* ptr = GetElementById(static_cast<int>(index));
  if (ptr != nullptr) {
    content = ElementHelper::GetStyleSheetText(ptr, style_sheet_id);
  } else {
    Json::Value error = Json::Value(Json::ValueType::objectValue);
    error["code"] = Json::Value(-32000);
    error["message"] = Json::Value("Node is not an Element");
    content["error"] = error;
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// Get info of the specified node, Unsure of usage scenario
void InspectorTasmExecutor::GetBackgroundColors(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  Element* ptr = GetElementById(static_cast<int>(index));
  if (ptr != nullptr && !ptr->IsDetached()) {
    content = ElementHelper::GetBackGroundColorsOfNode(ptr);
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::SetStyleSheetText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string style_sheet_id = params["styleSheetId"].asString();
  content["sourceMapURL"] = "";
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

// Create a style sheet (not sure where to use it)
void InspectorTasmExecutor::CreateStyleSheet(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  Json::Value header = ElementHelper::CreateStyleSheet(element_root_);
  content["styleSheetId"] = header["styleSheetId"];
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);

  // then send styleSheetAdded event
  Json::Value msg(Json::ValueType::objectValue);
  msg["method"] = "CSS.styleSheetAdded";
  msg["params"]["header"] = header;
  devtool_mediator->SendCDPEvent(msg);
}

// Add a CSS rule (such as a class) to the specified style sheet (not sure where
// to use it)
void InspectorTasmExecutor::AddRule(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  std::string style_sheet_id = params["styleSheetId"].asString();
  std::string rule_text = params["ruleText"].asString();
  Range range;
  range.start_line_ = params["location"]["startLine"].asInt();
  range.start_column_ = params["location"]["startColumn"].asInt();
  range.end_line_ = params["location"]["endLine"].asInt();
  range.end_column_ = params["location"]["endColumn"].asInt();
  int index = atoi(style_sheet_id.substr(style_sheet_id.find('.') + 1).c_str());
  Element* ptr = GetElementById(index);
  response["result"] =
      ElementHelper::AddRule(ptr, style_sheet_id, rule_text, range);

  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::CollectDomTreeCssUsage(
    Json::Value& rule_usage_array, const std::string& stylesheet_id,
    const std::string& content) {
  Element* root = element_root_;
  CHECK_NULL_AND_LOG_RETURN(root, "root is null");
  std::queue<Element*> inspect_node_queue;
  inspect_node_queue.push(root);
  while (!inspect_node_queue.empty()) {
    Element* element = inspect_node_queue.front();
    inspect_node_queue.pop();
    for (Element* child : element->GetChildren()) {
      inspect_node_queue.push(child);
    }
    if (ElementInspector::Type(element) == InspectorElementType::DOCUMENT) {
      continue;
    }

    std::string select_id = ElementInspector::SelectorId(element);
    if (!select_id.empty()) {
      rule_usage_array.append(GetUsageItem(stylesheet_id, content, select_id));
    }

    std::vector<std::string> class_order =
        ElementInspector::ClassOrder(element);
    for (std::string& order : class_order) {
      if (!order.empty()) {
        rule_usage_array.append(GetUsageItem(stylesheet_id, content, order));
      }
    }
  }
}

// CSS.startRuleUsageTracking/CSS.updateRuleUsageTracking/CSS.stopRuleUsageTracking
// added for Perf (wangjiankang)
void InspectorTasmExecutor::StartRuleUsageTracking(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  rule_usage_tracking_ = true;
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::UpdateRuleUsageTracking(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (rule_usage_tracking_) {
    Json::Value selectors(Json::ValueType::arrayValue);
    selectors = message["params"]["selector"];
    for (const Json::Value& selector : selectors) {
      css_used_selector_.insert(selector.asString());
    }
  }
}

void InspectorTasmExecutor::StopRuleUsageTracking(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value res(Json::ValueType::objectValue);
  Json::Value rule_usage(Json::ValueType::arrayValue);

  std::vector<Element*> elements;
  GetElementByType(InspectorElementType::STYLEVALUE, elements, element_root_);
  Element* ptr = elements.empty() ? element_root_ : elements[0];
  std::string style_sheet_id = std::to_string(ElementInspector::NodeId(ptr));

  std::string content;
  Element* element_ptr = GetElementById(ElementInspector::NodeId(ptr));
  if (element_ptr != nullptr) {
    content =
        ElementHelper::GetStyleSheetText(element_ptr, style_sheet_id)["text"]
            .asString();
  }

  if (css_used_selector_.empty()) {
    CollectDomTreeCssUsage(rule_usage, style_sheet_id, content);
  } else {
    for (const std::string& selector : css_used_selector_) {
      if (!selector.empty()) {
        rule_usage.append(GetUsageItem(style_sheet_id, content, selector));
      }
    }
  }

  res["ruleUsage"] = rule_usage;
  response["result"] = res;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);

  css_used_selector_.clear();
  rule_usage_tracking_ = false;
}

void InspectorTasmExecutor::GetMediaQueries(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value result(Json::ValueType::objectValue);
  Json::Value medias(Json::ValueType::arrayValue);
  std::unordered_set<const css::RuleSet*> visited_rule_sets;
  std::unordered_set<const css::ConditionRule*> visited_condition_rules;
  std::unordered_map<std::string, int> media_indices;

  Element* root = element_root_;
  CHECK_NULL_AND_LOG_RETURN(root, "root is null");
  std::queue<Element*> inspect_node_queue;
  inspect_node_queue.push(root);
  while (!inspect_node_queue.empty()) {
    Element* element = inspect_node_queue.front();
    inspect_node_queue.pop();
    if (!element) continue;
    for (Element* child : element->GetChildren()) {
      inspect_node_queue.push(child);
    }

    tasm::CSSFragment* fragment = element->GetRelatedCSSFragment();
    if (!fragment) continue;
    Element* style_value = ElementInspector::StyleValueElement(element);
    std::string style_sheet_id;
    if (style_value) {
      style_sheet_id = std::to_string(ElementInspector::NodeId(style_value));
    }
    struct Ctx {
      Json::Value* medias;
      std::unordered_set<const css::RuleSet*>* visited_rule_sets;
      std::unordered_set<const css::ConditionRule*>* visited_condition_rules;
      const std::string* style_sheet_id;
      std::unordered_map<std::string, int>* media_indices;
    };
    Ctx ctx{&medias, &visited_rule_sets, &visited_condition_rules,
            &style_sheet_id, &media_indices};
    fragment->ForEachRuleSet(
        [](css::RuleSet* rs, void* cb_data) {
          auto* c = static_cast<Ctx*>(cb_data);
          if (!rs || !rs->HasMediaQueryRules()) return;
          if (!c->visited_rule_sets->insert(rs).second) return;
          auto condition_rule_visitor = [c](const auto& condition_rule) {
            if (!condition_rule.HasStructuredMediaQuery()) return;
            if (!c->visited_condition_rules->insert(&condition_rule).second) {
              return;
            }
            const auto& query_set = condition_rule.MediaQueries();
            int& media_index = (*c->media_indices)[*c->style_sheet_id];
            Json::Value media = BuildMediaObject(
                query_set->Serialize(), *c->style_sheet_id, media_index);
            c->medias->append(media);
            ++media_index;
          };
          rs->ForEachConditionRule(condition_rule_visitor);
        },
        &ctx);
  }

  result["medias"] = medias;
  response["result"] = result;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::SetMediaText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  const ConditionTextEditRequest request =
      ParseConditionTextEditRequest(message, "@media");

  Element* root = element_root_;
  if (!root || request.style_sheet_id.empty() || !request.has_valid_range ||
      request.condition_index < 0) {
    SendConditionTextEditError(sender, message, kInvalidParams,
                               "Invalid media query location");
    return;
  }

  auto media_queries = css::MediaQueryParser::ParseMediaQuerySet(request.text);
  ConditionRuleTarget target = FindConditionRuleByIndex(
      root, request.style_sheet_id, request.condition_index,
      &css::ConditionRule::HasStructuredMediaQuery);
  if (!target) {
    SendConditionTextEditError(sender, message, kServerError,
                               "Media rule not found");
    return;
  }

  target.rule->SetMediaQueries(media_queries);
  Json::Value updated_media =
      BuildMediaObject(media_queries->Serialize(), request.style_sheet_id,
                       request.condition_index);
  root->UpdateDynamicElementStyle(
      tasm::DynamicCSSStylesManager::kAllStyleUpdate, true);
  SyncCachedMediaInfo(target.style_root, request.style_sheet_id,
                      request.condition_index,
                      updated_media["text"].asString());
  RequestStyleResolveAfterConditionEdit(root);

  SendCSSEventMsg(CssCdpEvent::MEDIA_QUERY_RESULT_CHANGED, "", nullptr);
  SendStyleSheetChanged(this, request.style_sheet_id);
  SendConditionTextEditResult(sender, message, "media", updated_media);
}

void InspectorTasmExecutor::SetSupportsText(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  const ConditionTextEditRequest request =
      ParseConditionTextEditRequest(message, "@supports");

  Element* root = element_root_;
  if (!root || request.style_sheet_id.empty() || !request.has_valid_range ||
      request.condition_index < 0) {
    SendConditionTextEditError(sender, message, kInvalidParams,
                               "Invalid supports rule location");
    return;
  }

  auto supports_condition = css::SupportsConditionParser::Parse(request.text);
  if (!supports_condition) {
    SendConditionTextEditError(sender, message, kInvalidParams,
                               "Invalid supports condition");
    return;
  }

  ConditionRuleTarget target = FindConditionRuleByIndex(
      root, request.style_sheet_id, request.condition_index,
      &css::ConditionRule::HasStructuredSupportsRules);
  if (!target) {
    SendConditionTextEditError(sender, message, kServerError,
                               "Supports rule not found");
    return;
  }

  tasm::CSSParserConfigs default_parser_configs;
  const tasm::CSSParserConfigs* parser_configs = &default_parser_configs;
  if (auto* element_manager = root->element_manager()) {
    parser_configs = &element_manager->GetCSSParserConfigs();
  }
  css::SupportsEvaluator supports_evaluator(*parser_configs);

  target.rule->SetSupportsCondition(supports_condition);
  Json::Value updated_supports =
      BuildSupportsObject(supports_condition->Serialize(),
                          request.style_sheet_id, request.condition_index,
                          supports_evaluator.Eval(supports_condition.get()));
  root->UpdateDynamicElementStyle(
      tasm::DynamicCSSStylesManager::kAllStyleUpdate, true);
  SyncCachedSupportsInfo(target.style_root, request.style_sheet_id,
                         request.condition_index,
                         updated_supports["text"].asString());
  RequestStyleResolveAfterConditionEdit(root);

  SendStyleSheetChanged(this, request.style_sheet_id);
  SendConditionTextEditResult(sender, message, "supports", updated_supports);
}

// CSS protocol end

// Overlay protocol
void InspectorTasmExecutor::RestoreOriginNodeInlineStyle() {
  if (origin_node_id_ == 0) return;
  Element* origin_node = GetElementById(static_cast<int>(origin_node_id_));
  CHECK_NULL_AND_LOG_RETURN(origin_node, "origin_node is null");
  ElementHelper::SetInlineStyleSheet(origin_node, origin_inline_style_);
}

void InspectorTasmExecutor::HighlightNode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  if (!params.isNull()) {
    auto node_id = static_cast<size_t>(params["nodeId"].asInt64());
    auto highlight_config = params["highlightConfig"];
    auto selector = params["selector"].asString();
    auto* current_node = GetElementById(static_cast<int>(node_id));
    if (current_node == nullptr || current_node->IsDetached() ||
        !ElementInspector::HasDataModel(current_node) ||
        ElementInspector::IsNeedEraseId(current_node)) {
      Json::Value error = Json::Value(Json::ValueType::objectValue);
      error["code"] = Json::Value(-32000);
      error["message"] = Json::Value("Node is not an Element");
      content["error"] = error;
    } else {
      if (node_id != origin_node_id_) {
        RestoreOriginNodeInlineStyle();
        origin_inline_style_ = ElementHelper::GetInlineStyleTexts(current_node);
        origin_node_id_ = node_id;
        auto json_content_color = highlight_config["contentColor"];
        std::stringstream highlightStyle;
        highlightStyle << "background-color:"
                       << lynx::tasm::CSSDecoder::ToRgbaFromRgbaValue(
                              json_content_color["r"].asString(),
                              json_content_color["g"].asString(),
                              json_content_color["b"].asString(),
                              json_content_color["a"].asString())
                       << ";";

        std::string inline_style_str =
            highlightStyle.str() + origin_inline_style_.css_text_;
        ElementHelper::SetInlineStyleTexts(current_node, inline_style_str,
                                           Range());
      }
    }
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::HideHighlight(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  RestoreOriginNodeInlineStyle();
  sender->SendMessage("CDP", response);
}
// Overlay protocol end

void InspectorTasmExecutor::LynxGetProperties(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  content["properties"] = "";
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  auto* ptr = GetElementById(index);
  if (ptr && ElementInspector::Type(ptr) == InspectorElementType::COMPONENT) {
    content["properties"] = ElementHelper::GetProperties(ptr);
  } else {
    content["properties"] = "";
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::LynxGetData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  content["data"] = "";
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  auto* ptr = GetElementById(index);
  if (ptr && ElementInspector::Type(ptr) == InspectorElementType::COMPONENT) {
    content["data"] = ElementHelper::GetData(ptr);
  } else {
    content["data"] = "";
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::LynxGetComponentId(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  content["componentId"] = -1;
  Json::Value params = message["params"];
  size_t index = static_cast<size_t>(params["nodeId"].asInt64());
  auto* ptr = GetElementById(index);
  if (ptr) {
    if (ElementInspector::Type(ptr) == InspectorElementType::COMPONENT) {
      content["componentId"] = ElementHelper::GetComponentId(ptr);
    }
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::TemplateGetTemplateApiInfo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value result(Json::ValueType::objectValue);
  if (tasm_) {
    lynx::lepus::Value default_processor_value = tasm_->GetDefaultProcessor();
    result["useDefault"] = default_processor_value.IsClosure();
    std::unordered_map<std::string, lynx::lepus::Value> processor_map =
        tasm_->GetProcessorMap();
    if (!processor_map.empty()) {
      Json::Value keys(Json::ValueType::arrayValue);
      for (auto& element : processor_map) {
        keys.append(element.first);
      }
      result["processMapKeys"] = keys;
    }
  } else {
    result["useDefault"] = false;
  }

  response["result"] = result;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTasmExecutor::LayerTreeEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  sender->SendOKResponse(message["id"].asInt64());
  layer_tree_enabled_ = true;

  SendLayerPaintedEvent();
  SendLayerTreeDidChangeEvent();
}

void InspectorTasmExecutor::LayerTreeDisable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  sender->SendOKResponse(message["id"].asInt64());
  layer_tree_enabled_ = false;
}

void InspectorTasmExecutor::SendLayerTreeDidChangeEvent() {
  if (layer_tree_enabled_) {
    Json::Value response(Json::ValueType::objectValue);
    response["method"] = "LayerTree.layerTreeDidChange";
    Json::Value layers(Json::ValueType::arrayValue);
    std::vector<InspectorBoxModelQuery> queries;
    std::vector<LayerBoxModelInfo> box_model_infos;
    lynx::tasm::Element* element = GetElementRoot();

    if (element != nullptr) {
      layers = BuildLayerTreeJson(this, element, queries, box_model_infos);
    }
    auto devtool_mediator = devtool_mediator_wp_.lock();
    CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
    if (queries.empty()) {
      response["params"]["layers"] = layers;
      devtool_mediator->SendCDPEvent(response);
      return;
    }
    devtool_mediator->GetBoxModels(
        queries, [devtool_mediator, response, layers = std::move(layers),
                  box_model_infos = std::move(box_model_infos)](
                     std::vector<std::vector<double>> box_models) mutable {
          ApplyBoxModelsToLayers(layers, box_model_infos, box_models);
          response["params"]["layers"] = layers;
          devtool_mediator->SendCDPEvent(response);
        });
  }
}

void InspectorTasmExecutor::SendLayerPaintedEvent() {
  Json::Value response(Json::ValueType::objectValue);
  response["method"] = "LayerTree.layerPainted";
  Json::Value layerId(Json::ValueType::stringValue);
  Json::Value clip(Json::ValueType::objectValue);
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");

  lynx::tasm::Element* element = GetElementRoot();
  if (element != nullptr) {
    Json::Value root_layer = GetLayerContentFromElement(element);
    layerId = root_layer["layerId"].asString();
    std::vector<InspectorBoxModelQuery> queries;
    std::vector<LayerBoxModelInfo> box_model_infos;
    std::unordered_map<int32_t, size_t> box_model_query_indexes;
    AppendLayerBoxModelInfo(this, element, 0, queries, box_model_query_indexes,
                            box_model_infos);
    if (!queries.empty()) {
      Json::Value layers(Json::ValueType::arrayValue);
      layers.append(root_layer);
      devtool_mediator->GetBoxModels(
          queries, [devtool_mediator, response, layerId,
                    box_model_infos = std::move(box_model_infos),
                    layers = std::move(layers)](
                       std::vector<std::vector<double>> box_models) mutable {
            Json::Value clip(Json::ValueType::objectValue);
            ApplyBoxModelsToLayers(layers, box_model_infos, box_models);
            Json::Value& root_layer = layers[0];
            clip["x"] = root_layer["offsetX"];
            clip["y"] = root_layer["offsetY"];
            clip["width"] = root_layer["width"];
            clip["height"] = root_layer["height"];
            response["params"]["layerId"] = layerId;
            response["params"]["clip"] = clip;
            devtool_mediator->SendCDPEvent(response);
          });
      return;
    }
    clip["x"] = root_layer["offsetX"];
    clip["y"] = root_layer["offsetY"];
    clip["width"] = root_layer["width"];
    clip["height"] = root_layer["height"];
    layerId = root_layer["layerId"].asString();
  }
  response["params"]["layerId"] = layerId;
  response["params"]["clip"] = clip;
  devtool_mediator->SendCDPEvent(response);
}

void InspectorTasmExecutor::CompositingReasons(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value compositingReasons(Json::ValueType::arrayValue);
  Json::Value compositingReasonsIds(Json::ValueType::arrayValue);
  Json::Value params = message["params"];
  int layerId = std::stoi(params["layerId"].asString());
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  lynx::tasm::Element* element = GetElementById(layerId);

  if (element) {
    compositingReasons.append(ElementInspector::LocalName(element));
    compositingReasonsIds.append(ElementInspector::NodeId(element));
  }
  content["compositingReasons"] = compositingReasons;
  content["compositingReasonsIds"] = compositingReasonsIds;
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  devtool_mediator->SendCDPEvent(response);
}

Json::Value InspectorTasmExecutor::GetLayerContentFromElement(
    lynx::tasm::Element* element) {
  return BuildLayerContentJson(element);
}

void InspectorTasmExecutor::SendLayoutTree() {
  SendLayoutTreeWithCallback(nullptr);
}

void InspectorTasmExecutor::FlushLayoutTreeForReplayEnd(
    std::function<void()> callback) {
  SendLayoutTreeWithCallback(std::move(callback));
}

void InspectorTasmExecutor::SendLayoutTreeWithCallback(
    std::function<void()> callback) {
  auto root = GetElementRoot();
  if (root == nullptr) {
    if (callback) {
      callback();
    }
    return;
  }
  auto devtool_mediator = devtool_mediator_wp_.lock();
  if (!devtool_mediator) {
    LOGE("devtool_mediator is null");
    if (callback) {
      callback();
    }
    return;
  }
  devtool_mediator->GetLayoutTree(
      root->impl_id(),
      [callback = std::move(callback)](std::string layout_tree) mutable {
        if (!layout_tree.empty()) {
          lynx::tasm::replay::ReplayController::SendFileByAgent("Layout",
                                                                layout_tree);
        }
        if (callback) {
          callback();
        }
      });
}

void InspectorTasmExecutor::PageGetResourceContent(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  content["base64Encoded"] = false;
  std::string html_content = "";
  if (element_root_ != nullptr) {
    html_content = ElementHelper::GetElementContent(element_root_, 0);
  }
  content["content"] = html_content;
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

Json::Value InspectorTasmExecutor::GetDocumentBodyFromNodeWithBoxModel(
    tasm::Element* ptr, std::vector<InspectorBoxModelQuery>& queries,
    std::vector<std::vector<Json::ArrayIndex>>& box_model_paths,
    const std::vector<Json::ArrayIndex>& node_path) {
  Json::Value res = Json::Value(Json::ValueType::objectValue);

  CHECK_NULL_AND_LOG_RETURN_VALUE(ptr, "ptr is null", res);

  auto set_node_func = [this, &queries, &box_model_paths, &node_path](
                           Json::Value& res, Element* ptr) {
    ElementHelper::SetJsonValueOfNode(ptr, res);
    res["box_model"] = Json::Value();
    if (!ptr->IsDetached() && ElementInspector::HasDataModel(ptr)) {
      InspectorBoxModelQuery query;
      if (BuildBoxModelQuery(ptr, query)) {
        box_model_paths.push_back(node_path);
        queries.push_back(std::move(query));
      }
    }

    res["childNodeCount"] = static_cast<int>(ptr->GetChildren().size());
    res["children"] = Json::Value(Json::ValueType::arrayValue);
    Json::ArrayIndex child_index = 0;
    for (Element* child : ptr->GetChildren()) {
      std::vector<Json::ArrayIndex> child_path = node_path;
      child_path.push_back(child_index);
      res["children"].append(GetDocumentBodyFromNodeWithBoxModel(
          child, queries, box_model_paths, child_path));
      ++child_index;
    }
  };

  if (ElementInspector::Type(ptr) == InspectorElementType::COMPONENT) {
    size_t res_paths_begin = box_model_paths.size();
    set_node_func(res, ptr);
    if (ElementInspector::SelectorTag(ptr) == "page") {
      size_t doc_paths_begin = box_model_paths.size();
      Json::Value doc = Json::Value(Json::ValueType::objectValue);
      set_node_func(doc, ElementInspector::DocElement(ptr));
      doc["childNodeCount"] = 1;
      Json::ArrayIndex child_index = doc["children"].size();
      InsertChildIndexInPaths(box_model_paths, res_paths_begin, doc_paths_begin,
                              node_path.size(), child_index);
      doc["children"].append(res);
      return doc;
    }
  } else {
    set_node_func(res, ptr);
  }
  return res;
}

}  // namespace devtool
}  // namespace lynx
