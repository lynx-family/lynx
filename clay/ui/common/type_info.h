// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_TYPE_INFO_H_
#define CLAY_UI_COMMON_TYPE_INFO_H_

#include <type_traits>
#include <utility>

namespace clay {

namespace detail {

struct TypeChain {
  const TypeChain* parent;
};

// This helper class is used to check if a class is directly derived from
// `WithTypeInfo`.
template <typename T>
class TypeRegisterChecker {};

}  // namespace detail

using TypeId = const detail::TypeChain*;

// This class is used to implement dynamic type checking. The root type you want
// to check should inherit from `TypeIdentifiable<YourRootClass>`.
// Example:
//     class BaseView : public TypeIdentifiable<BaseView> {};
//     class View1 : public WithTypeInfo<View1, BaseView> {};
//     class View2 : public WithTypeInfo<View2, View1> {};
//     BaseView* view = new View1();
//     assert(view->Is<View1>());
//     assert(!view->Is<View2>());
template <typename RootType>
class TypeIdentifiable {
 public:
  TypeIdentifiable() : type_id_(StaticType()) {}

  bool IsOfType(TypeId type) const {
    auto t = GetTypeId();
    do {
      if (t == type) {
        return true;
      }
      t = t->parent;
    } while (t);
    return false;
  }

  template <typename T>
  bool Is() const {
    static_assert(
        std::is_base_of_v<detail::TypeRegisterChecker<T>, T>,
        "To use Is<T>(), T must be directly derived from WithTypeInfo<>");
    return IsOfType(T::StaticType());
  }

  static TypeId StaticType() { return &s_static_type_; }

  TypeId GetTypeId() const { return type_id_; }

 private:
  template <typename Derived, typename Base>
  friend class WithTypeInfo;

  inline static const detail::TypeChain s_static_type_ = {nullptr};

  TypeId type_id_;
};

// This class is used to implement dynamic type checking. For detail, see
// `TypeIdentifiable`.
template <typename Derived, typename Base>
class WithTypeInfo : public Base, public detail::TypeRegisterChecker<Derived> {
 public:
  template <typename... Args>
  explicit WithTypeInfo(Args&&... args) : Base(std::forward<Args>(args)...) {
    this->type_id_ = StaticType();
  }

  static TypeId StaticType() { return &s_static_type_; }

 private:
  // Using static member instead of static variable to avoid extra binary size.
  inline static const detail::TypeChain s_static_type_ = {Base::StaticType()};
};

}  // namespace clay

#endif  // CLAY_UI_COMMON_TYPE_INFO_H_
