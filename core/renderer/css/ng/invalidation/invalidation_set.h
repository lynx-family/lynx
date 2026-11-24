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

#ifndef CORE_RENDERER_CSS_NG_INVALIDATION_INVALIDATION_SET_H_
#define CORE_RENDERER_CSS_NG_INVALIDATION_INVALIDATION_SET_H_

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/vector.h"

namespace lynx {
namespace tasm {
class AttributeHolder;
}
namespace css {

enum class InvalidationType {
  kInvalidateDescendants,
};

class InvalidationSet;
class DescendantInvalidationSet;

// Tracks data to determine which descendants in a DOM subtree need to have
// style recalculated.
//
// Some example invalidation sets:
//
// .z {}
//   For class z we will have a DescendantInvalidationSet with invalidatesSelf
//   (the element itself is invalidated).
//
// .y .z {}
//   For class y we will have a DescendantInvalidationSet containing class z.
//
// .v * {}
//   For class v we will have a DescendantInvalidationSet with
//   wholeSubtreeInvalid.
//
// Avoid virtual functions to minimize space consumption.
class InvalidationSet {
 public:
  struct Deleter {
    void operator()(InvalidationSet* set);
  };

  InvalidationSet(const InvalidationSet&) = delete;
  InvalidationSet& operator=(const InvalidationSet&) = delete;

  ~InvalidationSet() {
    DCHECK(is_alive_);
    is_alive_ = false;
    ClearAllBackings();
  }

  InvalidationType GetType() const {
    return static_cast<InvalidationType>(type_);
  }
  bool IsDescendantInvalidationSet() const {
    return GetType() == InvalidationType::kInvalidateDescendants;
  }

  bool InvalidatesElement(const tasm::AttributeHolder&) const;

  void AddClass(const std::string& class_name);
  void AddId(const std::string& id);
  void AddTagName(const std::string& tag_name);

  void SetWholeSubtreeInvalid();
  bool WholeSubtreeInvalid() const { return whole_subtree_invalid_; }
  void SetInvalidatesSelf() { invalidates_self_ = true; }
  bool InvalidatesSelf() const { return invalidates_self_; }

  bool IsEmpty() const { return HasEmptyBackings(); }

  bool IsAlive() const { return is_alive_; }

  void Combine(const InvalidationSet& other);

  // Returns a singleton DescendantInvalidationSet which only has
  // InvalidatesSelf() set and is otherwise empty. As this is a common
  // invalidation set for features only found in rightmost compounds,
  // sharing this singleton between such features saves a lot of memory on
  // sites with a big number of style rules.
  static InvalidationSet* SelfInvalidationSet();
  bool IsSelfInvalidationSet() const { return this == SelfInvalidationSet(); }

  enum class BackingType {
    kClasses,
    kIds,
    kTagNames,
    kAttributes
    // These values are used as bit-indices, and must be smaller than 8.
    // See Backing::GetMask.
  };

  union Backing;

  // Each BackingType has a corresponding bit in an instance of this class. A
  // set bit indicates that the Backing at that position is a
  // std::unordered_set. An unset bit indicates a std::string (which may be
  // nullptr).
  class BackingFlags {
   private:
    uint8_t bits_ = 0;
    friend union Backing;
  };

  // WARNING: Backings must be cleared manually in ~InvalidationSet, otherwise
  //          a std::string or std::unordered_set will leak.
  union Backing {
    using Flags = BackingFlags;
    static_assert(static_cast<size_t>(BackingType::kAttributes) <
                      sizeof(BackingFlags::bits_) * 8,
                  "Enough bits in BackingFlags");

    // Stores one string directly and upgrades to a set for multiple strings.
    void Add(Flags&, BackingType, const std::string&);
    // Clears the associated Backing. If the Backing is a std::string, it is
    // released. If the Backing is a std::unordered_set, it is deleted.
    void Clear(Flags&, BackingType);
    bool Contains(const Flags&, BackingType, const std::string&) const;
    bool IsEmpty(const Flags&, BackingType) const;
    size_t Size(const Flags&, BackingType) const;
    bool IsHashSet(const Flags& flags, BackingType type) const {
      return flags.bits_ & GetMask(type);
    }

    std::string* GetStringImpl(const Flags& flags, BackingType type) const {
      return IsHashSet(flags, type) ? nullptr : string_impl_;
    }
    const std::unordered_set<std::string>* GetHashSet(const Flags& flags,
                                                      BackingType type) const {
      return IsHashSet(flags, type) ? hash_set_ : nullptr;
    }

    // A simple forward iterator, which can either "iterate" over a single
    // std::string, or act as a wrapper for
    // std::unordered_set<std::string>::iterator.
    class Iterator {
     public:
      enum class Type { kString, kHashSet };

      explicit Iterator(std::string* string_impl)
          : type_(Type::kString),
            string_(string_impl == nullptr ? "" : *string_impl) {}
      explicit Iterator(std::unordered_set<std::string>::iterator iterator)
          : type_(Type::kHashSet), hash_set_iterator_(iterator) {}

      bool operator==(const Iterator& other) const {
        if (type_ != other.type_) return false;
        if (type_ == Type::kString) return string_ == other.string_;
        return hash_set_iterator_ == other.hash_set_iterator_;
      }
      bool operator!=(const Iterator& other) const { return !(*this == other); }
      void operator++() {
        if (type_ == Type::kString)
          string_ = "";
        else
          ++hash_set_iterator_;
      }

      const std::string& operator*() const {
        return type_ == Type::kString ? string_ : *hash_set_iterator_;
      }

     private:
      Type type_;
      // Used when type_ is kString.
      std::string string_;
      // Used when type_ is kHashSet.
      std::unordered_set<std::string>::iterator hash_set_iterator_;
    };

    class Range {
     public:
      Range(Iterator begin, Iterator end) : begin_(begin), end_(end) {}
      Iterator begin() const { return begin_; }
      Iterator end() const { return end_; }

     private:
      Iterator begin_;
      Iterator end_;
    };

    Range Items(const Flags& flags, BackingType type) const {
      Iterator begin = IsHashSet(flags, type) ? Iterator(hash_set_->begin())
                                              : Iterator(string_impl_);
      Iterator end = IsHashSet(flags, type) ? Iterator(hash_set_->end())
                                            : Iterator(nullptr);
      return Range(begin, end);
    }

   private:
    static uint8_t GetMask(BackingType type) {
      return 1u << static_cast<size_t>(type);
    }
    void SetIsString(Flags& flags, BackingType type) {
      flags.bits_ &= ~GetMask(type);
    }
    void SetIsHashSet(Flags& flags, BackingType type) {
      flags.bits_ |= GetMask(type);
    }

    std::string* string_impl_ = nullptr;
    std::unordered_set<std::string>* hash_set_;
  };

 protected:
  explicit InvalidationSet(InvalidationType);

 private:
  void ClearAllBackings() {
    classes_.Clear(backing_flags_, BackingType::kClasses);
    ids_.Clear(backing_flags_, BackingType::kIds);
    tag_names_.Clear(backing_flags_, BackingType::kTagNames);
  }
  bool HasEmptyBackings() const;

  bool HasClasses() const {
    return !classes_.IsEmpty(backing_flags_, BackingType::kClasses);
  }
  bool HasIds() const {
    return !ids_.IsEmpty(backing_flags_, BackingType::kIds);
  }
  bool HasTagNames() const {
    return !tag_names_.IsEmpty(backing_flags_, BackingType::kTagNames);
  }

  bool HasId(const std::string& string) const {
    return ids_.Contains(backing_flags_, BackingType::kIds, string);
  }

  bool HasTagName(const std::string& string) const {
    return tag_names_.Contains(backing_flags_, BackingType::kTagNames, string);
  }

  Backing::Range Classes() const {
    return classes_.Items(backing_flags_, BackingType::kClasses);
  }

  Backing::Range Ids() const {
    return ids_.Items(backing_flags_, BackingType::kIds);
  }

  Backing::Range TagNames() const {
    return tag_names_.Items(backing_flags_, BackingType::kTagNames);
  }

  const std::string* FindAnyClass(const tasm::AttributeHolder&) const;

  Backing classes_;
  Backing ids_;
  Backing tag_names_;

  bool whole_subtree_invalid_{false};
  BackingFlags backing_flags_;

  unsigned type_ : 2;

  // If true, the element itself is invalid.
  unsigned invalidates_self_ : 1;

  // If true, the instance is alive and can be used.
  unsigned is_alive_ : 1;
  friend class RuleInvalidationSetTest;
};

using InvalidationSetPtr =
    std::unique_ptr<InvalidationSet, InvalidationSet::Deleter>;
using DescendantInvalidationSetPtr =
    std::unique_ptr<DescendantInvalidationSet, InvalidationSet::Deleter>;

class DescendantInvalidationSet : public InvalidationSet {
 public:
  static DescendantInvalidationSetPtr Create() {
    return DescendantInvalidationSetPtr(new DescendantInvalidationSet());
  }

  DescendantInvalidationSet()
      : InvalidationSet(InvalidationType::kInvalidateDescendants) {}
};

using InvalidationSetVector = base::Vector<InvalidationSet*>;

struct InvalidationLists {
  InvalidationSetVector descendants;
};

inline void InvalidationSet::Deleter::operator()(InvalidationSet* set) {
  // Workaround Linux build issue
  if (set == InvalidationSet::SelfInvalidationSet()) {
    // Static variables cannot be deleted
    return;
  }
  if (set->IsDescendantInvalidationSet()) {
    delete static_cast<DescendantInvalidationSet*>(set);
  }
}

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_INVALIDATION_INVALIDATION_SET_H_
