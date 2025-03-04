// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_LIST_ORIENTATION_HELPER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_LIST_ORIENTATION_HELPER_H_

#include <memory>

#include "core/renderer/ui_component/list/item_holder.h"

namespace lynx {
namespace tasm {

class ListLayoutManager;

class ListOrientationHelper {
 public:
  static std::unique_ptr<ListOrientationHelper> CreateListOrientationHelper(
      ListLayoutManager* list_layout_manager, list::Orientation orientation);
  virtual ~ListOrientationHelper();

  virtual bool IsVertical() const = 0;
  // Get list size in main axis.
  virtual float GetMeasurement() const = 0;
  // Get list size in cross axis.
  virtual float GetMeasurementInOther() const = 0;
  // Get List content area.
  virtual float GetMeasurementInOtherWithoutPadding() const = 0;
  // Get list size port's start offset in main axis.
  virtual float GetStartAfterPadding() const = 0;
  // Get list size port's end offset in main axis.
  virtual float GetEndAfterPadding() const = 0;
  // Get list size port's start offset in cross axis.
  virtual float GetStartAfterPaddingInOther() const = 0;
  // Get list's end padding in main axis (padding-right / padding-bottom).
  virtual float GetEndPadding() const = 0;

  // Get child ItemHolder size in main axis.
  virtual float GetDecoratedMeasurement(
      const ItemHolder* item_holder) const = 0;
  // Get child ItemHolder size in cross axis.
  virtual float GetDecoratedMeasurementInOther(
      const ItemHolder* item_holder) const = 0;
  // Get child ItemHolder start layout offset in main axis.
  virtual float GetDecoratedStart(const ItemHolder* item_holder) const = 0;
  // Get child ItemHolder end layout offset in main axis.
  virtual float GetDecoratedEnd(const ItemHolder* item_holder) const = 0;
  // Get child ItemHolder start layout offset without main-axis-gap in main
  // axis.
  virtual float GetStart(const ItemHolder* item_holder) const = 0;
  // Get child ItemHolder's margin in main axis.
  virtual float GetItemHolderMainMargin(
      const ItemHolder* item_holder) const = 0;
  // Get child ItemHolder's margin in cross axis.
  virtual float GetItemHolderCrossMargin(
      const ItemHolder* item_holder) const = 0;

 protected:
  explicit ListOrientationHelper(ListLayoutManager* list_layout_manager);
  ListLayoutManager* list_layout_manager_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_LIST_ORIENTATION_HELPER_H_
