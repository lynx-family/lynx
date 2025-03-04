// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/container_node.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace starlight {
ContainerNode::~ContainerNode() {
  if (parent_) {
    parent_->RemoveChild(this);
    parent_ = nullptr;
  }
  if (first_child_) {
    while (ContainerNode* child = static_cast<ContainerNode*>(FirstChild())) {
      RemoveChild(child);
    }
    first_child_ = nullptr;
    last_child_ = nullptr;
  }
}

Node* ContainerNode::Find(int index) {
  if (index == 0) {
    return first_child_;
  }
  if (index < 0) {
    return nullptr;
  }

  Node* node = first_child_;

  while (index-- && node != nullptr) node = node->next_;

  return node;
}

int ContainerNode::IndexOf(Node* node) {
  int index = 0;
  Node* current = first_child_;
  while (current != nullptr) {
    if (current == node) {
      return index;
    }
    index++;
    current = current->next_;
  }
  return -1;
}

void ContainerNode::AppendChild(ContainerNode* child) {
  InsertChildBefore(child, nullptr);
}

void ContainerNode::InsertChildBefore(ContainerNode* child,
                                      ContainerNode* node_reference) {
  if (node_reference != nullptr && node_reference->parent() != this) {
    NOTREACHED();
    return;
  }

  child->next_ = node_reference;

  Node* previous;
  if (node_reference == nullptr) {
    previous = last_child_;
    last_child_ = child;
  } else {
    previous = node_reference->previous_;
    node_reference->previous_ = child;
  }

  child->previous_ = previous;
  if (previous == nullptr) {
    first_child_ = child;
  } else {
    previous->next_ = child;
  }

  child->parent_ = this;
  ++child_count_;
}

void ContainerNode::RemoveChild(ContainerNode* child) {
  if (child == nullptr || child_count_ == 0) return;
  Node* pre = child->previous_;
  Node* next = child->next_;

  child->parent_ = nullptr;
  if (pre == nullptr && next == nullptr) {
    first_child_ = nullptr;
    last_child_ = nullptr;
  } else if (pre == nullptr) {
    next->previous_ = pre;
    first_child_ = next;
  } else if (next == nullptr) {
    pre->next_ = next;
    last_child_ = pre;
  } else {
    next->previous_ = pre;
    pre->next_ = next;
  }
  child->previous_ = nullptr;
  child->next_ = nullptr;
  child_count_--;
}

}  // namespace starlight
}  // namespace lynx
