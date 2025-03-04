// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_REF_TYPE_H_
#define CORE_RUNTIME_VM_LEPUS_REF_TYPE_H_

#include <cstdint>

namespace lynx {
namespace lepus {

// TODO(songshourui.null): Currently, Element, AirElement, lepus::Array,
// lepus::Table, ByteArray, and LEPUSObject all inherit from lepus::RefCounted.
// In the future, new classes added to the Signal API will also be extended
// based on lepus::RefCounted. Since both Element and Signal are exposed to the
// frontend, Native needs to distinguish the specific types of
// lepus::RefCounted, hence the above changes were made. In the long term, we
// should avoid defining these enumerations within Lepus as much as possible. We
// will consider optimization plans in the future.
enum class RefType : int32_t {
  kLepusTable,
  kLepusArray,
  kByteArray,
  kJSIObject,
  kElement,
  kSignal,
  kComputation,
  kMemo,
  kScope,
  kOtherType,
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_REF_TYPE_H_
