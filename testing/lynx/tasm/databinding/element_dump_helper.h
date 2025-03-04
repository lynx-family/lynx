// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef TESTING_LYNX_TASM_DATABINDING_ELEMENT_DUMP_HELPER_H_
#define TESTING_LYNX_TASM_DATABINDING_ELEMENT_DUMP_HELPER_H_

#include <sstream>

#define private public
#define protected public

#include <functional>
#include <string>

#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_base.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/page_proxy.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/prettywriter.h"
#include "third_party/rapidjson/stringbuffer.h"

namespace lynx {
namespace tasm {

// TODO(songshourui.null): rename ElementDumpHelper to TasmDumpHelper
class ElementDumpHelper {
 public:
  static std::string DumpElement(Element* element);
  static rapidjson::Value DumpToJSON(Element* element,
                                     rapidjson::Document& doc);
  static lepus::Value DumpToSnapshot(Element* element,
                                     bool skip_children = false);
  static void DumpToMarkup(Element* element, std::ostringstream& ss,
                           std::string indent = "", bool skip_children = false);

 private:
  static std::string DumpTree(PageProxy* proxy);
  static rapidjson::Value DumpComponentInfoMap(rapidjson::Document& doc,
                                               RadonComponent* component);
  static rapidjson::Value RadonBaseDumpToJSON(rapidjson::Document& doc,
                                              RadonBase* base);
  static rapidjson::Value RadonNodeDumpToJSON(rapidjson::Document& doc,
                                              RadonNode* node);
  static rapidjson::Value DumpAttributeToJSON(
      rapidjson::Document& doc, AttributeHolder* holder,
      std::function<void(rapidjson::Value& target)> custom_dump = nullptr);
  static rapidjson::Value DumpFiberElementToJSON(rapidjson::Document& doc,
                                                 FiberElement* element);

  static void DumpAttributeToLepusValue(fml::RefPtr<lepus::Dictionary>& props,
                                        AttributeHolder* holder);
  static void DumpAttributeToMarkup(std::ostringstream& stream,
                                    AttributeHolder* holder);
};

}  // namespace tasm
}  // namespace lynx

#endif  // TESTING_LYNX_TASM_DATABINDING_ELEMENT_DUMP_HELPER_H_
