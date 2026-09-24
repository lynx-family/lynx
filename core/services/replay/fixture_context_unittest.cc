// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/fixture_context.h"

#include <sys/stat.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_set>

#include "core/services/replay/fixture_evaluator.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

#if defined(OS_WIN)
#include <direct.h>

#include <random>
#else
#include <unistd.h>
#endif

namespace lynx {
namespace tasm {
namespace replay {
namespace {

bool CreateDirectory(const std::string& path) {
#if defined(OS_WIN)
  return _mkdir(path.c_str()) == 0;
#else
  return mkdir(path.c_str(), 0700) == 0;
#endif
}

class FixtureContextTest : public ::testing::Test {
 protected:
  void SetUp() override {
#if defined(OS_WIN)
    // Windows has no mkdtemp; mkdir still creates the directory exclusively.
    std::random_device random;
    for (int attempt = 0; attempt < 16; ++attempt) {
      auto candidate = ::testing::TempDir() + "lynx_fixture_context_" +
                       std::to_string(random());
      if (CreateDirectory(candidate)) {
        fixture_directory_ = candidate;
        break;
      }
    }
#else
    std::string directory_template =
        ::testing::TempDir() + "lynx_fixture_context_XXXXXX";
    char* directory = mkdtemp(directory_template.data());
    ASSERT_NE(directory, nullptr);
    fixture_directory_ = directory;
#endif
    ASSERT_FALSE(fixture_directory_.empty());
    ASSERT_TRUE(CreateDirectory(fixture_directory_ + "/assets"));
  }

  void TearDown() override {
    if (!fixture_directory_.empty()) {
      for (const auto& path : written_files_) {
        EXPECT_EQ(std::remove(path.c_str()), 0) << path;
      }
#if defined(OS_WIN)
      _rmdir((fixture_directory_ + "/assets").c_str());
      EXPECT_EQ(_rmdir(fixture_directory_.c_str()), 0);
#else
      rmdir((fixture_directory_ + "/assets").c_str());
      EXPECT_EQ(rmdir(fixture_directory_.c_str()), 0);
#endif
    }
  }

  void WriteFixture(const std::string& source) {
    WriteFile(fixture_directory_ + "/fixture.js", source);
  }

  void WriteAsset(const std::string& path, const std::string& content) {
    auto file_path = fixture_directory_ + "/assets/" + path;
    WriteFile(file_path, content);
  }

  std::string fixture_directory_;
  std::unordered_set<std::string> written_files_;

 private:
  void WriteFile(const std::string& path, const std::string& content) {
    std::ofstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    written_files_.insert(path);
    stream << content;
    ASSERT_TRUE(stream.good());
  }
};

void ExpectJson(const std::string& actual, const char* expected) {
  rapidjson::Document actual_doc, expected_doc;
  ASSERT_FALSE(actual_doc.Parse(actual.c_str()).HasParseError()) << actual;
  ASSERT_FALSE(expected_doc.Parse(expected).HasParseError()) << expected;
  EXPECT_EQ(actual_doc, expected_doc) << actual;
}

void ExpectUnhandled(const FixtureDispatchResult& result) {
  EXPECT_FALSE(result.handled);
  EXPECT_TRUE(result.return_value_json.empty());
  EXPECT_TRUE(result.callbacks.empty());
}

void ExpectCallback(const FixtureDispatchResult::CallbackCall& call,
                    size_t index, const char* json, int64_t delay_ms) {
  EXPECT_EQ(call.index, index);
  ExpectJson(call.value_json, json);
  EXPECT_EQ(call.delay_ms, delay_ms);
}

TEST_F(FixtureContextTest, HandlesMissingScriptAndUnavailableHandlers) {
  FixtureContext context;
  ExpectUnhandled(context.Dispatch("M", "m", "[]", {}));
  EXPECT_FALSE(context.Initialize(fixture_directory_));
  EXPECT_FALSE(context.initialized());
  WriteFixture("ctx.register('M', 'm', function() { return null; });");
  ASSERT_TRUE(context.Initialize(fixture_directory_));
  EXPECT_TRUE(context.HasHandler("M", "m"));
  EXPECT_FALSE(context.HasHandler("Unknown", "m"));
  ExpectUnhandled(context.Dispatch("Unknown", "m", "[]", {}));
}

TEST_F(FixtureContextTest, PreservesArgumentsAndRestoresFunctionPlaceholders) {
  WriteFixture(R"(
export default function(ctx) {
  ctx.register('Network', 'request', function(args) {
    return {status: args[0].code, path: args[1], isFn: typeof args[2] === 'function'};
  });
}
)");
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));
  auto result = context.Dispatch("Network", "request",
                                 R"([{"code":200},"/v1/data",null])", {2});
  ASSERT_TRUE(result.handled);
  EXPECT_TRUE(result.callbacks.empty());
  ExpectJson(result.return_value_json,
             R"({"status":200,"path":"/v1/data","isFn":true})");
}

TEST_F(FixtureContextTest, ReplaysCallbacksInOrderWithRecordedDelays) {
  WriteFixture(R"(
ctx.register('Event', 'listen', function(args, callbacks) {
  callbacks[0]({first: true});
  callbacks[1]({second: 2}, 50);
});
)");
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));
  auto result = context.Dispatch("Event", "listen", "[null,null]", {0, 1});
  ASSERT_TRUE(result.handled);
  ASSERT_EQ(result.callbacks.size(), 2u);
  ExpectCallback(result.callbacks[0], 0, R"({"first":true})", 0);
  ExpectCallback(result.callbacks[1], 1, R"({"second":2})", 50);
}

TEST_F(FixtureContextTest, ReadsJsonAssetsAndRejectsUnsafePaths) {
  WriteAsset("config.json", R"({"feature":"on"})");
  WriteFixture(R"(
ctx.register('Asset', 'read', function(args) { return ctx.readAsset(args[0]); });
)");
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));
  auto read =
      context.Dispatch("Asset", "read", R"(["assets/config.json"])", {});
  ASSERT_TRUE(read.handled);
  ExpectJson(read.return_value_json, R"({"feature":"on"})");
  auto escape = context.Dispatch("Asset", "read", R"(["../secret.json"])", {});
  ASSERT_TRUE(escape.handled);
  EXPECT_TRUE(escape.return_value_json.empty());
}

TEST_F(FixtureContextTest, NonFiniteCallbackDelayIsNormalized) {
  WriteFixture(R"(
ctx.register('Delay', 'call', function(args, callbacks) {
  callbacks[0](1, NaN);
  callbacks[0](2, Infinity);
  callbacks[1](3, 1e308);
});
)");
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));
  auto result = context.Dispatch("Delay", "call", "[null,null]", {0, 1});
  ASSERT_TRUE(result.handled);
  ASSERT_EQ(result.callbacks.size(), 3u);
  ExpectCallback(result.callbacks[0], 0, "1", 0);
  ExpectCallback(result.callbacks[1], 0, "2", 0);
  ExpectCallback(result.callbacks[2], 1, "3",
                 std::numeric_limits<int64_t>::max());
}

TEST_F(FixtureContextTest, StaleCallbackProxyFromEarlierDispatchIsHarmless) {
  WriteFixture(R"(
ctx.register('Stale', 'call', function(args, callbacks) {
  if (!this.saved) {
    this.saved = callbacks[0];
    return 'stored';
  }
  this.saved({late: true});
  return 'fired';
});
)");
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));
  for (const char* expected : {"\"stored\"", "\"fired\""}) {
    SCOPED_TRACE(expected);
    auto result = context.Dispatch("Stale", "call", "[null]", {0});
    ASSERT_TRUE(result.handled);
    EXPECT_EQ(result.return_value_json, expected);
    // The second dispatch must not collect the first dispatch's saved proxy.
    EXPECT_TRUE(result.callbacks.empty());
  }
}

TEST_F(FixtureContextTest, OversizedScriptAndAssetAreRejected) {
  WriteFixture(R"(
ctx.register('Asset', 'read', function() { return ctx.readAsset('config.json'); });
)");
  WriteAsset("config.json", R"({"feature":"on"})");
  FixtureContext context;
  FixtureContextLimits limits;
  limits.max_script_bytes = 8;
  EXPECT_FALSE(context.Initialize(fixture_directory_, limits));
  limits.max_script_bytes = kDefaultFixtureMaxScriptBytes;
  limits.max_asset_bytes = 4;
  ASSERT_TRUE(context.Initialize(fixture_directory_, limits));
  auto result = context.Dispatch("Asset", "read", "[]", {});
  ASSERT_TRUE(result.handled);
  EXPECT_TRUE(result.return_value_json.empty());
}

TEST_F(FixtureContextTest,
       MatchesEvaluatorDslAndDoesNotRepeatLifecycleActions) {
  WriteFixture(R"(
export default function(ctx) {
  ctx.setThreadStrategy({id: 1});
  ctx.setGlobalProps({theme: 'dark'});
  ctx.updateViewPort({width: 320});
  ctx.loadTemplate('https://example.com/template.js', 'assets/template.bin');
  ctx.reloadTemplate({});
  ctx.sendCustomEvent({});
  ctx.sendTouchEvent({});
  ctx.sendGlobalEvent({});
  ctx.sendEventAndroid({});
  ctx.dispatch('customAction', {});
  ctx.after(10, function() {
    ctx.dispatch('delayedAction', {});
    ctx.register('Delayed', 'call', function() { return 1; });
  });
  const names = Object.keys(ctx).sort();
  ctx.sharedData('dsl', names);
  ctx.register('Inspect', 'dsl', function() { return names; });
}
)");
  auto evaluated = EvaluateFixture(fixture_directory_);
  ASSERT_TRUE(evaluated.has_value());
  ASSERT_EQ(evaluated->shared_data.size(), 1u);
  ASSERT_EQ(evaluated->actions.size(), 11u);
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));
  auto result = context.Dispatch("Inspect", "dsl", "[]", {});
  ASSERT_TRUE(result.handled);
  EXPECT_EQ(result.return_value_json, evaluated->shared_data[0].value_json);
  EXPECT_EQ(result.return_value_json.find("loadTemplateBundle"),
            std::string::npos);
  EXPECT_FALSE(context.HasHandler("Delayed", "call"));
}

TEST_F(FixtureContextTest, KeepsContextsIsolatedAndReinitializesFromScratch) {
  WriteFixture(R"(
let closedOver = 0;
ctx.register('Counter', 'next', function() {
  this.n = (this.n || 0) + 1;
  return [++closedOver, this.n];
});
)");
  FixtureContext first;
  FixtureContext second;
  ASSERT_TRUE(first.Initialize(fixture_directory_));
  ASSERT_TRUE(second.Initialize(fixture_directory_));
  EXPECT_EQ(first.Dispatch("Counter", "next", "[]", {}).return_value_json,
            "[1,1]");
  EXPECT_EQ(first.Dispatch("Counter", "next", "[]", {}).return_value_json,
            "[2,2]");
  EXPECT_EQ(second.Dispatch("Counter", "next", "[]", {}).return_value_json,
            "[1,1]");
  first.Destroy();
  EXPECT_FALSE(first.initialized());
  EXPECT_FALSE(first.HasHandler("Counter", "next"));
  ASSERT_TRUE(first.Initialize(fixture_directory_));
  EXPECT_EQ(first.Dispatch("Counter", "next", "[]", {}).return_value_json,
            "[1,1]");
  WriteFixture("throw new Error('invalid replacement');");
  EXPECT_FALSE(first.Initialize(fixture_directory_));
  EXPECT_FALSE(first.HasHandler("Counter", "next"));
  WriteFixture("ctx.register('New', 'call', function() { return 7; });");
  ASSERT_TRUE(first.Initialize(fixture_directory_));
  EXPECT_EQ(first.Dispatch("New", "call", "[]", {}).return_value_json, "7");
}

TEST_F(FixtureContextTest, SeparatesModuleMethodPairsAndAllowsReplacement) {
  WriteFixture(R"(
ctx.register('a.b', 'c', function() { return 1; });
ctx.register('a', 'b.c', function() { return 2; });
ctx.register('Replace', 'call', function() {
  ctx.register('Replace', 'call', function() { return 4; });
  return 3;
});
ctx.register('Ignored', 'call', 42);
)");
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));
  EXPECT_EQ(context.Dispatch("a.b", "c", "[]", {}).return_value_json, "1");
  EXPECT_EQ(context.Dispatch("a", "b.c", "[]", {}).return_value_json, "2");
  EXPECT_EQ(context.Dispatch("Replace", "call", "[]", {}).return_value_json,
            "3");
  EXPECT_EQ(context.Dispatch("Replace", "call", "[]", {}).return_value_json,
            "4");
  EXPECT_FALSE(context.HasHandler("Ignored", "call"));
}

TEST_F(FixtureContextTest, RejectsInvalidArgumentsWithoutInvokingHandler) {
  WriteFixture(R"(
ctx.register('Args', 'call', function() {
  return ++this.n;
});
)");
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));
  for (const char* json : {"", "{", "{}", "null", "42"}) {
    EXPECT_FALSE(context.Dispatch("Args", "call", json, {}).handled) << json;
  }
  EXPECT_FALSE(context.Dispatch("Args", "call", "[]", {0}).handled);
  EXPECT_EQ(context.Dispatch("Args", "call", "[]", {}).return_value_json, "1");
}

TEST_F(FixtureContextTest, PreservesCallbackOrdinalsAndUndefinedResults) {
  WriteFixture(R"(
ctx.register('Callbacks', 'call', function(args, callbacks) {
  if (args[0] !== 42 || typeof args[1] !== 'function' ||
      args[2] !== 'key' || typeof args[3] !== 'function') {
    throw new Error('argument mapping failed');
  }
  callbacks[1]('second', 12.9);
  callbacks[0]();
  callbacks[1](undefined, -1);
});
ctx.register('Callbacks', 'null', function() { return null; });
)");
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));
  auto result =
      context.Dispatch("Callbacks", "call", "[42,null,\"key\",null]", {1, 3});
  ASSERT_TRUE(result.handled);
  EXPECT_TRUE(result.return_value_json.empty());
  ASSERT_EQ(result.callbacks.size(), 3u);
  ExpectCallback(result.callbacks[0], 1, "\"second\"", 12);
  ExpectCallback(result.callbacks[1], 0, "null", 0);
  ExpectCallback(result.callbacks[2], 1, "null", 0);
  EXPECT_EQ(context.Dispatch("Callbacks", "null", "[]", {}).return_value_json,
            "null");
}

TEST_F(FixtureContextTest, DiscardsPartialCallbacksAfterErrorsAndTimeouts) {
  WriteFixture(R"(
ctx.register('Fail', 'throw', function(args, callbacks) {
  callbacks[0](1);
  throw new Error('handler failed');
});
ctx.register('Fail', 'loop', function(args, callbacks) {
  callbacks[0](1);
  while (true) {}
});
ctx.register('Fail', 'serialize', function(args, callbacks) {
  callbacks[0](1);
  return {toJSON() { while (true) {} }};
});
ctx.register('Fail', 'callbackJson', function(args, callbacks) {
  callbacks[0]({toJSON() { while (true) {} }});
});
ctx.register('Ok', 'call', function() { return 42; });
)");
  FixtureContextLimits limits;
  limits.timeout_ms = 20;
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_, limits));
  for (const char* method : {"throw", "loop", "serialize", "callbackJson"}) {
    SCOPED_TRACE(method);
    const auto start = std::chrono::steady_clock::now();
    ExpectUnhandled(context.Dispatch("Fail", method, "[null]", {0}));
    EXPECT_LT(std::chrono::steady_clock::now() - start,
              std::chrono::seconds(10));
    auto recovered = context.Dispatch("Ok", "call", "[]", {});
    EXPECT_TRUE(recovered.handled);
    EXPECT_EQ(recovered.return_value_json, "42");
  }
}

TEST_F(FixtureContextTest,
       BoundsNativeCallbackOutputsEvenIfScriptCatchesErrors) {
  WriteFixture(R"(
ctx.register('Limit', 'count', function(args, callbacks) {
  callbacks[0](1);
  try { callbacks[0](2); } catch (e) {}
});
ctx.register('Limit', 'bytes', function(args, callbacks) {
  try { callbacks[0]('12345678'); } catch (e) {}
});
ctx.register('Limit', 'combined', function(args, callbacks) {
  callbacks[0]('123');
  return '456';
});
ctx.register('Limit', 'exact', function(args, callbacks) {
  callbacks[0]('123');
  return 456;
});
)");
  FixtureContextLimits limits;
  limits.max_callbacks = 1;
  limits.max_result_bytes = 8;
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_, limits));
  for (const char* method : {"count", "bytes", "combined"}) {
    SCOPED_TRACE(method);
    ExpectUnhandled(context.Dispatch("Limit", method, "[null]", {0}));
  }
  auto exact = context.Dispatch("Limit", "exact", "[null]", {0});
  EXPECT_TRUE(exact.handled);
  ASSERT_EQ(exact.callbacks.size(), 1u);
  EXPECT_EQ(exact.return_value_json, "456");
}

TEST_F(FixtureContextTest, BoundsInitializationAndClearsFailedRegistrations) {
  FixtureContext context;
  FixtureContextLimits limits;
  limits.timeout_ms = 20;
  WriteFixture("ctx.register('M','m',function() {}); while (true) {}");
  EXPECT_FALSE(context.Initialize(fixture_directory_, limits));
  EXPECT_FALSE(context.HasHandler("M", "m"));
  WriteFixture(R"(
ctx.register('M', 'm', function() {});
try { ctx.register('M', 'another', function() {}); } catch (e) {}
)");
  limits.max_handlers = 1;
  EXPECT_FALSE(context.Initialize(fixture_directory_, limits));
  EXPECT_FALSE(context.HasHandler("M", "m"));
  WriteFixture("ctx.register('M','m',function() { return 1; });");
  limits.timeout_ms = 0;
  EXPECT_FALSE(context.Initialize(fixture_directory_, limits));
  limits.timeout_ms = 20;
  limits.memory_limit_bytes = 0;
  EXPECT_FALSE(context.Initialize(fixture_directory_, limits));
  limits.memory_limit_bytes = kDefaultFixtureMemoryLimitBytes;
  limits.max_script_bytes = 0;
  EXPECT_FALSE(context.Initialize(fixture_directory_, limits));
}

TEST_F(FixtureContextTest,
       BoundsRetainedHandlerNamesAndReusesReplacementBudget) {
  WriteFixture(R"(
ctx.register('M', 'a', function() { return 1; });
ctx.register('M', 'a', function() { return 2; });
ctx.register('M', 'b', function() { return 3; });
)");
  FixtureContextLimits limits;
  limits.max_handler_name_bytes = 3;
  FixtureContext context;
  EXPECT_FALSE(context.Initialize(fixture_directory_, limits));
  EXPECT_FALSE(context.HasHandler("M", "a"));
  limits.max_handler_name_bytes = 4;
  ASSERT_TRUE(context.Initialize(fixture_directory_, limits));
  EXPECT_EQ(context.Dispatch("M", "a", "[]", {}).return_value_json, "2");
  EXPECT_EQ(context.Dispatch("M", "b", "[]", {}).return_value_json, "3");
  ASSERT_TRUE(context.Initialize(fixture_directory_, limits));
  EXPECT_EQ(context.Dispatch("M", "a", "[]", {}).return_value_json, "2");
}

TEST_F(FixtureContextTest, HandlesDispatchAfterRetainedHeapExhaustion) {
  WriteFixture(R"(
ctx.register('M', 'fill', function() {
  this.heap = [];
  try { while (true) this.heap.push({}); } catch (e) {}
});
ctx.register('M', 'callback', function(args, callbacks) {
  callbacks[0](1, 0);
});
)");
  FixtureContextLimits limits;
  limits.memory_limit_bytes = 1024 * 1024;
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_, limits));
  context.Dispatch("M", "fill", "[]", {});
  for (int i = 0; i < 10; ++i) {
    auto result = context.Dispatch("M", "callback", "[null]", {0});
    if (result.handled) {
      ASSERT_EQ(result.callbacks.size(), 1u);
      EXPECT_EQ(result.callbacks[0].value_json, "1");
    } else {
      EXPECT_TRUE(result.callbacks.empty());
      EXPECT_TRUE(result.return_value_json.empty());
    }
  }
  ASSERT_TRUE(context.Initialize(fixture_directory_, limits));
  EXPECT_TRUE(context.Dispatch("M", "callback", "[null]", {0}).handled);
}

TEST_F(FixtureContextTest, RejectsTinyMemoryBudgetWithoutAborting) {
  WriteFixture("ctx.register('M', 'm', function() {});");
  FixtureContextLimits limits;
  limits.memory_limit_bytes = 1;
  FixtureContext context;
  EXPECT_FALSE(context.Initialize(fixture_directory_, limits));
}

TEST_F(FixtureContextTest, RejectsExcessiveRuntimeAllocations) {
  WriteFixture(R"(
ctx.register('Memory', 'allocate', function() {
  const arrays = [];
  for (let i = 0; i < 128; ++i) arrays.push(new ArrayBuffer(65536));
  return arrays.length;
});
)");
  FixtureContextLimits limits;
  limits.memory_limit_bytes = 4 * 1024 * 1024;
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_, limits));
  EXPECT_FALSE(context.Dispatch("Memory", "allocate", "[]", {}).handled);
}

}  // namespace
}  // namespace replay
}  // namespace tasm
}  // namespace lynx
