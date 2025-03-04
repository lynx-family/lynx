// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/renderer/css/ng/invalidation/invalidation_set.h"

#include <memory>
#include <utility>

#include "core/renderer/dom/attribute_holder.h"

namespace lynx {
namespace css {

InvalidationSet::InvalidationSet(InvalidationType type)
    : type_(static_cast<unsigned>(type)),
      invalidates_self_(false),
      is_alive_(true) {}

bool InvalidationSet::InvalidatesElement(
    const tasm::AttributeHolder& element) const {
  if (WholeSubtreeInvalid()) return true;

  if (HasTagNames() && HasTagName(element.tag().str())) {
    return true;
  }

  if (element.HasID() && HasIds() && HasId(element.idSelector().str())) {
    return true;
  }

  if (element.HasClass() && HasClasses()) {
    if (FindAnyClass(element)) {
      return true;
    }
  }

  return false;
}

void InvalidationSet::Combine(const InvalidationSet& other) {
  DCHECK(is_alive_);
  DCHECK(other.is_alive_);
  DCHECK_EQ(GetType(), other.GetType());

  if (IsSelfInvalidationSet()) {
    // We should never modify the SelfInvalidationSet singleton. When
    // aggregating the contents from another invalidation set into an
    // invalidation set which only invalidates self, we instantiate a new
    // DescendantInvalidation set before calling Combine(). We still may end up
    // here if we try to combine two references to the singleton set.
    DCHECK(other.IsSelfInvalidationSet());
    return;
  }

  DCHECK(&other != this);

  if (other.InvalidatesSelf()) {
    SetInvalidatesSelf();
    if (other.IsSelfInvalidationSet()) {
      return;
    }
  }

  // No longer bother combining data structures, since the whole subtree is
  // deemed invalid.
  if (WholeSubtreeInvalid()) {
    return;
  }

  if (other.WholeSubtreeInvalid()) {
    SetWholeSubtreeInvalid();
    return;
  }

  for (const auto& class_name : other.Classes()) {
    AddClass(class_name);
  }

  for (const auto& id : other.Ids()) {
    AddId(id);
  }

  for (const auto& tag_name : other.TagNames()) {
    AddTagName(tag_name);
  }
}

bool InvalidationSet::HasEmptyBackings() const {
  return classes_.IsEmpty(backing_flags_) && ids_.IsEmpty(backing_flags_) &&
         tag_names_.IsEmpty(backing_flags_);
}

const std::string* InvalidationSet::FindAnyClass(
    const tasm::AttributeHolder& element) const {
  const auto& class_names = element.classes();
  size_t size = class_names.size();
  if (std::string* string_impl = classes_.GetStringImpl(backing_flags_)) {
    for (size_t i = 0; i < size; ++i) {
      if (*string_impl == class_names[i].str()) {
        return string_impl;
      }
    }
  }
  if (const std::unordered_set<std::string>* set =
          classes_.GetHashSet(backing_flags_)) {
    for (size_t i = 0; i < size; ++i) {
      auto it = set->find(class_names[i].str());
      if (it != set->end()) {
        return &*it;
      }
    }
  }
  return nullptr;
}

void InvalidationSet::AddClass(const std::string& class_name) {
  if (WholeSubtreeInvalid()) return;
  DCHECK(!class_name.empty());
  classes_.Add(backing_flags_, class_name);
}

void InvalidationSet::AddId(const std::string& id) {
  if (WholeSubtreeInvalid()) return;
  DCHECK(!id.empty());
  ids_.Add(backing_flags_, id);
}

void InvalidationSet::AddTagName(const std::string& tag_name) {
  if (WholeSubtreeInvalid()) return;
  DCHECK(!tag_name.empty());
  tag_names_.Add(backing_flags_, tag_name);
}

void InvalidationSet::SetWholeSubtreeInvalid() {
  if (whole_subtree_invalid_) {
    return;
  }
  whole_subtree_invalid_ = true;
  ClearAllBackings();
}

namespace {
DescendantInvalidationSet* CreateSelfInvalidationSet() {
  auto* new_set = new DescendantInvalidationSet();
  new_set->SetInvalidatesSelf();
  return new_set;
}
}  // namespace

InvalidationSet* InvalidationSet::SelfInvalidationSet() {
  static InvalidationSet* singleton_ = CreateSelfInvalidationSet();
  return singleton_;
}

}  // namespace css
}  // namespace lynx
