// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_ABSTRACT_NODE_H_
#define CLAY_UI_RENDERING_ABSTRACT_NODE_H_

namespace clay {

// An abstract node in a tree, which will be used as the base class of
// RenderObject and RenderLayer.
class AbstractNode {
 public:
  AbstractNode() = default;
  virtual ~AbstractNode() = default;

  unsigned Depth() const { return depth_; }

  // Mark the given node as being a child of this node.
  void AdoptChild(AbstractNode* child);

  // Disconnect the given node from this node.
  void DropChild(AbstractNode* child) const;

  // Adjust the [depth] of this node's children, if any.
  virtual void RedepthChildren() = 0;

  AbstractNode* Parent() { return parent_; }
  const AbstractNode* Parent() const { return parent_; }

 protected:
  // Adjust the [depth] of the given [child] to be greater than this node's own
  // [depth].
  // Only call this method from overrides of [RedepthChildren].
  void RedepthChild(AbstractNode* child) const;

  void SetDepth(unsigned depth) { depth_ = depth; }
  void SetParent(AbstractNode* parent) { parent_ = parent; }

  // TRUE means the check passed, there is no a cycle.
  bool CheckCycle(const AbstractNode* child) const;

 private:
  // The depth of this node in the tree.
  unsigned depth_ = 0;
  // The parent of this node in the tree.
  AbstractNode* parent_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_ABSTRACT_NODE_H_
