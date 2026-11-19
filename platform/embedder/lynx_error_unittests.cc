// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <type_traits>

#include "base/include/debug/lynx_error.h"
#include "base/include/value/table.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "platform/embedder/core/native_facade_impl.h"
#include "platform/embedder/lynx_view_clients.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace {
using lynx::embedder::LynxTemplateRenderer;
using lynx::embedder::LynxViewClients;

static_assert(std::is_standard_layout<lynx_error_t>::value &&
              std::is_trivial<lynx_error_t>::value);

struct ErrorCopy {
  int count = 0;
  int code = 0;
  int sub_code = 0;
  int type = 0;
  std::string summary;
  int32_t level = 0;
  std::string suggestion;
  std::string cause;
  std::unordered_map<std::string, std::string> custom_info;
  std::vector<int32_t> consumers;
  lynx::lepus::Value custom_fields;

  static void Receive(lynx_view_client_t* client, const lynx_error_t* error) {
    ASSERT_NE(error, nullptr);
    ASSERT_GE(error->struct_size, sizeof(lynx_error_t));
    auto* copy =
        static_cast<ErrorCopy*>(lynx_view_client_get_user_data(client));
    ++copy->count;
    copy->code = error->error_code;
    copy->sub_code = error->sub_code;
    copy->type = error->type;
    copy->summary.assign(error->summary_message, error->summary_size);
    copy->level = error->level;
    copy->suggestion.assign(error->fix_suggestion, error->suggestion_size);
    copy->cause.assign(error->root_cause, error->cause_size);
    copy->custom_fields = lynx::lepus::Value(nullptr, *error->custom_info);
    copy->custom_info.clear();
    for (const auto& entry : *copy->custom_fields.Table()) {
      copy->custom_info.emplace(entry.first.str(), entry.second.StdString());
    }
    copy->consumers.clear();
    if (error->consumer_count > 0) {
      ASSERT_NE(error->consumers, nullptr);
      copy->consumers.assign(error->consumers,
                             error->consumers + error->consumer_count);
    }
  }
};

ErrorCopy ReceiveError(
    int level, int32_t code, const std::string& summary,
    const std::string& suggestion,
    const std::unordered_map<std::string, std::string>& custom_info,
    const std::string& cause) {
  ErrorCopy copy;
  auto* client = lynx_view_client_create(&copy);
  lynx_view_client_bind_on_received_error_v2(client, ErrorCopy::Receive);
  LynxViewClients clients(nullptr);
  clients.AddClient(client);
  clients.OnErrorOccurred(level, code, summary, suggestion, custom_info, false,
                          cause);
  clients.RemoveClient(client);
  lynx_view_client_release(client);
  return copy;
}

TEST(LynxErrorTest, KnownCodeUsesMetadataAndKeepsOriginalSubCode) {
  const auto code = lynx::error::E_APP_BUNDLE_LOAD_BAD_RESPONSE;
  const auto metadata = lynx::error::GetMetaData(code);
  ASSERT_NE(metadata, nullptr);
  const auto error = ReceiveError(2, code, "network failed", "local suggestion",
                                  {}, "provider cause");
  EXPECT_EQ(error.code, code / 100);
  EXPECT_EQ(error.sub_code, code);
  EXPECT_EQ(error.level, static_cast<int32_t>(metadata->level_));
  EXPECT_EQ(error.suggestion, metadata->fix_suggestion_.empty()
                                  ? "local suggestion"
                                  : metadata->fix_suggestion_);
  EXPECT_EQ(error.type, -1);
  EXPECT_EQ(error.summary, "network failed");
  EXPECT_EQ(error.cause, "provider cause");
}

TEST(LynxErrorTest, UnknownLegacyAndNegativeCodesAreNotDivided) {
  for (int code : {100, 102, 999999, -42}) {
    ASSERT_EQ(lynx::error::GetMetaData(code), nullptr);
    const auto error = ReceiveError(2, code, "warning", "local", {}, "");
    EXPECT_EQ(error.code, code);
    EXPECT_EQ(error.sub_code, code);
    EXPECT_EQ(error.level, 2);
    EXPECT_EQ(error.suggestion, "local");
    EXPECT_TRUE(error.consumers.empty());
  }
}

TEST(LynxErrorTest, UnspecifiedMetadataRetainsSiteLevelAndSuggestion) {
  const auto code = lynx::error::E_BTS_RUNTIME_ERROR;
  ASSERT_EQ(lynx::error::GetMetaData(code)->level_,
            lynx::error::Level::UNDECIDED);
  for (int level : {0, 1, 2, 3, -1, 42}) {
    const auto error = ReceiveError(level, code, "JS error", "local", {}, "");
    EXPECT_EQ(error.level, level);
    EXPECT_EQ(error.suggestion, "local");
    EXPECT_EQ(error.code, code / 100);
    EXPECT_EQ(error.sub_code, code);
    // A JS code does not change the source of native-path delivery.
    EXPECT_EQ(error.type, -1);
  }
}

TEST(LynxErrorTest, CustomInfoAndConsumersAreForwarded) {
  const std::unordered_map<std::string, std::string> custom_info = {
      {"error_code", "custom code"},
      {"lynx_context_component", "image"},
      {"error_stack", "stack"},
      {std::string("k\0y", 3), std::string("v\0lue", 5)},
  };
  const std::string summary("sum\0mary", 8);
  const std::string cause("ca\0se", 5);
  const auto error =
      ReceiveError(1, lynx::error::E_APP_BUNDLE_RELOAD_EARLY_RELOAD, summary,
                   "", custom_info, cause);
  EXPECT_EQ(error.summary, summary);
  EXPECT_EQ(error.cause, cause);
  EXPECT_EQ(error.custom_info, custom_info);
  EXPECT_EQ(error.consumers, (std::vector<int32_t>{1}));
  // Retained values remain readable after native delivery and client teardown.
  EXPECT_EQ(error.custom_fields.GetProperty("error_stack").StdString(),
            "stack");
  EXPECT_EQ(error.consumers.front(), 1);
}

TEST(LynxErrorTest, UnknownLevelIsForwardedUnchanged) {
  const auto error = ReceiveError(-1, -1, "", "", {}, "");
  EXPECT_EQ(error.level, -1);
  EXPECT_EQ(error.summary, "");
  EXPECT_EQ(error.cause, "");
  EXPECT_TRUE(error.custom_info.empty());
  EXPECT_TRUE(error.consumers.empty());
}

TEST(LynxErrorTest, V2PreferredAndLogBoxOnlyLegacyDeliveryPreserved) {
  ErrorCopy copy;
  auto* client = lynx_view_client_create(&copy);
  LynxViewClients clients(nullptr);
  clients.AddClient(client);
  lynx_view_client_bind_on_received_error(
      client, [](lynx_view_client_t* client, int code, const char* summary) {
        auto* copy =
            static_cast<ErrorCopy*>(lynx_view_client_get_user_data(client));
        ++copy->count;
        copy->code = code;
        copy->summary = summary;
      });
  lynx_view_client_bind_on_received_error_v2(client, ErrorCopy::Receive);
  auto report = [&](bool logbox_only) {
    clients.OnErrorOccurred(1, lynx::error::E_APP_BUNDLE_LOAD_BAD_RESPONSE,
                            "summary", "", {}, logbox_only, "cause");
  };
  report(false);
  EXPECT_EQ(copy.count, 1);
  EXPECT_EQ(copy.code, lynx::error::E_APP_BUNDLE_LOAD_BAD_RESPONSE / 100);
  EXPECT_EQ(copy.sub_code, lynx::error::E_APP_BUNDLE_LOAD_BAD_RESPONSE);
  EXPECT_EQ(copy.cause, "cause");
  report(true);
  EXPECT_EQ(copy.count, 1);
  lynx_view_client_bind_on_received_error_v2(client, nullptr);
  report(true);
  EXPECT_EQ(copy.count, 2);
  EXPECT_EQ(copy.code, lynx::error::E_APP_BUNDLE_LOAD_BAD_RESPONSE);
  EXPECT_EQ(copy.summary, "summary");
  clients.RemoveClient(client);
  lynx_view_client_release(client);
  // Owned strings remain valid after the callback and client expire.
  EXPECT_EQ(copy.cause, "cause");
}

class LogBoxErrorClient : public lynx::embedder::TemplateRendererClient {
 public:
  void OnErrorOccurred(
      int level, int32_t code, const std::string& summary,
      const std::string& suggestion,
      const std::unordered_map<std::string, std::string>& custom_info,
      bool is_logbox_only, const std::string& root_cause) override {
    ++count;
    logbox_only = is_logbox_only;
    cause = root_cause;
  }
  int count = 0;
  bool logbox_only = false;
  std::string cause;
};

TEST(LynxErrorTest, NativeFacadePreservesCauseAndLogBoxDelivery) {
  LynxTemplateRenderer renderer({}, nullptr, nullptr, nullptr);
  LynxViewClients clients(nullptr);
  LogBoxErrorClient logbox;
  ErrorCopy copy;
  auto* client = lynx_view_client_create(&copy);
  lynx_view_client_bind_on_received_error_v2(client, ErrorCopy::Receive);
  clients.AddClient(client);
  renderer.AddClient(&clients);
  renderer.AddClient(&logbox);
  // Empty data errors occur before the client receives page start.
  renderer.LoadTemplate("test://early", {}, nullptr, nullptr);
  lynx::embedder::NativeFacadeImpl facade(&renderer);
  lynx::base::LynxError error(lynx::error::E_BTS_RUNTIME_ERROR, "JS failure");
  error.root_cause_ = "original cause";
  facade.ReportError(error);
  EXPECT_EQ(copy.cause, "original cause");
  EXPECT_EQ(copy.level, 1);
  EXPECT_EQ(copy.type, -1);
  EXPECT_EQ(logbox.count, 2);
  EXPECT_FALSE(logbox.logbox_only);
  EXPECT_EQ(logbox.cause, "original cause");
  error.is_logbox_only_ = true;
  facade.ReportError(error);
  EXPECT_EQ(copy.count, 2);
  EXPECT_EQ(logbox.count, 3);
  EXPECT_TRUE(logbox.logbox_only);
  EXPECT_EQ(logbox.cause, "original cause");
  clients.RemoveClient(client);
  lynx_view_client_release(client);
}

TEST(LynxErrorTest, NoResourceLoaderReportsErrorBeforePageStart) {
  LynxTemplateRenderer renderer({}, nullptr, nullptr, nullptr);
  LynxViewClients clients(nullptr);
  ErrorCopy copy;
  auto* client = lynx_view_client_create(&copy);
  lynx_view_client_bind_on_received_error_v2(client, ErrorCopy::Receive);
  lynx_view_client_bind_on_page_start(
      client, [](lynx_view_client_t*, const char*) {
        ADD_FAILURE() << "A failed load must not report page start.";
      });
  clients.AddClient(client);
  renderer.AddClient(&clients);
  renderer.LoadTemplateFromURL("test://first");
  EXPECT_EQ(copy.summary,
            "Failed to load template test://first: no resource loader.");
  renderer.LoadTemplateFromURL("test://second");
  EXPECT_EQ(copy.summary,
            "Failed to load template test://second: no resource loader.");
  EXPECT_EQ(copy.count, 2);
  // Provider causes remain available without native-owned URL state.
  renderer.OnErrorOccurred(1, -42, "late failure", "", {}, false,
                           "provider cause");
  EXPECT_EQ(copy.cause, "provider cause");
  clients.RemoveClient(client);
  lynx_view_client_release(client);
}
}  // namespace
