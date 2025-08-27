// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_CONTAINER_NODE_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_CONTAINER_NODE_H_

#include "core/base/lynx_export.h"
#include "core/renderer/starlight/layout/node.h"

namespace lynx {
namespace starlight {
class ContainerNode : public Node {
 public:
  ContainerNode()
      : parent_(NULL), first_child_(NULL), last_child_(NULL), child_count_(0) {}
  virtual ~ContainerNode();
  /**
   *  Insert a layout node after a node.
   *  @param child the node will be inserted
   *  @param node before which child will be inserted
   */
  LYNX_EXPORT void InsertChildBefore(ContainerNode* child, ContainerNode* node);
  LYNX_EXPORT void AppendChild(ContainerNode* child);

  LYNX_EXPORT void RemoveChild(ContainerNode* child);

  Node* FirstChild() const { return first_child_; }
  Node* LastChild() const { return last_child_; }

  LYNX_EXPORT Node* Find(int n);
  LYNX_EXPORT int IndexOf(Node* node);

  LYNX_EXPORT int GetChildCount() { return child_count_; }

  LYNX_EXPORT ContainerNode* parent() { return parent_; }

 protected:
  ContainerNode* parent_;

 private:
  Node* first_child_;
  Node* last_child_;

  int child_count_;
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_CONTAINER_NODE_H_
