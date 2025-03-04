// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_PIPER_JS_UPDATE_DATA_TYPE_H_
#define CORE_RUNTIME_PIPER_JS_UPDATE_DATA_TYPE_H_

#include <cstdint>

#include "base/include/to_underlying.h"

namespace lynx::runtime {

struct UpdateDataType {
  UpdateDataType() : UpdateDataType(0) {}
  explicit UpdateDataType(std::uint32_t type) : type_(type) {}

  // copiable
  UpdateDataType(const UpdateDataType&) = default;
  UpdateDataType& operator=(const UpdateDataType&) = default;
  // movable
  UpdateDataType(UpdateDataType&&) = default;
  UpdateDataType& operator=(UpdateDataType&&) = default;

  bool IsUnknown() const noexcept {
    return type_ == base::to_underlying(Type::Unknown);
  }

  bool IsUpdateExplictByUser() const noexcept {
    return type_ & base::to_underlying(Type::UpdateExplictByUser);
  }

  bool IsUpdateByKernelFromCtor() const noexcept {
    return type_ & base::to_underlying(Type::UpdateByKernelFromCtor);
  }

  bool IsUpdateByKernelFromRender() const noexcept {
    return type_ & base::to_underlying(Type::UpdateByKernelFromRender);
  }

  bool IsUpdateByKernel() const noexcept {
    return IsUpdateByKernelFromCtor() || IsUpdateByKernelFromRender() ||
           IsUpdateByKernelFromHydrate() || IsUpdateByKernelFromGetDerived() ||
           IsUpdateByKernelFromConflict();
  }

  bool IsUpdateByKernelFromHydrate() const noexcept {
    return type_ & base::to_underlying(Type::UpdateByKernelFromHydrate);
  }

  bool IsUpdateByKernelFromGetDerived() const noexcept {
    return type_ & base::to_underlying(Type::UpdateByKernelFromGetDerived);
  }

  bool IsUpdateByKernelFromConflict() const noexcept {
    return type_ & base::to_underlying(Type::UpdateByKernelFromConflict);
  }

  explicit operator std::uint32_t() const { return type_; }

 private:
  // this should sync with `oliver/lynx-kernel/src/typings/native.ts`
  enum class Type : std::uint32_t {
    // default
    Unknown = 0,

    // update by `setState` or `setData`
    UpdateExplictByUser = 1,

    // update by lynx_core from ctor
    UpdateByKernelFromCtor = 1 << 1,

    // update by lynx_core from render
    UpdateByKernelFromRender = 1 << 2,

    // update by ssr hydrate
    UpdateByKernelFromHydrate = 1 << 3,

    // update by `getDerivedStateFromProps`
    UpdateByKernelFromGetDerived = 1 << 4,

    // update by conflict detected
    UpdateByKernelFromConflict = 1 << 5,
  };

  std::uint32_t type_;
};

}  // namespace lynx::runtime

#endif  // CORE_RUNTIME_PIPER_JS_UPDATE_DATA_TYPE_H_
