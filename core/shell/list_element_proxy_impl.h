// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LIST_ELEMENT_PROXY_IMPL_H_
#define CORE_SHELL_LIST_ELEMENT_PROXY_IMPL_H_

#include <memory>

#include "core/public/list_element_proxy.h"
#include "core/shell/lynx_engine.h"

namespace lynx {
namespace shell {

class ListElementProxyImpl : public ListElementProxy {
 public:
  class LegacyImpl : public Legacy {
   public:
    explicit LegacyImpl(shell::LynxActor<shell::LynxEngine>* actor)
        : engine_actor_(actor) {}

    int32_t ObtainListChild(int32_t tag, uint32_t index, int64_t operation_id,
                            bool enable_reuse_notification) override;
    void ObtainListChildAsync(int32_t tag, uint32_t index, int64_t operation_id,
                              bool enable_reuse_notification) override;

    void RecycleListChild(int32_t tag, uint32_t sign) override;
    void RecycleListChildAsync(int32_t tag, uint32_t sign) override;

    void RenderListChild(int32_t tag, uint32_t index,
                         int64_t operation_id) override;

    void UpdateListChild(int32_t tag, uint32_t sign, uint32_t index,
                         int64_t operation_id) override;

    tasm::ListData GetListData(int view_id) override;

   private:
    shell::LynxActor<shell::LynxEngine>* engine_actor_;
  };

  explicit ListElementProxyImpl(
      std::shared_ptr<shell::LynxActor<shell::LynxEngine>> actor)
      : ListElementProxy(std::make_unique<LegacyImpl>(actor.get())),
        engine_actor_(actor) {}

  void ScrollByListContainer(int32_t tag, float x, float y, float original_x,
                             float original_y) override;

  void ScrollToPosition(int32_t tag, int index, float offset, int align,
                        bool smooth) override;

  void ScrollStopped(int32_t tag) override;

 private:
  std::shared_ptr<shell::LynxActor<shell::LynxEngine>> engine_actor_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LIST_ELEMENT_PROXY_IMPL_H_
