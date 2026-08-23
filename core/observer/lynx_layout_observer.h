// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_OBSERVER_LYNX_LAYOUT_OBSERVER_H_
#define CORE_OBSERVER_LYNX_LAYOUT_OBSERVER_H_

#include <cstdint>
#include <string>

namespace lynx {

namespace lepus {
class Value;
}

namespace starlight {
class LayoutObject;
}

namespace observer {

class LayoutHierarchyObserver {
 public:
  virtual ~LayoutHierarchyObserver() = default;

  virtual void OnLayoutObjectCreated(
      int32_t id, starlight::LayoutObject* layout_object) = 0;
  virtual void OnLayoutObjectDestroyed(int32_t id) = 0;
  virtual void OnComponentUselessUpdate(const std::string& component_name,
                                        const lepus::Value& properties) = 0;
};

class LayoutComponentObserver {
 public:
  virtual ~LayoutComponentObserver() = default;

  virtual void OnLayoutNodeTypeAttached(const std::string& tag,
                                        int32_t type) = 0;
};

class LynxLayoutObserver {
 public:
  virtual ~LynxLayoutObserver() = default;

  virtual LayoutHierarchyObserver* Hierarchy() const = 0;
  virtual LayoutComponentObserver* Component() const = 0;
};

}  // namespace observer
}  // namespace lynx

#endif  // CORE_OBSERVER_LYNX_LAYOUT_OBSERVER_H_
