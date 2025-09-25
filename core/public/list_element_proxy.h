// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_LIST_ELEMENT_PROXY_H_
#define CORE_PUBLIC_LIST_ELEMENT_PROXY_H_

#include <memory>
#include <utility>

#include "core/public/list_data.h"

namespace lynx {
namespace shell {

class ListElementProxy {
 public:
  class Legacy {
   public:
    virtual ~Legacy() = default;
    virtual int32_t ObtainListChild(int32_t tag, uint32_t index,
                                    int64_t operation_id,
                                    bool enable_reuse_notification) = 0;
    virtual void ObtainListChildAsync(int32_t tag, uint32_t index,
                                      int64_t operation_id,
                                      bool enable_reuse_notification) = 0;

    virtual void RecycleListChild(int32_t tag, uint32_t sign) = 0;
    virtual void RecycleListChildAsync(int32_t tag, uint32_t sign) = 0;

    virtual void RenderListChild(int32_t tag, uint32_t index,
                                 int64_t operation_id) = 0;

    virtual void UpdateListChild(int32_t tag, uint32_t sign, uint32_t index,
                                 int64_t operation_id) = 0;

    virtual tasm::ListData GetListData(int view_id) = 0;
  };

  explicit ListElementProxy(std::unique_ptr<Legacy> legacy)
      : legacy_(std::move(legacy)) {}
  virtual ~ListElementProxy() = default;

  virtual void ScrollByListContainer(int32_t tag, float x, float y,
                                     float original_x, float original_y) = 0;

  virtual void ScrollToPosition(int32_t tag, int index, float offset, int align,
                                bool smooth) = 0;

  virtual void ScrollStopped(int32_t tag) = 0;

  Legacy* GetLegacy() { return legacy_.get(); }

 protected:
  std::unique_ptr<Legacy> legacy_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_PUBLIC_LIST_ELEMENT_PROXY_H_
