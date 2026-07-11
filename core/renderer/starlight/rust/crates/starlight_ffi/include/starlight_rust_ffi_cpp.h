// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_RUST_CRATES_STARLIGHT_FFI_INCLUDE_STARLIGHT_RUST_FFI_CPP_H_
#define CORE_RENDERER_STARLIGHT_RUST_CRATES_STARLIGHT_FFI_INCLUDE_STARLIGHT_RUST_FFI_CPP_H_

#include "starlight_rust_ffi.h"

#ifdef __cplusplus

#include <type_traits>
#include <utility>

namespace lynx {
namespace starlight {
namespace rust_ffi {
namespace detail {

template <typename Tree, typename = void>
struct HasChildCountMethod : std::false_type {};

template <typename Tree>
struct HasChildCountMethod<
    Tree, std::void_t<decltype(std::declval<Tree&>().ChildCount(
              std::declval<SLRustNodeId>()))>>
    : std::is_convertible<decltype(std::declval<Tree&>().ChildCount(
                              std::declval<SLRustNodeId>())),
                          size_t> {};

template <typename Tree, typename = void>
struct HasChildAtMethod : std::false_type {};

template <typename Tree>
struct HasChildAtMethod<
    Tree, std::void_t<decltype(std::declval<Tree&>().ChildAt(
              std::declval<SLRustNodeId>(), std::declval<size_t>()))>>
    : std::is_convertible<decltype(std::declval<Tree&>().ChildAt(
                              std::declval<SLRustNodeId>(),
                              std::declval<size_t>())),
                          SLRustNodeId> {};

template <typename Tree, typename = void>
struct HasStyleMethod : std::false_type {};

template <typename Tree>
struct HasStyleMethod<
    Tree, std::void_t<decltype(std::declval<Tree&>().Style(
              std::declval<SLRustNodeId>(), std::declval<SLRustStyle*>()))>>
    : std::is_convertible<decltype(std::declval<Tree&>().Style(
                              std::declval<SLRustNodeId>(),
                              std::declval<SLRustStyle*>())),
                          bool> {};

template <typename Tree, typename = void>
struct HasSetLayoutMethod : std::false_type {};

template <typename Tree>
struct HasSetLayoutMethod<
    Tree, std::void_t<decltype(std::declval<Tree&>().SetLayout(
              std::declval<SLRustNodeId>(),
              std::declval<SLRustLayoutResult>()))>>
    : std::is_same<decltype(std::declval<Tree&>().SetLayout(
                       std::declval<SLRustNodeId>(),
                       std::declval<SLRustLayoutResult>())),
                   void> {};

template <typename Tree, typename = void>
struct HasHasMeasureMethod : std::false_type {};

template <typename Tree>
struct HasHasMeasureMethod<
    Tree, std::void_t<decltype(std::declval<Tree&>().HasMeasure(
              std::declval<SLRustNodeId>()))>>
    : std::is_convertible<decltype(std::declval<Tree&>().HasMeasure(
                              std::declval<SLRustNodeId>())),
                          bool> {};

template <typename Tree, typename = void>
struct HasMeasureMethod : std::false_type {};

template <typename Tree>
struct HasMeasureMethod<
    Tree, std::void_t<decltype(std::declval<Tree&>().Measure(
              std::declval<SLRustNodeId>(),
              std::declval<SLRustConstraints>(),
              std::declval<SLRustSize*>()))>>
    : std::is_convertible<decltype(std::declval<Tree&>().Measure(
                              std::declval<SLRustNodeId>(),
                              std::declval<SLRustConstraints>(),
                              std::declval<SLRustSize*>())),
                          bool> {};

template <typename Tree, typename = void>
struct HasBaselineMethod : std::false_type {};

template <typename Tree>
struct HasBaselineMethod<
    Tree, std::void_t<decltype(std::declval<Tree&>().Baseline(
              std::declval<SLRustNodeId>(), std::declval<SLRustSize>(),
              std::declval<float*>()))>>
    : std::is_convertible<decltype(std::declval<Tree&>().Baseline(
                              std::declval<SLRustNodeId>(),
                              std::declval<SLRustSize>(),
                              std::declval<float*>())),
                          bool> {};

template <typename Tree, typename = void>
struct HasSetLayoutWithConstraintsMethod : std::false_type {};

template <typename Tree>
struct HasSetLayoutWithConstraintsMethod<
    Tree, std::void_t<decltype(std::declval<Tree&>().SetLayoutWithConstraints(
              std::declval<SLRustNodeId>(),
              std::declval<SLRustConstraints>(),
              std::declval<SLRustLayoutResult>()))>>
    : std::is_same<decltype(std::declval<Tree&>().SetLayoutWithConstraints(
                       std::declval<SLRustNodeId>(),
                   std::declval<SLRustConstraints>(),
                   std::declval<SLRustLayoutResult>())),
                   void> {};

template <typename Tree, typename = void>
struct HasPhysicalPixelsPerLayoutUnitMethod : std::false_type {};

template <typename Tree>
struct HasPhysicalPixelsPerLayoutUnitMethod<
    Tree, std::void_t<decltype(std::declval<Tree&>().PhysicalPixelsPerLayoutUnit(
              std::declval<SLRustNodeId>(), std::declval<float*>()))>>
    : std::is_convertible<decltype(std::declval<Tree&>().PhysicalPixelsPerLayoutUnit(
                              std::declval<SLRustNodeId>(),
                              std::declval<float*>())),
                          bool> {};

template <typename Tree>
struct HasRequiredCallbacks
    : std::integral_constant<bool,
                             HasChildCountMethod<Tree>::value &&
                                 HasChildAtMethod<Tree>::value &&
                                 HasStyleMethod<Tree>::value &&
                                 HasSetLayoutMethod<Tree>::value> {};

template <typename Tree>
struct HasCompleteMeasureCallbacks
    : std::integral_constant<bool, HasHasMeasureMethod<Tree>::value &&
                                       HasMeasureMethod<Tree>::value> {};

}  // namespace detail

// Adapts an existing C++ tree object to the callback ABI expected by the Rust
// Starlight engine. Tree must provide:
//   size_t ChildCount(SLRustNodeId)
//   SLRustNodeId ChildAt(SLRustNodeId, size_t)
//   bool Style(SLRustNodeId, SLRustStyle*)
//   void SetLayout(SLRustNodeId, SLRustLayoutResult)
// Tree may provide:
//   void SetLayoutWithConstraints(SLRustNodeId, SLRustConstraints,
//                                 SLRustLayoutResult)
//   bool HasMeasure(SLRustNodeId)
//   bool Measure(SLRustNodeId, SLRustConstraints, SLRustSize*)
//   bool Baseline(SLRustNodeId, SLRustSize, float*)
//   bool PhysicalPixelsPerLayoutUnit(SLRustNodeId, float*)
template <typename Tree>
struct ExternalTreeCallbacks {
  static SLRustTreeCallbacks Make(Tree* tree) {
    static_assert(detail::HasRequiredCallbacks<Tree>::value,
                  "Rust Starlight external tree adapters require "
                  "ChildCount(SLRustNodeId), "
                  "ChildAt(SLRustNodeId, size_t), "
                  "Style(SLRustNodeId, SLRustStyle*), and "
                  "SetLayout(SLRustNodeId, SLRustLayoutResult)");
    static_assert(
        detail::HasHasMeasureMethod<Tree>::value ==
            detail::HasMeasureMethod<Tree>::value,
        "Rust Starlight external tree measure support requires both "
        "HasMeasure(SLRustNodeId) and "
        "Measure(SLRustNodeId, SLRustConstraints, SLRustSize*)");
    SLRustTreeCallbacks callbacks = {};
    callbacks.context = tree;
    callbacks.child_count = &ChildCount;
    callbacks.child_at = &ChildAt;
    callbacks.style = &Style;
    callbacks.set_layout = &SetLayout;
    callbacks.set_layout_with_constraints =
        detail::HasSetLayoutWithConstraintsMethod<Tree>::value
            ? &SetLayoutWithConstraints
            : nullptr;
    callbacks.has_measure = detail::HasCompleteMeasureCallbacks<Tree>::value
                                ? &HasMeasure
                                : nullptr;
    callbacks.measure = detail::HasCompleteMeasureCallbacks<Tree>::value
                            ? &Measure
                            : nullptr;
    callbacks.baseline =
        detail::HasBaselineMethod<Tree>::value ? &Baseline : nullptr;
    callbacks.physical_pixels_per_layout_unit =
        detail::HasPhysicalPixelsPerLayoutUnitMethod<Tree>::value
            ? &PhysicalPixelsPerLayoutUnit
            : nullptr;
    return callbacks;
  }

 private:
  static Tree* FromContext(void* context) {
    return static_cast<Tree*>(context);
  }

  static size_t ChildCount(void* context, SLRustNodeId node) {
    Tree* tree = FromContext(context);
    return tree == nullptr ? 0 : tree->ChildCount(node);
  }

  static SLRustNodeId ChildAt(void* context, SLRustNodeId node, size_t index) {
    Tree* tree = FromContext(context);
    return tree == nullptr ? 0 : tree->ChildAt(node, index);
  }

  static bool Style(void* context, SLRustNodeId node, SLRustStyle* out_style) {
    Tree* tree = FromContext(context);
    return tree != nullptr && tree->Style(node, out_style);
  }

  static void SetLayout(void* context, SLRustNodeId node,
                        SLRustLayoutResult layout) {
    Tree* tree = FromContext(context);
    if (tree != nullptr) {
      tree->SetLayout(node, layout);
    }
  }

  static void SetLayoutWithConstraints(void* context, SLRustNodeId node,
                                       SLRustConstraints constraints,
                                       SLRustLayoutResult layout) {
    Tree* tree = FromContext(context);
    if (tree == nullptr) {
      return;
    }
    if constexpr (detail::HasSetLayoutWithConstraintsMethod<Tree>::value) {
      tree->SetLayoutWithConstraints(node, constraints, layout);
    }
  }

  static bool HasMeasure(void* context, SLRustNodeId node) {
    Tree* tree = FromContext(context);
    if (tree == nullptr) {
      return false;
    }
    if constexpr (detail::HasHasMeasureMethod<Tree>::value) {
      return tree->HasMeasure(node);
    }
    return false;
  }

  static bool Measure(void* context, SLRustNodeId node,
                      SLRustConstraints constraints, SLRustSize* out_size) {
    Tree* tree = FromContext(context);
    if (tree == nullptr) {
      return false;
    }
    if constexpr (detail::HasMeasureMethod<Tree>::value) {
      return tree->Measure(node, constraints, out_size);
    }
    return false;
  }

  static bool Baseline(void* context, SLRustNodeId node,
                       SLRustSize content_size, float* out_baseline) {
    Tree* tree = FromContext(context);
    if (tree == nullptr) {
      return false;
    }
    if constexpr (detail::HasBaselineMethod<Tree>::value) {
      return tree->Baseline(node, content_size, out_baseline);
    }
    return false;
  }

  static bool PhysicalPixelsPerLayoutUnit(void* context, SLRustNodeId node,
                                          float* out_value) {
    Tree* tree = FromContext(context);
    if (tree == nullptr) {
      return false;
    }
    if constexpr (detail::HasPhysicalPixelsPerLayoutUnitMethod<Tree>::value) {
      return tree->PhysicalPixelsPerLayoutUnit(node, out_value);
    }
    return false;
  }
};

template <typename Tree>
inline SLRustTreeCallbacks MakeTreeCallbacks(Tree* tree) {
  return ExternalTreeCallbacks<Tree>::Make(tree);
}

template <typename Tree>
inline SLRustStatus LayoutExternalChecked(Tree* tree, SLRustNodeId root,
                                          SLRustConstraints constraints,
                                          SLRustSize* out_size) {
  if (tree == nullptr) {
    return SLRustStatusNullPointer;
  }
  SLRustAbiInfo caller_abi = SLRustMakeCallerAbiInfo();
  SLRustTreeCallbacks callbacks = MakeTreeCallbacks(tree);
  return SLRustLayoutExternalChecked(&caller_abi, &callbacks, root,
                                     constraints, out_size);
}

template <typename Tree>
inline SLRustStatus LayoutExternalWithOwnerConstraintsChecked(
    Tree* tree, SLRustNodeId root, SLRustConstraints constraints,
    SLRustSize* out_size) {
  if (tree == nullptr) {
    return SLRustStatusNullPointer;
  }
  SLRustAbiInfo caller_abi = SLRustMakeCallerAbiInfo();
  SLRustTreeCallbacks callbacks = MakeTreeCallbacks(tree);
  return SLRustLayoutExternalWithOwnerConstraintsChecked(
      &caller_abi, &callbacks, root, constraints, out_size);
}

template <typename Tree>
inline SLRustStatus LayoutExternalWithOwnerConstraintsAndDirectionChecked(
    Tree* tree, SLRustNodeId root, SLRustConstraints constraints,
    int32_t owner_direction, SLRustSize* out_size) {
  if (tree == nullptr) {
    return SLRustStatusNullPointer;
  }
  SLRustAbiInfo caller_abi = SLRustMakeCallerAbiInfo();
  SLRustTreeCallbacks callbacks = MakeTreeCallbacks(tree);
  return SLRustLayoutExternalWithOwnerConstraintsAndDirectionChecked(
      &caller_abi, &callbacks, root, constraints, owner_direction, out_size);
}

template <typename Tree>
inline SLRustStatus LayoutExternalWithNodeConstraintsChecked(
    Tree* tree, SLRustNodeId root, SLRustConstraints constraints,
    SLRustSize* out_size) {
  if (tree == nullptr) {
    return SLRustStatusNullPointer;
  }
  SLRustAbiInfo caller_abi = SLRustMakeCallerAbiInfo();
  SLRustTreeCallbacks callbacks = MakeTreeCallbacks(tree);
  return SLRustLayoutExternalWithNodeConstraintsChecked(
      &caller_abi, &callbacks, root, constraints, out_size);
}

}  // namespace rust_ffi
}  // namespace starlight
}  // namespace lynx

#endif  // __cplusplus

#endif  // CORE_RENDERER_STARLIGHT_RUST_CRATES_STARLIGHT_FFI_INCLUDE_STARLIGHT_RUST_FFI_CPP_H_
