// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_INSPECTOR_IMPL_H_
#define DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_INSPECTOR_IMPL_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_inspected_context.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_inspector.h"

namespace quickjs_inspector {

class QJSInspectorImpl;

// Dispatch messages(CDP, console, pause, etc) between the QuickJS and the
// DevTool.
class QJSInspectorSessionImpl : public QJSInspectorSession {
 public:
  static std::unique_ptr<QJSInspectorSessionImpl> Create(
      QJSInspectorImpl* inspector, int32_t session_id,
      QJSInspector::QJSChannel* channel);
  ~QJSInspectorSessionImpl() override;

  void DispatchProtocolMessage(const std::string& message) override;
  void SchedulePauseOnNextStatement(const std::string& reason) override;
  void CancelPauseOnNextStatement() override {}
  void SetEnableConsoleInspect(bool enable) override;

  void SendProtocolResponse(int call_id, const std::string& message);
  void SendProtocolNotification(const std::string& message);
  void OnConsoleMessage(const std::string& message, int32_t runtime_id);

 private:
  QJSInspectorSessionImpl(QJSInspectorImpl* inspector, int32_t session_id,
                          QJSInspector::QJSChannel* channel);

  QJSInspector::QJSChannel* channel_;
  QJSInspectorImpl* inspector_;
  int32_t session_id_;
};

using InspectorSessionMap =
    std::unordered_map<int32_t, QJSInspectorSessionImpl*>;

// Manage all the inspector related instances.
class QJSInspectorImpl : public QJSInspector {
 public:
  QJSInspectorImpl(LEPUSContext* ctx, QJSInspectorClient* client,
                   const std::string& group_id, const std::string& name);
  ~QJSInspectorImpl() override = default;

  std::unique_ptr<QJSInspectorSession> Connect(QJSChannel* channel,
                                               const std::string& group_id,
                                               int32_t session_id) override;

  QJSInspectorClient* GetClient() { return client_; }
  const std::unique_ptr<QJSInspectedContext>& GetContext() { return context_; }
  const std::string& GetGroupID() { return group_id_; }
  QJSInspectorSessionImpl* GetSession(int32_t session_id);
  // get all the sessions
  const InspectorSessionMap& GetSessions() { return sessions_; }
  // remove session by session id
  void RemoveSession(int32_t session_id);

  bool IsFullFuncEnabled();

 private:
  QJSInspectorClient* client_;
  std::unique_ptr<QJSInspectedContext> context_;
  InspectorSessionMap sessions_;
  std::string group_id_;
};

}  // namespace quickjs_inspector

#endif  // DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_INSPECTOR_IMPL_H_
