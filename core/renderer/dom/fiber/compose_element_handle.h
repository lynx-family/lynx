// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_COMPOSE_ELEMENT_HANDLE_H_
#define CORE_RENDERER_DOM_FIBER_COMPOSE_ELEMENT_HANDLE_H_

#include <cstdint>
#include <utility>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/ref_counted_class.h"
#include "core/renderer/dom/element.h"

namespace lynx {
namespace tasm {

class ComposeModifierApplicator;

enum class ComposeElementKind : int32_t {
  kView = 1,
  kText = 2,
  kImage = 3,
};

// Stable, opaque identity for one Compose-owned Fiber subtree. The concrete
// Element always owns view/text/image behavior; mount_root_ only describes the
// physical node that should be inserted into its external parent.
class ComposeElementHandle final : public lepus::RefCounted {
 public:
  ComposeElementHandle(ComposeElementKind, fml::RefPtr<Element> content_element)
      : content_element_(std::move(content_element)),
        mount_root_(content_element_) {}

  void ReleaseSelf() const override { delete this; }
  lepus::RefType GetRefType() const override {
    return lepus::RefType::kComposeElementHandle;
  }

  const fml::RefPtr<Element>& content_element() const {
    return content_element_;
  }
  const fml::RefPtr<Element>& mount_root() const { return mount_root_; }

 private:
  friend class ComposeModifierApplicator;

  void SetMountRoot(fml::RefPtr<Element> mount_root) {
    mount_root_ = std::move(mount_root);
  }

  fml::RefPtr<Element> content_element_;
  fml::RefPtr<Element> mount_root_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_COMPOSE_ELEMENT_HANDLE_H_
