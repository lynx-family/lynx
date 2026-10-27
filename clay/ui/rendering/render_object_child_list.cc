// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/render_object_child_list.h"

#include "clay/fml/logging.h"
#include "clay/ui/rendering/render_object.h"

namespace clay {

void RenderObjectChildList::DestroyLeftoverChildren() {
  while (FirstChild()) {
    FirstChild()->Destroy();
  }
}

int RenderObjectChildList::IndexOfChild(RenderObject* child) const {
  RenderObject* first = FirstChild();
  for (int index = 0; first != nullptr; index++) {
    if (first == child) {
      return index;
    }
    first = first->NextSibling();
  }
  return -1;
}

void RenderObjectChildList::RemoveChild(RenderObject* owner,
                                        RenderObject* old_child) {
  FML_DCHECK(old_child->Parent() == owner);

  if (old_child->PreviousSibling()) {
    old_child->PreviousSibling()->SetNextSibling(old_child->NextSibling());
  }
  if (old_child->NextSibling()) {
    old_child->NextSibling()->SetPreviousSibling(old_child->PreviousSibling());
  }

  if (FirstChild() == old_child) {
    SetFirstChild(old_child->NextSibling());
  }
  if (LastChild() == old_child) {
    SetLastChild(old_child->PreviousSibling());
  }

  old_child->SetPreviousSibling(nullptr);
  old_child->SetNextSibling(nullptr);

  if (owner) {
    owner->DirtyChildrenPaintingOrder();
  }
}

void RenderObjectChildList::InsertChild(RenderObject* owner,
                                        RenderObject* new_child,
                                        RenderObject* before_child) {
  while (before_child && before_child->Parent() &&
         before_child->Parent() != owner) {
    before_child = static_cast<RenderObject*>(before_child->Parent());
  }

  // This should never happen, but if it does prevent render tree corruption
  // where child->parent() ends up being owner but
  // child->NextSibling()->Parent() is not owner.
  if (before_child && before_child->Parent() != owner) {
    FML_UNREACHABLE();
    return;
  }

  if (FirstChild() == before_child) {
    SetFirstChild(new_child);
  }

  if (before_child) {
    RenderObject* previous_sibling = before_child->PreviousSibling();
    if (previous_sibling) {
      previous_sibling->SetNextSibling(new_child);
    }
    new_child->SetPreviousSibling(previous_sibling);
    new_child->SetNextSibling(before_child);
    before_child->SetPreviousSibling(new_child);
  } else {
    if (LastChild()) {
      LastChild()->SetNextSibling(new_child);
    }
    new_child->SetPreviousSibling(LastChild());
    SetLastChild(new_child);
  }

  if (owner) {
    owner->DirtyChildrenPaintingOrder();
  }
}

}  // namespace clay
