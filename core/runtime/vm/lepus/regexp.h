// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_REGEXP_H_
#define CORE_RUNTIME_VM_LEPUS_REGEXP_H_
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/value/base_string.h"
namespace lynx {
namespace lepus {
class Value;
class RegExp : public fml::RefCountedThreadSafeStorage {
 public:
  static fml::RefPtr<RegExp> Create() {
    return fml::AdoptRef<RegExp>(new RegExp());
  }

  static fml::RefPtr<RegExp> Create(const base::String& pattern) {
    return fml::AdoptRef<RegExp>(new RegExp(pattern, base::String()));
  }
  static fml::RefPtr<RegExp> Create(const base::String& pattern,
                                    const base::String& flags) {
    return fml::AdoptRef<RegExp>(new RegExp(pattern, flags));
  }

  RegExp(const RegExp& other)
      : pattern_(other.pattern_), flags_(other.flags_) {}

  const base::String& get_pattern() const { return pattern_; }
  const base::String& get_flags() const { return flags_; }

  void set_pattern(const base::String& pattern) { pattern_ = pattern; }
  void set_flags(const base::String& flags) { flags_ = flags; }

  void ReleaseSelf() const override { delete this; }

  ~RegExp() override = default;

  friend bool operator==(const RegExp& left, const RegExp& right) {
    return left.pattern_ == right.pattern_ && left.flags_ == right.flags_;
  }

  friend bool operator!=(const RegExp& left, const RegExp& right) {
    return !(left == right);
  }

 protected:
  RegExp() = default;
  RegExp(const base::String& pattern, const base::String& flags) {
    pattern_ = pattern;
    flags_ = flags;
  }

 private:
  base::String pattern_;
  base::String flags_;
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_REGEXP_H_
