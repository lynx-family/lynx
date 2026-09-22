// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/fixture_context.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace runtime {
namespace js {
namespace {

std::string SanitizeTestName(std::string name) {
  for (char& character : name) {
    const auto value = static_cast<unsigned char>(character);
    if (!std::isalnum(value) && character != '_' && character != '-') {
      character = '_';
    }
  }
  return name;
}

std::string TempRoot() {
  const char* tmpdir = std::getenv("TMPDIR");
  if (tmpdir != nullptr && *tmpdir != '\0') {
    return tmpdir;
  }
  return "/tmp";
}

// Creates a unique fixture directory. mkdtemp replaces the trailing "XXXXXX"
// in place, so the template must stay a mutable buffer. Returns an empty string
// on failure. POSIX-only: the replay unittests are not built on Windows.
std::string CreateFixtureDirectory(const char* test_name) {
  std::string tmpl = TempRoot() + "/lynx_fixture_context_" +
                     SanitizeTestName(test_name) + "_XXXXXX";
  if (::mkdtemp(tmpl.data()) == nullptr) {
    return {};
  }
  return tmpl;
}

// Best-effort recursive removal used by TearDown.
void RemoveTree(const std::string& path) {
  if (DIR* dir = ::opendir(path.c_str())) {
    while (struct dirent* entry = ::readdir(dir)) {
      const std::string name = entry->d_name;
      if (name == "." || name == "..") {
        continue;
      }
      const std::string child = path + "/" + name;
      struct stat child_stat {};
      if (::lstat(child.c_str(), &child_stat) == 0 &&
          S_ISDIR(child_stat.st_mode)) {
        RemoveTree(child);
      } else {
        std::remove(child.c_str());
      }
    }
    ::closedir(dir);
  }
  ::rmdir(path.c_str());
}

class FixtureContextTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const auto* test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    fixture_directory_ = CreateFixtureDirectory(test_info->name());
    ASSERT_FALSE(fixture_directory_.empty());
    ASSERT_EQ(::mkdir((fixture_directory_ + "/assets").c_str(), 0755), 0);
  }

  void TearDown() override {
    if (!fixture_directory_.empty()) {
      RemoveTree(fixture_directory_);
    }
  }

  void WriteFixture(const std::string& source) {
    WriteFile(fixture_directory_ + "/fixture.js", source);
  }

  // Asset paths are relative to the fixture assets directory and must be flat
  // file names (the tests below do not use nested asset directories).
  void WriteAsset(const std::string& path, const std::string& content) {
    WriteFile(fixture_directory_ + "/assets/" + path, content);
  }

  std::string fixture_directory_;

 private:
  static void WriteFile(const std::string& path, const std::string& content) {
    std::ofstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    stream << content;
    ASSERT_TRUE(stream.good());
  }
};

TEST_F(FixtureContextTest, InitializeFailsWhenScriptMissing) {
  FixtureContext context;
  EXPECT_FALSE(context.Initialize(fixture_directory_));
  EXPECT_FALSE(context.initialized());
}

TEST_F(FixtureContextTest, DispatchReturnsHandlerResultUsingRealArguments) {
  WriteFixture(R"(
export default function(ctx) {
  ctx.register("NetworkModule", "request", function(args, callbacks) {
    return {"status": args[0].code, "path": args[1]};
  });
}
)");

  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));
  ASSERT_TRUE(context.HasHandler("NetworkModule", "request"));

  FixtureDispatchResult result = context.Dispatch(
      "NetworkModule", "request", R"([{"code":200},"/v1/data"])", {});

  EXPECT_TRUE(result.handled);
  EXPECT_TRUE(result.callbacks.empty());

  rapidjson::Document doc;
  doc.Parse(result.return_value_json.c_str());
  ASSERT_FALSE(doc.HasParseError());
  ASSERT_TRUE(doc.IsObject());
  ASSERT_TRUE(doc["status"].IsInt());
  EXPECT_EQ(doc["status"].GetInt(), 200);
  ASSERT_TRUE(doc["path"].IsString());
  EXPECT_STREQ(doc["path"].GetString(), "/v1/data");
}

TEST_F(FixtureContextTest, UnregisteredHandlerIsNotHandled) {
  WriteFixture(R"(
export default function(ctx) {
  ctx.register("KnownModule", "known", function() { return null; });
}
)");

  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));

  EXPECT_FALSE(context.HasHandler("UnknownModule", "method"));
  FixtureDispatchResult result =
      context.Dispatch("UnknownModule", "method", "[]", {});
  EXPECT_FALSE(result.handled);
  EXPECT_TRUE(result.return_value_json.empty());
  EXPECT_TRUE(result.callbacks.empty());
}

TEST_F(FixtureContextTest, ReplaysCallbacksInOrderWithRecordedDelays) {
  WriteFixture(R"(
export default function(ctx) {
  ctx.register("EventModule", "listen", function(args, callbacks) {
    callbacks[0]({"first": true});
    callbacks[1]({"second": 2}, 50);
  });
}
)");

  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));

  // Two function arguments at positions 0 and 1 in the original JSB call.
  FixtureDispatchResult result =
      context.Dispatch("EventModule", "listen", "[null,null]", {0, 1});

  ASSERT_TRUE(result.handled);
  ASSERT_EQ(result.callbacks.size(), 2u);

  EXPECT_EQ(result.callbacks[0].index, 0u);
  EXPECT_EQ(result.callbacks[0].delay_ms, 0);
  rapidjson::Document first;
  first.Parse(result.callbacks[0].value_json.c_str());
  ASSERT_TRUE(first.IsObject());
  ASSERT_TRUE(first["first"].IsBool());
  EXPECT_TRUE(first["first"].GetBool());

  EXPECT_EQ(result.callbacks[1].index, 1u);
  EXPECT_EQ(result.callbacks[1].delay_ms, 50);
  rapidjson::Document second;
  second.Parse(result.callbacks[1].value_json.c_str());
  ASSERT_TRUE(second.IsObject());
  ASSERT_TRUE(second["second"].IsInt());
  EXPECT_EQ(second["second"].GetInt(), 2);
}

TEST_F(FixtureContextTest, RestoresFunctionPlaceholdersForCallbackArguments) {
  WriteFixture(R"(
export default function(ctx) {
  ctx.register("TypeModule", "inspect", function(args, callbacks) {
    return {"isFn": typeof args[1] === "function", "value": args[0]};
  });
}
)");

  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));

  // Position 1 held a function in the original call; the JSON transport carries
  // it as null, and Dispatch must restore a function placeholder so the
  // fixture's `typeof` check still sees a function.
  FixtureDispatchResult result =
      context.Dispatch("TypeModule", "inspect", "[42,null]", {1});

  ASSERT_TRUE(result.handled);
  rapidjson::Document doc;
  doc.Parse(result.return_value_json.c_str());
  ASSERT_TRUE(doc.IsObject());
  ASSERT_TRUE(doc["isFn"].IsBool());
  EXPECT_TRUE(doc["isFn"].GetBool());
  ASSERT_TRUE(doc["value"].IsInt());
  EXPECT_EQ(doc["value"].GetInt(), 42);
}

TEST_F(FixtureContextTest, TracksInvocationOrdinalAcrossDispatches) {
  WriteFixture(R"(
export default function(ctx) {
  ctx.register("CounterModule", "next", function(args, callbacks) {
    return {"n": this.n};
  });
}
)");

  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));

  // `this.n` is incremented before each handler invocation, so the first call
  // observes 1 and the second observes 2.
  FixtureDispatchResult first =
      context.Dispatch("CounterModule", "next", "[]", {});
  FixtureDispatchResult second =
      context.Dispatch("CounterModule", "next", "[]", {});

  rapidjson::Document first_doc;
  first_doc.Parse(first.return_value_json.c_str());
  ASSERT_TRUE(first_doc.IsObject());
  ASSERT_TRUE(first_doc["n"].IsInt());
  EXPECT_EQ(first_doc["n"].GetInt(), 1);

  rapidjson::Document second_doc;
  second_doc.Parse(second.return_value_json.c_str());
  ASSERT_TRUE(second_doc.IsObject());
  ASSERT_TRUE(second_doc["n"].IsInt());
  EXPECT_EQ(second_doc["n"].GetInt(), 2);
}

TEST_F(FixtureContextTest, ReadsJsonAssetsAndRejectsUnsafePaths) {
  WriteAsset("config.json", R"({"feature":"on"})");
  WriteFixture(R"(
export default function(ctx) {
  ctx.register("AssetModule", "read", function(args, callbacks) {
    return ctx.readAsset("assets/config.json");
  });
  ctx.register("AssetModule", "escape", function(args, callbacks) {
    return {"escaped": ctx.readAsset("../secret.json") === undefined};
  });
}
)");

  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));

  FixtureDispatchResult read =
      context.Dispatch("AssetModule", "read", "[]", {});
  ASSERT_TRUE(read.handled);
  rapidjson::Document read_doc;
  read_doc.Parse(read.return_value_json.c_str());
  ASSERT_TRUE(read_doc.IsObject());
  ASSERT_TRUE(read_doc["feature"].IsString());
  EXPECT_STREQ(read_doc["feature"].GetString(), "on");

  FixtureDispatchResult escape =
      context.Dispatch("AssetModule", "escape", "[]", {});
  ASSERT_TRUE(escape.handled);
  rapidjson::Document escape_doc;
  escape_doc.Parse(escape.return_value_json.c_str());
  ASSERT_TRUE(escape_doc.IsObject());
  ASSERT_TRUE(escape_doc["escaped"].IsBool());
  EXPECT_TRUE(escape_doc["escaped"].GetBool());
}

TEST_F(FixtureContextTest, DispatchOnUninitializedContextIsNotHandled) {
  FixtureContext context;
  FixtureDispatchResult result = context.Dispatch("M", "m", "[]", {});
  EXPECT_FALSE(result.handled);
}

TEST_F(FixtureContextTest, NonFiniteCallbackDelayIsNormalized) {
  WriteFixture(R"(
export default function(ctx) {
  ctx.register("DelayModule", "go", function(args, callbacks) {
    callbacks[0]({"seq": 1}, 0 / 0);
    callbacks[0]({"seq": 2}, Infinity);
    callbacks[1]({"seq": 3}, 1e308);
  });
}
)");

  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));

  FixtureDispatchResult result =
      context.Dispatch("DelayModule", "go", "[null,null]", {0, 1});

  ASSERT_TRUE(result.handled);
  ASSERT_EQ(result.callbacks.size(), 3u);
  // NaN and Infinity must normalize to 0 instead of hitting the int64_t cast
  // (UB); a huge finite delay saturates.
  EXPECT_EQ(result.callbacks[0].delay_ms, 0);
  EXPECT_EQ(result.callbacks[1].delay_ms, 0);
  EXPECT_EQ(result.callbacks[2].delay_ms, std::numeric_limits<int64_t>::max());
}

TEST_F(FixtureContextTest, StaleCallbackProxyFromEarlierDispatchIsHarmless) {
  WriteFixture(R"(
export default function(ctx) {
  ctx.register("StaleModule", "call", function(args, callbacks) {
    if (this.n === 1) {
      this.saved = callbacks[0];
      return {"stored": true};
    }
    this.saved({"late": true});
    return {"fired": true};
  });
}
)");

  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_));

  FixtureDispatchResult first =
      context.Dispatch("StaleModule", "call", "[null]", {0});
  ASSERT_TRUE(first.handled);
  EXPECT_TRUE(first.callbacks.empty());

  // The handler invokes the stashed proxy from the *first* dispatch during the
  // second dispatch. The proxy must own its result sink, so the deferred call
  // is dropped instead of writing into the destroyed first result.
  FixtureDispatchResult second =
      context.Dispatch("StaleModule", "call", "[null]", {0});
  ASSERT_TRUE(second.handled);
  EXPECT_TRUE(second.callbacks.empty());
  rapidjson::Document doc;
  doc.Parse(second.return_value_json.c_str());
  ASSERT_TRUE(doc.IsObject());
  EXPECT_TRUE(doc["fired"].GetBool());
}

TEST_F(FixtureContextTest, InfiniteLoopHandlerIsInterruptedByTimeout) {
  WriteFixture(R"(
export default function(ctx) {
  ctx.register("HangModule", "hang", function(args, callbacks) {
    while (true) { }
  });
  ctx.register("OkModule", "ok", function(args, callbacks) {
    return {"fine": true};
  });
}
)");

  FixtureContextLimits limits;
  limits.timeout_ms = 100;
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_, limits));

  const auto start = std::chrono::steady_clock::now();
  FixtureDispatchResult result =
      context.Dispatch("HangModule", "hang", "[]", {});
  const auto elapsed = std::chrono::steady_clock::now() - start;

  // The handler is reported as unmatched so the caller falls back to invoking
  // the original callbacks with null, and the dispatch must not hang.
  EXPECT_FALSE(result.handled);
  EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count(),
            10);

  // The context stays usable after an interrupted dispatch.
  FixtureDispatchResult ok = context.Dispatch("OkModule", "ok", "[]", {});
  ASSERT_TRUE(ok.handled);
  rapidjson::Document doc;
  doc.Parse(ok.return_value_json.c_str());
  ASSERT_TRUE(doc.IsObject());
  EXPECT_TRUE(doc["fine"].GetBool());
}

TEST_F(FixtureContextTest, OversizedScriptAndAssetAreRejected) {
  // A script budget smaller than the real fixture.js must fail Initialize.
  WriteFixture(R"(
export default function(ctx) {
  ctx.register("AssetModule", "read", function(args, callbacks) {
    return {"isUndefined": ctx.readAsset("assets/config.json") === undefined};
  });
}
)");
  WriteAsset("config.json", R"({"feature":"on"})");

  FixtureContextLimits script_limits;
  script_limits.max_script_bytes = 8;
  FixtureContext oversized_script;
  EXPECT_FALSE(oversized_script.Initialize(fixture_directory_, script_limits));

  // An asset budget smaller than the file makes readAsset return undefined.
  FixtureContextLimits asset_limits;
  asset_limits.max_asset_bytes = 4;
  FixtureContext context;
  ASSERT_TRUE(context.Initialize(fixture_directory_, asset_limits));

  FixtureDispatchResult result =
      context.Dispatch("AssetModule", "read", "[]", {});
  ASSERT_TRUE(result.handled);
  rapidjson::Document doc;
  doc.Parse(result.return_value_json.c_str());
  ASSERT_TRUE(doc.IsObject());
  EXPECT_TRUE(doc["isUndefined"].GetBool());
}

}  // namespace
}  // namespace js
}  // namespace runtime
}  // namespace lynx
