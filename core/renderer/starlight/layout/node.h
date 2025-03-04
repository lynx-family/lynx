// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_NODE_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_NODE_H_

#include <stddef.h>

namespace lynx {
namespace starlight {
class Node {
 public:
  Node() : previous_(NULL), next_(NULL) {}
  virtual ~Node() {}
  inline Node* Next() { return next_; }
  inline Node* Previous() { return previous_; }
  friend class ContainerNode;

 private:
  Node* previous_;
  Node* next_;
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_NODE_H_
