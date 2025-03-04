// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_COMPONENT_ATTRIBUTES_H_
#define CORE_RENDERER_DOM_COMPONENT_ATTRIBUTES_H_

#include <set>
#include <string>

#include "base/include/no_destructor.h"

namespace lynx {
namespace tasm {

class ComponentAttributes {
 public:
  static const std::set<std::string>& GetAttrNames();
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_COMPONENT_ATTRIBUTES_H_
