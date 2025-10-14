// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_PUBLIC_UI_COMPONENT_DELEGATE_H_
#define CLAY_PUBLIC_UI_COMPONENT_DELEGATE_H_

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>

#include "core/public/list_engine_proxy.h"

namespace clay {
class LynxListData;
class UIComponentDelegate {
 public:
  virtual int OnObtainChild(int view_id, int index, int operation_id) = 0;
  virtual void OnRecycleChild(int view_id, int child_id) = 0;
  virtual void OnCreateAddChild(int view_id, int index, int operation_id) = 0;
  virtual void OnUpdateChild(int parent_id, int to_update_id, int target_index,
                             int64_t operation_id) = 0;
  virtual LynxListData* OnListGetData(int view_id) = 0;
  virtual void OnScrollByListContainer(int view_id, float offset_x,
                                       float offset_y, float original_x,
                                       float original_y) = 0;
  virtual void OnScrollToPosition(int view_id, int position, float offset,
                                  int align, bool smooth) = 0;
  virtual void OnScrollStopped(int view_id) = 0;
  virtual bool OnEnableRasterAnimation() = 0;
  virtual std::list<int32_t> GetAncestorElements(int32_t tag) = 0;

  void SetListEngineProxy(
      const std::shared_ptr<lynx::shell::ListEngineProxy>& list_engine_proxy) {
    list_engine_proxy_ = list_engine_proxy;
  }

  std::shared_ptr<lynx::shell::ListEngineProxy> GetListEngineProxy() {
    return list_engine_proxy_;
  }

 private:
  std::shared_ptr<lynx::shell::ListEngineProxy> list_engine_proxy_ = nullptr;
};
}  // namespace clay

#endif  // CLAY_PUBLIC_UI_COMPONENT_DELEGATE_H_
