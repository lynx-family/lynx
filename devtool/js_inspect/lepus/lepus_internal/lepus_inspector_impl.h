// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTOR_IMPL_H_
#define DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTOR_IMPL_H_

#include <memory>
#include <string>

#include "devtool/js_inspect/lepus/lepus_internal/lepus_inspected_context.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepus_inspector_ng.h"

namespace lepus_inspector {

class LepusInspectorNGImpl;

class LepusInspectorSessionNGImpl : public LepusInspectorSessionNG {
 public:
  static std::unique_ptr<LepusInspectorSessionNGImpl> Create(
      LepusInspectorNGImpl* inspector, LepusInspectorNG::LepusChannel* channel);
  ~LepusInspectorSessionNGImpl() override;

  void DispatchProtocolMessage(const std::string& message) override;
  void SchedulePauseOnNextStatement(const std::string& reason) override;
  void CancelPauseOnNextStatement() override {}

  void SendProtocolResponse(int call_id, const std::string& message);
  void SendProtocolNotification(const std::string& message);

 private:
  LepusInspectorSessionNGImpl(LepusInspectorNGImpl* inspector,
                              LepusInspectorNG::LepusChannel* channel);

  LepusInspectorNGImpl* inspector_;
  LepusInspectorNG::LepusChannel* channel_;
};

class LepusInspectorNGImpl : public LepusInspectorNG {
 public:
  ~LepusInspectorNGImpl() override = default;

  std::unique_ptr<LepusInspectorSessionNG> Connect(
      LepusChannel* channel) override;

  LepusInspectorClientNG* GetClient() { return client_; }
  const std::shared_ptr<LepusInspectedContext>& GetContext() {
    return context_;
  }
  LepusInspectorSessionNGImpl* GetSession() { return session_; }
  void RemoveSession() { session_ = nullptr; }

  void SetDebugInfo(const std::string& url,
                    const std::string& debug_info) override;

 private:
  friend class LepusInspectorNG;
  LepusInspectorNGImpl(lynx::lepus::Context* context,
                       LepusInspectorClientNG* client, const std::string& name);

  LepusInspectorClientNG* client_;
  std::shared_ptr<LepusInspectedContext> context_;
  LepusInspectorSessionNGImpl* session_;
};

}  // namespace lepus_inspector

#endif  // DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUS_INSPECTOR_IMPL_H_
