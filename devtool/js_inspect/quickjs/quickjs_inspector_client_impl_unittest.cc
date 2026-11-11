// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#define private public
#include "devtool/js_inspect/quickjs/quickjs_inspector_client_impl.h"
#undef private

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {
namespace {

class RecordingSession final : public quickjs_inspector::QJSInspectorSession {
 public:
  explicit RecordingSession(std::vector<std::string>* messages)
      : messages_(messages) {}

  void DispatchProtocolMessage(const std::string& message) override {
    messages_->push_back(message);
  }
  void SchedulePauseOnNextStatement(const std::string& reason) override {}
  void CancelPauseOnNextStatement() override {}
  void SetEnableConsoleInspect(bool enable) override {}

 private:
  std::vector<std::string>* messages_;
};

class RecordingInspector final : public quickjs_inspector::QJSInspector {
 public:
  explicit RecordingInspector(
      std::vector<std::vector<std::string>*> session_messages)
      : session_messages_(std::move(session_messages)) {}

  void AddContext(LEPUSContext* ctx, const std::string& name) override {}
  void RemoveContext(LEPUSContext* ctx) override {}

  std::unique_ptr<quickjs_inspector::QJSInspectorSession> Connect(
      QJSChannel* channel, const std::string& group_id, int32_t session_id,
      LEPUSContext* context) override {
    connected_contexts_.push_back(context);
    return std::make_unique<RecordingSession>(
        session_messages_.at(connection_count_++));
  }

  size_t ConnectionCount() const { return connection_count_; }
  const std::vector<LEPUSContext*>& ConnectedContexts() const {
    return connected_contexts_;
  }

 private:
  std::vector<std::vector<std::string>*> session_messages_;
  std::vector<LEPUSContext*> connected_contexts_;
  size_t connection_count_{0};
};

TEST(QJSInspectorClientImplTest,
     StaleRuntimeCannotDisconnectReplacementSharedGroupSession) {
  constexpr int kViewId = 7;
  constexpr int64_t kOldRuntimeId = 101;
  constexpr int64_t kNewRuntimeId = 102;
  constexpr char kSharedGroupId[] = "shared-group";
  constexpr char kMessage[] = R"({"id":1,"method":"Runtime.evaluate"})";
  auto* old_context = reinterpret_cast<LEPUSContext*>(0x1001);
  auto* new_context = reinterpret_cast<LEPUSContext*>(0x1002);

  std::vector<std::string> old_messages;
  std::vector<std::string> new_messages;
  auto inspector = std::make_unique<RecordingInspector>(
      std::vector{&old_messages, &new_messages});
  auto* inspector_ptr = inspector.get();
  auto client = std::make_shared<QJSInspectorClientImpl>();
  client->inspectors_.emplace(kSharedGroupId, std::move(inspector));

  client->ConnectSession(kViewId, kSharedGroupId, old_context, kOldRuntimeId);
  client->ConnectSession(kViewId, kSharedGroupId, new_context, kNewRuntimeId);
  client->ConnectSession(kViewId, kSharedGroupId, new_context, kNewRuntimeId);
  client->DisconnectSession(kViewId, kOldRuntimeId);
  client->DispatchMessage(kMessage, kViewId);

  EXPECT_EQ(inspector_ptr->ConnectionCount(), 2u);
  EXPECT_EQ(inspector_ptr->ConnectedContexts(),
            (std::vector<LEPUSContext*>{old_context, new_context}));
  EXPECT_TRUE(old_messages.empty());
  EXPECT_EQ(new_messages, std::vector<std::string>{kMessage});

  client->DisconnectSession(kViewId, kNewRuntimeId);
}

}  // namespace
}  // namespace testing
}  // namespace devtool
}  // namespace lynx
