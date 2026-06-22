// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/lynx_element_query.h"

#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/base_string.h"
#include "base/include/value/table.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/fiber_node_info.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace tasm {
namespace {

Element* RootElement(ElementManager* manager) {
  if (manager == nullptr) {
    return nullptr;
  }
  return manager->root();
}

Element* ElementBySign(ElementManager* manager, int32_t sign) {
  if (manager == nullptr || manager->node_manager() == nullptr) {
    return nullptr;
  }
  if (sign == LynxElementQuery::kInvalidSign) {
    return nullptr;
  }
  Element* root = RootElement(manager);
  if (root != nullptr && root->impl_id() == sign) {
    return root;
  }
  return manager->node_manager()->Get(sign);
}

bool ShouldExposeAttribute(const base::String& key, const lepus::Value& value) {
  return key != AttributeHolder::kIdSelectorAttrName && !value.IsJSFunction() &&
         !value.IsNil() && !value.IsUndefined();
}

lepus::Value GetElementFields(Element* element,
                              const std::vector<std::string>& fields) {
  if (element == nullptr) {
    return lepus::Value(lepus::Dictionary::Create());
  }
  return FiberNodeInfo::GetNodeInfo(static_cast<FiberElement*>(element),
                                    fields);
}

lepus::Value GetField(Element* element, const std::string& field) {
  auto fields = GetElementFields(element, {field});
  if (!fields.IsTable()) {
    return lepus::Value();
  }
  return fields.Table()->GetValue(base::String(field));
}

Element* FindByIdInSubtree(Element* root, const base::String& id) {
  if (root == nullptr) {
    return nullptr;
  }
  if (root->GetIdSelector() == id) {
    return root;
  }
  for (auto* child : root->GetChildren()) {
    if (auto* result = FindByIdInSubtree(child, id)) {
      return result;
    }
  }
  return nullptr;
}

rapidjson::Value LepusValueToJson(
    const lepus::Value& value, rapidjson::Document::AllocatorType& allocator) {
  if (value.IsNil() || value.IsUndefined()) {
    return rapidjson::Value(rapidjson::kNullType);
  }
  if (value.IsBool()) {
    return rapidjson::Value(value.Bool());
  }
  if (value.IsNumber()) {
    return rapidjson::Value(value.Number());
  }
  if (value.IsString()) {
    return rapidjson::Value(value.StdString().c_str(), allocator);
  }
  if (value.IsArray()) {
    rapidjson::Value array(rapidjson::kArrayType);
    for (size_t index = 0; index < value.Array()->size(); ++index) {
      array.PushBack(LepusValueToJson(value.Array()->get(index), allocator),
                     allocator);
    }
    return array;
  }
  if (value.IsTable()) {
    rapidjson::Value object(rapidjson::kObjectType);
    for (const auto& pair : *value.Table()) {
      object.AddMember(rapidjson::Value(pair.first.c_str(), allocator),
                       LepusValueToJson(pair.second, allocator), allocator);
    }
    return object;
  }
  return rapidjson::Value(rapidjson::kNullType);
}

rapidjson::Value AttrMapToJson(const AttrMap& map,
                               rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value object(rapidjson::kObjectType);
  for (const auto& pair : map) {
    object.AddMember(rapidjson::Value(pair.first.c_str(), allocator),
                     LepusValueToJson(pair.second, allocator), allocator);
  }
  return object;
}

rapidjson::Value AttributesToJson(
    Element* element, rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value object(rapidjson::kObjectType);
  if (element == nullptr || element->data_model() == nullptr) {
    return object;
  }
  for (const auto& pair : element->data_model()->attributes()) {
    if (ShouldExposeAttribute(pair.first, pair.second)) {
      object.AddMember(rapidjson::Value(pair.first.c_str(), allocator),
                       LepusValueToJson(pair.second, allocator), allocator);
    }
  }
  return object;
}

rapidjson::Value PositionInfoToJson(
    Element* element, rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value object(rapidjson::kObjectType);
  if (element == nullptr) {
    return object;
  }
  object.AddMember(rapidjson::StringRef(LynxElementQuery::kLeftKey),
                   element->left(), allocator);
  object.AddMember(rapidjson::StringRef(LynxElementQuery::kTopKey),
                   element->top(), allocator);
  object.AddMember(rapidjson::StringRef(LynxElementQuery::kWidthKey),
                   element->width(), allocator);
  object.AddMember(rapidjson::StringRef(LynxElementQuery::kHeightKey),
                   element->height(), allocator);
  return object;
}

lepus::Value PositionInfoToLepusValue(const LynxElementPositionInfo& info) {
  auto table = lepus::Dictionary::Create();
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kLeftKey), info.left);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kTopKey), info.top);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kWidthKey), info.width);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kHeightKey),
                  info.height);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kPaddingLeftKey),
                  info.padding_left);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kPaddingTopKey),
                  info.padding_top);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kPaddingRightKey),
                  info.padding_right);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kPaddingBottomKey),
                  info.padding_bottom);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kMarginLeftKey),
                  info.margin_left);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kMarginTopKey),
                  info.margin_top);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kMarginRightKey),
                  info.margin_right);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kMarginBottomKey),
                  info.margin_bottom);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kBorderLeftWidthKey),
                  info.border_left_width);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kBorderTopWidthKey),
                  info.border_top_width);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kBorderRightWidthKey),
                  info.border_right_width);
  table->SetValue(BASE_STATIC_STRING(LynxElementQuery::kBorderBottomWidthKey),
                  info.border_bottom_width);
  return lepus::Value(table);
}

void DumpElement(Element* element, rapidjson::Value& output,
                 rapidjson::Document::AllocatorType& allocator) {
  output.SetObject();
  if (element == nullptr) {
    return;
  }
  output.AddMember(rapidjson::StringRef(LynxElementQuery::kSignKey),
                   element->impl_id(), allocator);
  output.AddMember(rapidjson::StringRef(LynxElementQuery::kTagNameKey),
                   rapidjson::Value(element->GetTag().c_str(), allocator),
                   allocator);
  output.AddMember(
      rapidjson::StringRef(LynxElementQuery::kIdKey),
      rapidjson::Value(element->GetIdSelector().c_str(), allocator), allocator);
  output.AddMember(LynxElementQuery::kPositionInfoKey,
                   PositionInfoToJson(element, allocator), allocator);
  output.AddMember(LynxElementQuery::kDatasetKey,
                   AttrMapToJson(element->dataset(), allocator), allocator);
  output.AddMember(LynxElementQuery::kAttributesKey,
                   AttributesToJson(element, allocator), allocator);

  rapidjson::Value children(rapidjson::kArrayType);
  for (auto* child : element->GetChildren()) {
    rapidjson::Value child_json;
    DumpElement(child, child_json, allocator);
    children.PushBack(child_json, allocator);
  }
  output.AddMember(LynxElementQuery::kChildrenKey, children, allocator);
}

}  // namespace

int32_t LynxElementQuery::GetRootSign(ElementManager* manager) {
  Element* root = RootElement(manager);
  return root ? root->impl_id() : kInvalidSign;
}

bool LynxElementQuery::IsAlive(ElementManager* manager, int32_t sign) {
  return ElementBySign(manager, sign) != nullptr;
}

bool LynxElementQuery::GetIdentity(ElementManager* manager, int32_t sign,
                                   LynxElementIdentity& identity) {
  Element* element = ElementBySign(manager, sign);
  if (element == nullptr) {
    return false;
  }
  auto fields =
      GetElementFields(element, {kUniqueIdFieldKey, kTagFieldKey, kIdKey});
  if (!fields.IsTable()) {
    return false;
  }
  auto sign_value =
      fields.Table()->GetValue(BASE_STATIC_STRING(kUniqueIdFieldKey));
  auto tag_value = fields.Table()->GetValue(BASE_STATIC_STRING(kTagFieldKey));
  auto id_value = fields.Table()->GetValue(BASE_STATIC_STRING(kIdKey));
  identity.sign = sign_value.IsNumber()
                      ? static_cast<int32_t>(sign_value.Number())
                      : element->impl_id();
  identity.tag = tag_value.IsString() ? tag_value.StdString() : std::string();
  identity.id = id_value.IsString() ? id_value.StdString() : std::string();
  return true;
}

int32_t LynxElementQuery::GetParentSign(ElementManager* manager, int32_t sign) {
  Element* element = ElementBySign(manager, sign);
  Element* parent = element ? element->parent() : nullptr;
  return parent ? parent->impl_id() : kInvalidSign;
}

bool LynxElementQuery::GetChildrenSigns(ElementManager* manager, int32_t sign,
                                        std::vector<int32_t>& children) {
  Element* element = ElementBySign(manager, sign);
  if (element == nullptr) {
    return false;
  }
  children.clear();
  for (auto* child : element->GetChildren()) {
    if (child != nullptr) {
      children.push_back(child->impl_id());
    }
  }
  return true;
}

int32_t LynxElementQuery::FindById(ElementManager* manager, int32_t root_sign,
                                   std::string id) {
  Element* root = ElementBySign(manager, root_sign);
  if (root == nullptr) {
    return kInvalidSign;
  }
  auto* element = FindByIdInSubtree(root, base::String(std::move(id)));
  return element == nullptr ? kInvalidSign : element->impl_id();
}

bool LynxElementQuery::GetPositionInfo(ElementManager* manager, int32_t sign,
                                       LynxElementPositionInfo& info) {
  Element* element = ElementBySign(manager, sign);
  if (element == nullptr) {
    return false;
  }
  info.left = element->left();
  info.top = element->top();
  info.width = element->width();
  info.height = element->height();
  const auto& paddings = element->paddings();
  const auto& margins = element->margins();
  const auto& borders = element->borders();
  info.padding_left = paddings[0];
  info.padding_top = paddings[1];
  info.padding_right = paddings[2];
  info.padding_bottom = paddings[3];
  info.margin_left = margins[0];
  info.margin_top = margins[1];
  info.margin_right = margins[2];
  info.margin_bottom = margins[3];
  info.border_left_width = borders[0];
  info.border_top_width = borders[1];
  info.border_right_width = borders[2];
  info.border_bottom_width = borders[3];
  return true;
}

lepus::Value LynxElementQuery::GetPositionInfoValue(ElementManager* manager,
                                                    int32_t sign) {
  LynxElementPositionInfo info;
  if (!GetPositionInfo(manager, sign, info)) {
    return lepus::Value();
  }
  return PositionInfoToLepusValue(info);
}

lepus::Value LynxElementQuery::GetDataset(ElementManager* manager,
                                          int32_t sign) {
  Element* element = ElementBySign(manager, sign);
  if (element == nullptr) {
    return lepus::Value();
  }
  return GetField(element, kDatasetKey);
}

lepus::Value LynxElementQuery::GetAttributes(ElementManager* manager,
                                             int32_t sign) {
  Element* element = ElementBySign(manager, sign);
  if (element == nullptr) {
    return lepus::Value();
  }
  return GetField(element, kAttributeFieldKey);
}

lepus::Value LynxElementQuery::GetAttribute(ElementManager* manager,
                                            int32_t sign, std::string name) {
  Element* element = ElementBySign(manager, sign);
  if (element == nullptr) {
    return lepus::Value();
  }
  if (name == AttributeHolder::kIdSelectorAttrName) {
    return lepus::Value();
  }
  auto attributes = GetAttributes(manager, sign);
  if (!attributes.IsTable()) {
    return lepus::Value();
  }
  return attributes.Table()->GetValue(base::String(std::move(name)));
}

std::string LynxElementQuery::ToJSONString(ElementManager* manager,
                                           int32_t root_sign) {
  Element* root = ElementBySign(manager, root_sign);
  if (root == nullptr) {
    return "{}";
  }
  rapidjson::Document doc;
  auto& allocator = doc.GetAllocator();
  DumpElement(root, doc, allocator);
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

}  // namespace tasm
}  // namespace lynx
