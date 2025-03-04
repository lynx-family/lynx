// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_NODE_SELECT_OPTIONS_H_
#define CORE_RENDERER_DOM_VDOM_RADON_NODE_SELECT_OPTIONS_H_

#include <string>

namespace lynx {
namespace tasm {

struct NodeSelectOptions {
  enum class IdentifierType {
    CSS_SELECTOR = 0,
    REF_ID = 1,
    ELEMENT_ID = 2,
  };

  NodeSelectOptions(const IdentifierType identifier_type,
                    const std::string &identifier)
      : identifier_type(identifier_type), identifier(identifier) {}

  NodeSelectOptions(const NodeSelectOptions &options) = default;
  NodeSelectOptions(NodeSelectOptions &&options) = default;

  NodeSelectOptions &operator=(const NodeSelectOptions &options) = default;
  NodeSelectOptions &operator=(NodeSelectOptions &&options) = default;

  const std::string &NodeIdentifierMessage() const { return identifier; }

  std::string ToString() const {
    std::string str = "{ type: ";
    switch (identifier_type) {
      case IdentifierType::CSS_SELECTOR:
        str += "CSS_SELECTOR";
        break;
      case IdentifierType::REF_ID:
        str += "REF_ID";
        break;
      case IdentifierType::ELEMENT_ID:
        str += "ELEMENT_ID";
        break;
    }
    str.append(", identifier: ")
        .append(identifier)
        .append(", first_only: ")
        .append(std::to_string(first_only))
        .append(", only_current_component: ")
        .append(std::to_string(only_current_component))
        .append(", component_only: ")
        .append(std::to_string(component_only))
        .append("}");
    return str;
  }

  IdentifierType identifier_type;
  std::string identifier;
  bool first_only = true;
  bool only_current_component = true;
  bool component_only = false;
};

struct NodeSelectRoot {
  enum class RootType {
    COMPONENT_ID = 0,
    // NODE_UNIQUE_ID is actually element_id
    NODE_UNIQUE_ID = 1,
  } root_type;
  std::string component_id;
  int node_unique_id;

  static NodeSelectRoot ByComponentId(const std::string &component_id) {
    return {RootType::COMPONENT_ID, component_id, 0};
  }

  static NodeSelectRoot ByUniqueId(int unique_id) {
    return {RootType::NODE_UNIQUE_ID, "", unique_id};
  }

  std::string ToPrettyString() const {
    std::string str = "{ type: ";
    switch (root_type) {
      case RootType::COMPONENT_ID:
        str.append("COMPONENT_ID, component_id: ").append(component_id);
        break;
      case RootType::NODE_UNIQUE_ID:
        str.append("NODE_UNIQUE_ID, node_unique_id: ")
            .append(std::to_string(node_unique_id));
        break;
    }
    str.append("}");
    return str;
  }
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_NODE_SELECT_OPTIONS_H_
