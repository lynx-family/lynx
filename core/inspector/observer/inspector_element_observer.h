// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INSPECTOR_OBSERVER_INSPECTOR_ELEMENT_OBSERVER_H_
#define CORE_INSPECTOR_OBSERVER_INSPECTOR_ELEMENT_OBSERVER_H_

#include <cstdint>
#include <map>
#include <string>

#include "core/base/utils/any.h"
#include "core/renderer/dom/element.h"

namespace lynx {
namespace lepus {
class Value;
}  // namespace lepus

namespace tasm {

class Element;
enum CSSPropertyID : int32_t;

class InspectorElementObserver {
 public:
  virtual void OnDocumentUpdated() = 0;

  // OnElementNodeAdded handles notifications for new element nodes.
  // 1. It can notify each node individually as they are added,
  //    or notify the devtool once with the root of a newly added subtree,
  //    avoiding the need to notify every single node.
  // 2. When the page root node is notified,
  //    it sets element_root_ on the devtool side,
  //    which is used by the devtool to find element nodes by nodeId.
  virtual void OnElementNodeAdded(Element *ptr) = 0;
  virtual void OnElementNodeRemoved(Element *ptr) = 0;
  virtual void OnCharacterDataModified(Element *ptr) = 0;
  virtual void OnElementDataModelSet(Element *ptr) = 0;

  virtual void OnElementManagerWillDestroy() = 0;

  virtual void OnCSSStyleSheetAdded(Element *ptr) = 0;

  virtual void OnCSSMediaQueryResultChanged() = 0;

  virtual void OnComponentUselessUpdate(const std::string &component_name,
                                        const lepus::Value &properties) = 0;
  virtual void OnSetNativeProps(Element *ptr, const std::string &name,
                                const std::string &value, bool is_style) = 0;
  virtual void OnAddInlineStyle(int32_t backend_node_id,
                                CSSPropertyID property_id,
                                const lepus::Value &value) {}
  virtual void OnFiberFlushElementTree() {}
  virtual std::map<lynx::devtool::DevToolFunction,
                   std::function<void(const base::any &)>>
  GetDevToolFunction() = 0;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_INSPECTOR_OBSERVER_INSPECTOR_ELEMENT_OBSERVER_H_
