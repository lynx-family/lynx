// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_REGEXP_H_
#define CORE_RUNTIME_VM_LEPUS_REGEXP_H_

#include "base/include/value/base_string.h"
#include "base/include/value/ref_counted_class.h"
#include "base/include/value/ref_type.h"

namespace lynx {
namespace lepus {
class Value;
class RegExp : public lepus::RefCounted {
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

  RefType GetRefType() const override { return RefType::kRegExp; }

  void Print(std::ostream& output) override {
    output << "regexp" << std::endl;
    output << "pattern: " << get_pattern().str() << std::endl;
    output << "flags: " << get_flags().str() << std::endl;
  }

  bool Equals(const fml::RefPtr<RefCounted>& other) override {
    auto reg_exp = fml::static_ref_ptr_cast<RegExp>(other);
    return get_pattern() == reg_exp->get_pattern() &&
           get_flags() == reg_exp->get_flags();
  }

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

  friend class Value;

  void Reset() {
    pattern_ = base::String();
    flags_ = base::String();
  }

 private:
  base::String pattern_;
  base::String flags_;
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_REGEXP_H_
