// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_VIEW_TREE_OBSERVER_H_
#define CLAY_UI_COMPONENT_VIEW_TREE_OBSERVER_H_

#include <list>

namespace clay {

// Interface definition for a callback to be invoked when the view tree is about
// to be painting.
class OnPaintingListener {
 public:
  virtual ~OnPaintingListener() = default;
  virtual void OnPainting() = 0;
};

// Refer to android, A view tree observer is used to register
// listeners that can be notified of global changes in the view tree. Such
// global events now only define the painting event.
// We can define more global events in the future, such as layout events, etc.
class ViewTreeObserver {
 public:
  ViewTreeObserver() = default;
  ~ViewTreeObserver() = default;

  // Add a listener to be invoked when the view tree about to be painting.
  void AddOnPaintingListener(OnPaintingListener* listener);
  void RemoveOnPaintingListener(OnPaintingListener* listener);
  // Notifies registered listeners that the view tree is about to be painting.
  void DispatchOnPainting();

 private:
  std::list<OnPaintingListener*> on_painting_listeners_;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_VIEW_TREE_OBSERVER_H_
