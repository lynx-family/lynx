// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "testing/lynx/tasm/databinding/element_dump_helper.h"

#include <algorithm>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <string>

#include "core/renderer/css/css_decoder.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/ui_wrapper/common/testing/prop_bundle_mock.h"
#include "core/runtime/vm/lepus/json_parser.h"

namespace lynx {
namespace tasm {

namespace {

static const std::string DumpRadonNodeTypeStrings[] = {
    "RadonUnknown",  "RadonNode",  "RadonComponent",
    "RadonPage",     "RadonSlot",  "RadonPlug",
    "RadonListNode", "RadonBlock", "RadonDynamicComponent",
};

}

std::string ElementDumpHelper::DumpElement(Element* element) {
  if (!element) {
    return std::string();
  }

  rapidjson::Document dumped_document;
  rapidjson::Value dumped_virtual_tree = DumpToJSON(element, dumped_document);
  dumped_document.Swap(dumped_virtual_tree);

  rapidjson::StringBuffer buffer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
  dumped_document.Accept(writer);

  return buffer.GetString();
}

rapidjson::Value ElementDumpHelper::DumpToJSON(Element* element,
                                               rapidjson::Document& doc) {
  rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
  rapidjson::Value value;
  value.SetObject();

  std::shared_ptr<PropBundleMock> prop_bundle_mock_ptr =
      std::static_pointer_cast<PropBundleMock>(element->pre_prop_bundle_);
  if (prop_bundle_mock_ptr) {
    const std::map<std::string, lepus::Value>& prop_bundle_mock_map =
        prop_bundle_mock_ptr->GetPropsMap();
    for (auto it = prop_bundle_mock_map.begin();
         it != prop_bundle_mock_map.end(); ++it) {
      const auto& key_str = std::string(it->first);
      rapidjson::Value key(key_str, allocator);

      rapidjson::Document temp_doc;
      const auto& value_str = lepus::lepusValueToString(it->second);
      temp_doc.Parse(value_str);
      if (temp_doc.IsObject()) {
        std::sort(temp_doc.MemberBegin(), temp_doc.MemberEnd(),
                  [](const rapidjson::Value::Member& lhs,
                     const rapidjson::Value::Member& rhs) {
                    return std::strcmp(lhs.name.GetString(),
                                       rhs.name.GetString()) < 0;
                  });
      }

      rapidjson::Value val;
      val.SetObject();
      val.CopyFrom(temp_doc, allocator);
      value.AddMember(key, val, allocator);
    }
  }

  value.AddMember("Tag", element->tag_.str(), allocator);
  value.AddMember("Font Size", element->GetFontSize(), allocator);

  auto children_size = static_cast<uint32_t>(element->GetChildren().size());
  value.AddMember("Child Count", children_size, allocator);
  if (children_size > 0) {
    rapidjson::Value children;
    children.SetArray();
    for (auto&& child : element->GetChildren()) {
      children.GetArray().PushBack(DumpToJSON(child, doc), allocator);
    }
    value.AddMember("Children", children, allocator);
  }
  return value;
}

lepus::Value ElementDumpHelper::DumpToSnapshot(Element* element,
                                               bool skip_children) {
  auto jsx = lepus::Dictionary::Create();
  auto props = lepus::Dictionary::Create();
  auto children = lepus::CArray::Create();

  auto data_model = element->data_model();
  if (data_model) {
    DumpAttributeToLepusValue(props, data_model);
    if (element->is_radon_element()) {
      auto* radon_node = static_cast<RadonNode*>(data_model->radon_node_ptr());
      if (radon_node->IsRadonComponent()) {
        auto* component = static_cast<RadonComponent*>(radon_node);
        if (component) {
          props->SetValue("name", component->name());
          props->SetValue("path", component->path());
        }
      }
    }

    tasm::StyleMap computed_styles;
    if (element->is_fiber_element()) {
      static_cast<FiberElement*>(element)->DumpStyle(computed_styles);
    } else if (element->is_radon_element()) {
      static_cast<RadonElement*>(element)->ResolveStyle(computed_styles);
    }

    if (computed_styles.size() > 0) {
      std::map<CSSPropertyID, CSSValue> ordered_computed_styles_map(
          computed_styles.begin(), computed_styles.end());

      auto styles = lepus::Dictionary::Create();
      for (auto& it : ordered_computed_styles_map) {
        styles->SetValue(
            CSSProperty::GetPropertyName(it.first),
            tasm::CSSDecoder::CSSValueToString(it.first, it.second));
      }

      // The name `computed-style` is from DOM spec
      // https://developer.mozilla.org/en-US/docs/Web/API/Window/getComputedStyle
      props->SetValue("computed-style", styles);
    }
  }

  if (skip_children == false && element->GetChildren().size() > 0) {
    for (auto&& child : element->GetChildren()) {
      children->push_back(ElementDumpHelper::DumpToSnapshot(child));
    }
  }

  jsx->SetValue("type", element->tag_);
  jsx->SetValue("props", props);
  jsx->SetValue("children", children);
  jsx->SetValue("$$typeof", 0);

  return lepus::Value(jsx);
}

void ElementDumpHelper::DumpToMarkup(Element* element, std::ostringstream& ss,
                                     std::string indent, bool skip_children) {
  ss << indent;
  ss << "<";
  ss << element->tag_.str();

  auto data_model = element->data_model();
  if (data_model) {
    if (element->is_radon_element()) {
      auto* radon_node = static_cast<RadonNode*>(data_model->radon_node_ptr());
      if (radon_node->IsRadonComponent()) {
        auto* component = static_cast<RadonComponent*>(radon_node);
        if (component) {
          ss << " name=\"" << component->name().str() << "\"";
          ss << " path=\"" << component->path().str() << "\"";
        }
      }
    }
    DumpAttributeToMarkup(ss, data_model);

    tasm::StyleMap computed_styles;
    if (element->is_fiber_element()) {
      static_cast<FiberElement*>(element)->DumpStyle(computed_styles);
    } else if (element->is_radon_element()) {
      static_cast<RadonElement*>(element)->ResolveStyle(computed_styles);
    }
    if (computed_styles.size() > 0) {
      std::map<CSSPropertyID, CSSValue> ordered_computed_styles_map(
          computed_styles.begin(), computed_styles.end());

      ss << " computed-style=\"";
      for (auto& it : ordered_computed_styles_map) {
        ss << CSSProperty::GetPropertyName(it.first).str() << ": "
           << tasm::CSSDecoder::CSSValueToString(it.first, it.second) << "; ";
      }
      ss << "\"";
    }
  }

  if (skip_children == false && element->GetChildren().size() > 0) {
    ss << ">";
    for (auto&& child : element->GetChildren()) {
      ss << "\n";
      ElementDumpHelper::DumpToMarkup(child, ss, indent + "  ");
    }
    ss << "\n";
    ss << indent;
    ss << "</" << element->tag_.str() << ">";
  } else {
    ss << "/>";
  }
}

std::string ElementDumpHelper::DumpTree(PageProxy* proxy) {
  rapidjson::Document dumped_document;
  rapidjson::Value dumped_virtual_tree;
  if (proxy->radon_page_) {
    dumped_virtual_tree = ElementDumpHelper::RadonBaseDumpToJSON(
        dumped_document, proxy->radon_page_.get());
  } else if (proxy->client_ && proxy->client_->GetEnableFiberArch()) {
    auto page = proxy->client_->root();
    if (page->is_fiber_element()) {
      dumped_virtual_tree = DumpFiberElementToJSON(
          dumped_document, static_cast<FiberElement*>(page));
    }
  } else {
    return std::string();
  }

  dumped_document.Swap(dumped_virtual_tree);
  rapidjson::StringBuffer buffer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
  dumped_document.Accept(writer);
  return buffer.GetString();
}

rapidjson::Value ElementDumpHelper::DumpComponentInfoMap(
    rapidjson::Document& doc, RadonComponent* component) {
  auto comp = [](const base::String& lhs, const base::String& rhs) {
    return lhs.str() < rhs.str();
  };
  std::map<base::String, lepus::Value, decltype(comp)>
      ordered_component_info_map(
          component->GetComponentInfoMap().Table()->begin(),
          component->GetComponentInfoMap().Table()->end(), comp);

  rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
  rapidjson::Value component_info_value;
  component_info_value.SetObject();

  for (auto it = ordered_component_info_map.begin();
       it != ordered_component_info_map.end(); ++it) {
    rapidjson::Value key((it->first).str(), allocator);
    lepus::Value lepus_val = it->second;
    std::string json_str = lepusValueToJSONString(lepus_val);
    rapidjson::Value val(json_str, allocator);
    component_info_value.AddMember(key, val, allocator);
  }
  return component_info_value;
}

rapidjson::Value ElementDumpHelper::RadonBaseDumpToJSON(
    rapidjson::Document& doc, RadonBase* base) {
  if (base->IsRadonNode()) {
    return RadonNodeDumpToJSON(doc, static_cast<RadonNode*>(base));
  }
  rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
  rapidjson::Value value;
  value.SetObject();

  value.AddMember("Type", DumpRadonNodeTypeStrings[base->NodeType() + 1],
                  allocator);
  value.AddMember("Tag", base->tag_name_.str(), allocator);
  if (!base->radon_component_->name().empty()) {
    value.AddMember("Radon Component", base->radon_component_->name().str(),
                    allocator);
  }

  auto radon_children_size =
      static_cast<uint32_t>(base->radon_children_.size());
  value.AddMember("child count", radon_children_size, allocator);

  if (radon_children_size > 0) {
    rapidjson::Value children;
    children.SetArray();
    for (auto&& child : base->radon_children_) {
      children.GetArray().PushBack(RadonBaseDumpToJSON(doc, child.get()),
                                   allocator);
    }
    value.AddMember("children", children, allocator);
  }

  return value;
}

rapidjson::Value ElementDumpHelper::RadonNodeDumpToJSON(
    rapidjson::Document& doc, RadonNode* node) {
  rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
  rapidjson::Value value;
  value.SetObject();

  if (node->element()) {
    value.AddMember("Impl Id", node->element()->impl_id(), allocator);
  }
  value.AddMember("Type", DumpRadonNodeTypeStrings[node->NodeType() + 1],
                  allocator);
  value.AddMember("Tag", node->tag_name_.str(), allocator);
  value.AddMember(
      "Attribute",
      DumpAttributeToJSON(
          doc, node->attribute_holder().get(),
          [&node, &allocator](rapidjson::Value& target) {
            if (node->cached_styles_.size() > 0) {
              std::map<CSSPropertyID, CSSValue> ordered_cached_styles_map(
                  node->cached_styles_.begin(), node->cached_styles_.end());
              rapidjson::Value cached_styles_value;
              cached_styles_value.SetObject();
              for (auto it = ordered_cached_styles_map.begin();
                   it != ordered_cached_styles_map.end(); ++it) {
                rapidjson::Value key(
                    CSSProperty::GetPropertyName(it->first).str(), allocator);
                rapidjson::Value val(
                    tasm::CSSDecoder::CSSValueToString(it->first, it->second),
                    allocator);
                cached_styles_value.AddMember(key, val, allocator);
              }
              target.AddMember("Cached Styles", cached_styles_value, allocator);
            }
          }),
      allocator);
  if (!node->radon_component_->name().empty()) {
    value.AddMember("Radon Component", node->radon_component_->name().str(),
                    allocator);
  }

  if (node->IsRadonComponent()) {
    auto* component = static_cast<RadonComponent*>(node);
    if (component) {
      std::string dsl = lynx::tasm::GetDSLName(component->GetDSL());
      value.AddMember("Component DSL", dsl, allocator);
      value.AddMember("Component Info Map",
                      DumpComponentInfoMap(doc, component), allocator);
    }
  }

  auto radon_children_size =
      static_cast<uint32_t>(node->radon_children_.size());
  value.AddMember("child count", radon_children_size, allocator);

  if (radon_children_size > 0) {
    rapidjson::Value children;
    children.SetArray();
    for (auto&& child : node->radon_children_) {
      children.GetArray().PushBack(RadonBaseDumpToJSON(doc, child.get()),
                                   allocator);
    }
    value.AddMember("children", children, allocator);
  }

  return value;
}

rapidjson::Value ElementDumpHelper::DumpAttributeToJSON(
    rapidjson::Document& doc, AttributeHolder* holder,
    std::function<void(rapidjson::Value&)> custom_dump) {
  rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
  rapidjson::Value value;
  value.SetObject();

  value.AddMember("IdSelector", holder->id_selector_.str(), allocator);

  if (holder->classes_.size() > 0) {
    rapidjson::Value class_array;
    class_array.SetArray();
    rapidjson::Value value_str(rapidjson::kStringType);
    for (const auto& clazz : holder->classes_) {
      auto size = static_cast<uint32_t>(clazz.str().size());
      value_str.SetString(clazz.str().c_str(), size, allocator);
      class_array.GetArray().PushBack(value_str, allocator);
    }
    value.AddMember("Classes", class_array, allocator);
  }

  if (holder->inline_styles_.size() > 0) {
    std::map<CSSPropertyID, CSSValue> ordered_inline_styles_map(
        holder->inline_styles_.begin(), holder->inline_styles_.end());
    rapidjson::Value inline_styles_value;
    inline_styles_value.SetObject();
    for (auto it = ordered_inline_styles_map.begin();
         it != ordered_inline_styles_map.end(); ++it) {
      rapidjson::Value key(CSSProperty::GetPropertyName(it->first).str(),
                           allocator);
      rapidjson::Value val(
          tasm::CSSDecoder::CSSValueToString(it->first, it->second), allocator);
      inline_styles_value.AddMember(key, val, allocator);
    }
    value.AddMember("Inline Styles", inline_styles_value, allocator);
  } else if (holder->radon_node_ptr() &&
             holder->radon_node_ptr()->raw_inline_styles().size() > 0) {
    StyleMap new_styles;
    auto& configs = holder->radon_node_ptr()
                        ->page_proxy_->element_manager()
                        ->GetCSSParserConfigs();
    for (const auto& style : holder->radon_node_ptr()->raw_inline_styles()) {
      UnitHandler::Process(style.first, style.second, new_styles, configs);
    }

    std::map<CSSPropertyID, CSSValue> ordered_inline_styles_map(
        new_styles.begin(), new_styles.end());
    rapidjson::Value inline_styles_value;
    inline_styles_value.SetObject();
    for (auto it = ordered_inline_styles_map.begin();
         it != ordered_inline_styles_map.end(); ++it) {
      rapidjson::Value key(CSSProperty::GetPropertyName(it->first).str(),
                           allocator);
      rapidjson::Value val(
          tasm::CSSDecoder::CSSValueToString(it->first, it->second), allocator);
      inline_styles_value.AddMember(key, val, allocator);
    }
    value.AddMember("Inline Styles", inline_styles_value, allocator);
  }

  // custom_dump is placed here for unittest conformance with previous dumped
  // data.
  if (custom_dump) {
    custom_dump(value);
  }

  if (holder->attributes_.size() > 0) {
    auto comp = [](const base::String& lhs, const base::String& rhs) {
      return lhs.str() < rhs.str();
    };
    std::map<base::String, lepus::Value, decltype(comp)> ordered_attributes_map(
        holder->attributes_.begin(), holder->attributes_.end(), comp);
    rapidjson::Value attributes_value;
    attributes_value.SetObject();
    for (auto it = ordered_attributes_map.begin();
         it != ordered_attributes_map.end(); ++it) {
      rapidjson::Value key((it->first).str(), allocator);
      lepus::Value lepus_val = it->second;
      if (lepus_val.IsString()) {
        rapidjson::Value val(lepus_val.StdString(), allocator);
        attributes_value.AddMember(key, val, allocator);
      } else if (lepus_val.IsNumber()) {
        attributes_value.AddMember(key, lepus_val.Number(), allocator);
      }
    }
    value.AddMember("Attributes", attributes_value, allocator);
  }

  if (holder->data_set_.size() > 0) {
    auto comp = [](const base::String& lhs, const base::String& rhs) {
      return lhs.str() < rhs.str();
    };
    std::map<base::String, lepus::Value, decltype(comp)> ordered_data_set(
        holder->data_set_.begin(), holder->data_set_.end(), comp);
    rapidjson::Value data_set_value;
    data_set_value.SetObject();
    for (auto it = ordered_data_set.begin(); it != ordered_data_set.end();
         ++it) {
      rapidjson::Value key((it->first).str(), allocator);
      lepus::Value lepus_val = it->second;
      if (lepus_val.IsString()) {
        rapidjson::Value val(lepus_val.StdString(), allocator);
        data_set_value.AddMember(key, val, allocator);
      } else if (lepus_val.IsNumber()) {
        data_set_value.AddMember(key, lepus_val.Number(), allocator);
      }
    }
    value.AddMember("Data Set", data_set_value, allocator);
  }

  if (holder->static_events_.size() > 0) {
    auto comp = [](const base::String& lhs, const base::String& rhs) {
      return lhs.str() < rhs.str();
    };
    std::set<base::String, decltype(comp)> keys(comp);
    for (auto it = holder->static_events_.begin();
         it != holder->static_events_.end(); ++it) {
      keys.insert(it->first);
    }
    rapidjson::Value static_events_value;
    static_events_value.SetObject();
    for (auto it = keys.begin(); it != keys.end(); ++it) {
      EventHandler* handler = holder->static_events_[*it].get();
      rapidjson::Value event_value;
      event_value.SetObject();
      event_value.AddMember("name", handler->name().str(), allocator);
      event_value.AddMember("type", handler->type().str(), allocator);
      event_value.AddMember("function", handler->function().str(), allocator);
      rapidjson::Value key((*it).str(), allocator);
      rapidjson::Value val(event_value, allocator);
      static_events_value.AddMember(key, val, allocator);
    }
    value.AddMember("Static Events", static_events_value, allocator);
  }

  return value;
}

void ElementDumpHelper::DumpAttributeToLepusValue(
    fml::RefPtr<lepus::Dictionary>& props, AttributeHolder* holder) {
  if (!holder->id_selector_.str().empty()) {
    props->SetValue("id", holder->id_selector_);
  }

  if (holder->inline_styles_.size() > 0) {
    std::map<CSSPropertyID, CSSValue> ordered_inline_styles_map(
        holder->inline_styles_.begin(), holder->inline_styles_.end());

    auto inline_styles = lepus::Dictionary::Create();
    for (auto& it : ordered_inline_styles_map) {
      inline_styles->SetValue(
          CSSProperty::GetPropertyName(it.first),
          tasm::CSSDecoder::CSSValueToString(it.first, it.second));
    }

    props->SetValue("style", inline_styles);
  }

  if (holder->classes_.size() > 0) {
    std::ostringstream cls;
    for (const auto& clazz : holder->classes_) {
      cls << clazz.str() << " ";
    }

    props->SetValue("class", cls.str());
  }

  if (holder->attributes_.size() > 0) {
    auto comp = [](const base::String& lhs, const base::String& rhs) {
      return lhs.str() < rhs.str();
    };
    std::map<base::String, lepus::Value, decltype(comp)> ordered_attributes_map(
        holder->attributes_.begin(), holder->attributes_.end(), comp);

    for (auto& it : ordered_attributes_map) {
      props->SetValue(it.first.str(), it.second);
    }
  }

  if (holder->data_set_.size() > 0) {
    auto comp = [](const base::String& lhs, const base::String& rhs) {
      return lhs.str() < rhs.str();
    };
    std::map<base::String, lepus::Value, decltype(comp)> ordered_data_set(
        holder->data_set_.begin(), holder->data_set_.end(), comp);

    for (auto& it : ordered_data_set) {
      props->SetValue("data-" + it.first.str(), it.second);
    }
  }

  if (holder->static_events_.size() > 0) {
    auto comp = [](const base::String& lhs, const base::String& rhs) {
      return lhs.str() < rhs.str();
    };
    std::set<base::String, decltype(comp)> keys(comp);
    for (auto& static_event : holder->static_events_) {
      keys.insert(static_event.first);
    }

    for (const auto& key : keys) {
      EventHandler* handler = holder->static_events_[key].get();

      if (handler->IsBindEvent()) {
        props->SetValue("bind" + handler->name().str(),
                        handler->function().str());
      } else if (handler->IsCatchEvent()) {
        props->SetValue("catch" + handler->name().str(),
                        handler->function().str());
      } else if (handler->IsCaptureBindEvent()) {
        props->SetValue("capture-bind" + handler->name().str(),
                        handler->function().str());
      } else if (handler->IsCaptureCatchEvent()) {
        props->SetValue("capture-catch" + handler->name().str(),
                        handler->function().str());
      } else if (handler->IsGlobalBindEvent()) {
        props->SetValue("global-bind" + handler->name().str(),
                        handler->function().str());
      }
    }
  }
}

void ElementDumpHelper::DumpAttributeToMarkup(std::ostringstream& ss,
                                              AttributeHolder* holder) {
  if (!holder->id_selector_.str().empty()) {
    ss << " id=\"" << holder->id_selector_.str() << "\"";
  }

  if (holder->inline_styles_.size() > 0) {
    std::map<CSSPropertyID, CSSValue> ordered_inline_styles_map(
        holder->inline_styles_.begin(), holder->inline_styles_.end());

    ss << " style=\"";
    for (auto& it : ordered_inline_styles_map) {
      ss << CSSProperty::GetPropertyName(it.first).str() << ": "
         << tasm::CSSDecoder::CSSValueToString(it.first, it.second) << "; ";
    }
    ss << "\"";
  }

  if (holder->classes_.size() > 0) {
    ss << " class=\"";
    for (const auto& clazz : holder->classes_) {
      ss << clazz.str() << " ";
    }
    ss << "\"";
  }

  if (holder->attributes_.size() > 0) {
    auto comp = [](const base::String& lhs, const base::String& rhs) {
      return lhs.str() < rhs.str();
    };
    std::map<base::String, lepus::Value, decltype(comp)> ordered_attributes_map(
        holder->attributes_.begin(), holder->attributes_.end(), comp);

    for (auto& it : ordered_attributes_map) {
      ss << " " << it.first.str();
      lepus::Value lepus_val = it.second;
      if (lepus_val.IsString()) {
        const auto& lepus_val_str = lepus_val.StdString();
        // check if lepus_val_str contains "
        if (lepus_val_str.find("\"") != std::string::npos) {
          // Allow syntax like <svg content={"<path d=\"...\"></path>"}></svg>
          auto s = std::regex_replace(lepus_val_str, std::regex("\""), "\\\"");
          ss << "={\"" << s << "\"}";
        } else {
          ss << "=\"" << lepus_val_str << "\"";
        }
      } else if (lepus_val.IsNumber()) {
        ss << "=\"" << lepus_val.Number() << "\"";
      }
    }
  }

  if (holder->data_set_.size() > 0) {
    auto comp = [](const base::String& lhs, const base::String& rhs) {
      return lhs.str() < rhs.str();
    };
    std::map<base::String, lepus::Value, decltype(comp)> ordered_data_set(
        holder->data_set_.begin(), holder->data_set_.end(), comp);

    for (auto& it : ordered_data_set) {
      ss << " data-" << it.first.str() << "=\"" << it.second.StdString()
         << "\"";
    }
  }

  if (holder->static_events_.size() > 0) {
    auto comp = [](const base::String& lhs, const base::String& rhs) {
      return lhs.str() < rhs.str();
    };
    std::set<base::String, decltype(comp)> keys(comp);
    for (auto& static_event : holder->static_events_) {
      keys.insert(static_event.first);
    }

    for (const auto& key : keys) {
      EventHandler* handler = holder->static_events_[key].get();

      if (handler->IsBindEvent()) {
        ss << " bind" << handler->name().str() << "=\""
           << handler->function().str() << "\"";
      } else if (handler->IsCatchEvent()) {
        ss << " catch" << handler->name().str() << "=\""
           << handler->function().str() << "\"";
      } else if (handler->IsCaptureBindEvent()) {
        ss << " capture-bind" << handler->name().str() << "=\""
           << handler->function().str() << "\"";
      } else if (handler->IsCaptureCatchEvent()) {
        ss << " capture-catch" << handler->name().str() << "=\""
           << handler->function().str() << "\"";
      }
    }
  }
}

rapidjson::Value ElementDumpHelper::DumpFiberElementToJSON(
    rapidjson::Document& doc, FiberElement* element) {
  rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
  rapidjson::Value value;
  value.SetObject();
  value.AddMember("Id", element->impl_id(), allocator);
  value.AddMember("Tag", element->GetTag().str(), allocator);
  value.AddMember("Attributes", DumpAttributeToJSON(doc, element->data_model()),
                  allocator);

  if (element->current_raw_inline_styles_.size() > 0) {
    std::map<CSSPropertyID, CSSValue> ordered_inline_styles_map(
        element->current_raw_inline_styles_.begin(),
        element->current_raw_inline_styles_.end());
    rapidjson::Value inline_styles_value;
    inline_styles_value.SetObject();
    for (auto& it : ordered_inline_styles_map) {
      rapidjson::Value key(CSSProperty::GetPropertyName(it.first).str(),
                           allocator);
      rapidjson::Value val(
          tasm::CSSDecoder::CSSValueToString(it.first, it.second), allocator);
      inline_styles_value.AddMember(key, val, allocator);
    }
    value.AddMember("Inline Styles", inline_styles_value, allocator);
  }

  if (element->parsed_styles_map_.size() > 0) {
    std::map<CSSPropertyID, CSSValue> ordered_parsed_styles_map(
        element->parsed_styles_map_.begin(), element->parsed_styles_map_.end());
    rapidjson::Value parsed_styles_value;
    parsed_styles_value.SetObject();
    for (auto& it : ordered_parsed_styles_map) {
      rapidjson::Value key(CSSProperty::GetPropertyName(it.first).str(),
                           allocator);
      rapidjson::Value val(
          tasm::CSSDecoder::CSSValueToString(it.first, it.second), allocator);
      parsed_styles_value.AddMember(key, val, allocator);
    }
    value.AddMember("Parsed Styles", parsed_styles_value, allocator);
  }

  value.AddMember("Parent Component Id", element->ParentComponentIdString(),
                  allocator);

  if (element->is_component()) {
    auto component = static_cast<ComponentElement*>(element);
    value.AddMember("Component Id", component->component_id().str(), allocator);
    value.AddMember("Component Path", component->component_path().str(),
                    allocator);
    value.AddMember("Component Name", component->component_name().str(),
                    allocator);
    value.AddMember("Component Entry", component->component_entry().str(),
                    allocator);
  }

  auto children_size = static_cast<uint32_t>(element->children().size());
  value.AddMember("Child Count", children_size, allocator);

  if (children_size > 0) {
    rapidjson::Value children_json;
    children_json.SetArray();
    for (auto&& child : element->children()) {
      children_json.GetArray().PushBack(
          DumpFiberElementToJSON(doc, child.get()), allocator);
    }
    value.AddMember("Children", children_json, allocator);
  }

  return value;
}

}  // namespace tasm
}  // namespace lynx
