// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/abstract_node.h"

#include "clay/fml/logging.h"

namespace clay {

void AbstractNode::RedepthChild(AbstractNode* child) const {
  if (child->Depth() <= depth_) {
    child->SetDepth(depth_ + 1);
    child->RedepthChildren();
  }
}

void AbstractNode::AdoptChild(AbstractNode* child) {
  FML_DCHECK(child->Parent() == nullptr);
  FML_DCHECK(CheckCycle(child));

  child->SetParent(this);
  RedepthChild(child);
}

void AbstractNode::DropChild(AbstractNode* child) const {
  FML_DCHECK(child->Parent() == this);

  child->SetParent(nullptr);
}

bool AbstractNode::CheckCycle(const AbstractNode* child) const {
  const AbstractNode* node = this;
  while (node->Parent() != nullptr) {
    node = node->Parent();
  }

  FML_DCHECK(node != child);
  return node != child;
}

}  // namespace clay
