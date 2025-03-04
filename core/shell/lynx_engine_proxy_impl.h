// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_ENGINE_PROXY_IMPL_H_
#define CORE_SHELL_LYNX_ENGINE_PROXY_IMPL_H_

#include <memory>
#include <string>

#include "core/public/lynx_engine_proxy.h"
#include "core/shell/lynx_actor_specialization.h"
#include "core/shell/lynx_engine.h"

namespace lynx {
namespace shell {

class LynxEngineProxyImpl : public LynxEngineProxy {
 public:
  LynxEngineProxyImpl(
      std::shared_ptr<shell::LynxActor<shell::LynxEngine>> actor)
      : engine_actor_(actor) {}
  ~LynxEngineProxyImpl() = default;

  bool SendTouchEvent(const std::string& name, int32_t tag, float x, float y,
                      float client_x, float client_y, float page_x,
                      float page_y, int64_t timestamp = 0) override;

  bool SendTouchEvent(const std::string& name, const pub::Value& params,
                      int64_t timestamp = 0) override;

  void SendCustomEvent(const std::string& name, int32_t tag,
                       const pub::Value& params,
                       const std::string& params_name) override;

  void SendGestureEvent(int tag, int gesture_id, std::string name,
                        const pub::Value& params) override;

  void SendBubbleEvent(const std::string& name, int32_t tag,
                       const pub::Value& params) override;

  void OnPseudoStatusChanged(int32_t id, int32_t pre_status,
                             int32_t current_status) override;

  void ScrollByListContainer(int32_t tag, float x, float y, float original_x,
                             float original_y) override;

  void ScrollToPosition(int32_t tag, int index, float offset, int align,
                        bool smooth) override;

  void ScrollStopped(int32_t tag) override;

  int32_t ObtainListChild(int32_t tag, uint32_t index, int64_t operation_id,
                          bool enable_reuse_notification) override;

  void RecycleListChild(int32_t tag, uint32_t sign) override;

  void RenderListChild(int32_t tag, uint32_t index,
                       int64_t operation_id) override;

  void UpdateListChild(int32_t tag, uint32_t sign, uint32_t index,
                       int64_t operation_id) override;

  tasm::ListData GetListData(int view_id) override;

  void MarkLayoutDirty(int sign) override;

  bool EnableRasterAnimation() override;

  float GetDensity() const override;

  void OnFirstMeaningfulPaint() override;

 protected:
  std::shared_ptr<shell::LynxActor<shell::LynxEngine>> engine_actor_;
};

}  // namespace shell

}  // namespace lynx

#endif  // CORE_SHELL_LYNX_ENGINE_PROXY_IMPL_H_
