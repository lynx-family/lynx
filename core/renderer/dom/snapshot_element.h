// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_SNAPSHOT_ELEMENT_H_
#define CORE_RENDERER_DOM_SNAPSHOT_ELEMENT_H_

#if ENABLE_TRACE_PERFETTO

#include <string>
#include <unordered_map>
#include <vector>

#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/ui_wrapper/painting/catalyzer.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace dom {

class SnapshotElement {
 public:
  bool flatten;
  short overflow;
  int32_t id;
  float width;
  float height;
  float left;
  float top;
  std::string name;
  base::String id_selector;
  tasm::ClassList classes;
  std::unordered_map<base::String, lepus::Value> attributes;
  std::vector<SnapshotElement*> children;
};
static rapidjson::Value DumpSnapshotElementTreeRecursively(
    SnapshotElement* node, rapidjson::Document& doc) {
  rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
  rapidjson::Value value;
  value.SetObject();
  value.AddMember("i", node->id, allocator);
  value.AddMember("w", node->width, allocator);
  value.AddMember("h", node->height, allocator);
  value.AddMember("l", node->left, allocator);
  value.AddMember("t", node->top, allocator);
  value.AddMember("n", node->name, allocator);
  value.AddMember("id", node->id_selector.str(), allocator);
  value.AddMember("f", node->flatten, allocator);
  value.AddMember("o", node->overflow, allocator);
  // classes
  if (node->classes.size() > 0) {
    rapidjson::Value class_array;
    class_array.SetArray();
    rapidjson::Value value_str(rapidjson::kStringType);
    for (const auto& clazz : node->classes) {
      auto size = static_cast<uint32_t>(clazz.str().size());
      value_str.SetString(clazz.str().c_str(), size, allocator);
      class_array.GetArray().PushBack(value_str, allocator);
    }
    value.AddMember("cl", class_array, allocator);
  }

  // attributes
  if (node->attributes.size() > 0) {
    rapidjson::Value attributes_value;
    attributes_value.SetObject();
    for (auto& it : node->attributes) {
      rapidjson::Value key((it.first).str(), allocator);
      lepus::Value lepus_val = it.second;
      if (lepus_val.IsString() &&
          lepus_val.StdString().size() <
              tasm::Catalyzer::kMaxAttributeValueLength) {
        rapidjson::Value val(lepus_val.StdString(), allocator);
        attributes_value.AddMember(key, val, allocator);
      } else if (lepus_val.IsNumber()) {
        attributes_value.AddMember(key, lepus_val.Number(), allocator);
      }
    }
    value.AddMember("at", attributes_value, allocator);
  }

  if (node->children.size() > 0) {
    rapidjson::Value children_json;
    children_json.SetArray();

    for (auto child : node->children) {
      children_json.GetArray().PushBack(
          DumpSnapshotElementTreeRecursively(child, doc), allocator);
    }
    value.AddMember("c", children_json, allocator);
  }
  return value;
}

static SnapshotElement* constructSnapshotElementTree(
    lynx::tasm::Element* node) {
  SnapshotElement* root = new SnapshotElement();
  root->width = node->width();
  root->height = node->height();
  root->left = node->left();
  root->top = node->top();
  root->name = node->GetTag().str();
  root->id_selector = node->data_model()->idSelector();
  root->classes = node->data_model()->classes();
  root->attributes = node->data_model()->attributes();
  root->id = node->impl_id();
  root->flatten = node->TendToFlatten();
  root->overflow = node->overflow();

  root->children = std::vector<SnapshotElement*>();

  for (lynx::tasm::Element* child : node->GetChildren()) {
    root->children.push_back(constructSnapshotElementTree(child));
  }
  return root;
}

}  // namespace dom
}  // namespace lynx

#endif  // ENABLE_TRACE_PERFETTO

#endif  // CORE_RENDERER_DOM_SNAPSHOT_ELEMENT_H_
