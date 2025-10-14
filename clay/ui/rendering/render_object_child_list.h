// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDER_OBJECT_CHILD_LIST_H_
#define CLAY_UI_RENDERING_RENDER_OBJECT_CHILD_LIST_H_

namespace clay {

class RenderObject;

// Provides a child model for a render object subclass that has a doubly-linked
// list of children.
class RenderObjectChildList {
 public:
  RenderObjectChildList() : first_child_(nullptr), last_child_(nullptr) {}

  RenderObject* FirstChild() const { return first_child_; }
  RenderObject* LastChild() const { return last_child_; }

  void SetFirstChild(RenderObject* child) { first_child_ = child; }
  void SetLastChild(RenderObject* child) { last_child_ = child; }

  void DestroyLeftoverChildren();
  int IndexOfChild(RenderObject* child) const;

  void RemoveChild(RenderObject* owner, RenderObject*);
  void InsertChild(RenderObject* owner, RenderObject* new_child,
                   RenderObject* before_child);

  void AppendChild(RenderObject* owner, RenderObject* new_child) {
    InsertChild(owner, new_child, nullptr);
  }

 private:
  RenderObject* first_child_;
  RenderObject* last_child_;
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_RENDER_OBJECT_CHILD_LIST_H_
