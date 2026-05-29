// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ELEMENT_TREE_SERIALIZER_H_
#define CORE_RENDERER_DOM_ELEMENT_TREE_SERIALIZER_H_

#include <string>

#include "base/include/value/base_value.h"

namespace lynx {
namespace tasm {

class Element;

class ElementTreeSerializer {
 public:
  ElementTreeSerializer() = delete;
  ~ElementTreeSerializer() = delete;

  static lepus::Value ToLepusValue(Element* root);
  static std::string ToJSONString(Element* root);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ELEMENT_TREE_SERIALIZER_H_
