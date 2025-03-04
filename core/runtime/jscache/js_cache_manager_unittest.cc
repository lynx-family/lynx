// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/js_cache_manager.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>

#include "base/include/fml/synchronization/waitable_event.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/runtime/jscache/js_cache_tracker.h"
#include "core/runtime/jscache/js_cache_tracker_unittest.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace cache {
namespace testing {

namespace {
using namespace lynx::tasm::report;
using namespace lynx::tasm::report::test;
constexpr static char k_source_url[] = "/app-service.js";
constexpr static char k_template_url[] = "template.js";

static tasm::report::EventTracker::EventBuilder s_current_builder;
void SetTestInterceptEvent(
    tasm::report::EventTracker::EventBuilder event_builder) {
  s_current_builder = std::move(event_builder);
}
}  // namespace

class JsCacheManagerTestBase : public JsCacheManager {
 public:
  JsCacheManagerTestBase(JSRuntimeType type) : JsCacheManager(type){};
  virtual ~JsCacheManagerTestBase() = default;
  JSRuntimeType RuntimeType() { return engine_type_; }
};

class QuickjsCacheManagerForTesting : public JsCacheManagerTestBase {
 public:
  static QuickjsCacheManagerForTesting &GetInstance() {
    static QuickjsCacheManagerForTesting instance;
    return instance;
  }

  QuickjsCacheManagerForTesting()
      : JsCacheManagerTestBase(JSRuntimeType::quickjs){};
  ~QuickjsCacheManagerForTesting() override = default;
};

class QuickjsCacheManagerForTestingCleanCache
    : public QuickjsCacheManagerForTesting {
 public:
  static QuickjsCacheManagerForTestingCleanCache &GetInstance() {
    static QuickjsCacheManagerForTestingCleanCache instance;
    return instance;
  }
  // test for 10 kb
  virtual size_t MaxCacheSize() { return 10 * 1024; }

  virtual bool WriteFile(const std::string &filename, uint8_t *out_buf,
                         size_t out_buf_len) {
    if (mock_write_success) {
      QuickjsCacheManagerForTesting::WriteFile(filename, out_buf, out_buf_len);
      return true;
    }
    return mock_write_success;
  }

  bool mock_write_success{true};
};

class QuickjsCacheManagerForTestingWithFailedWriteFile
    : public QuickjsCacheManagerForTesting {
 public:
  static QuickjsCacheManagerForTestingWithFailedWriteFile &GetInstance() {
    static QuickjsCacheManagerForTestingWithFailedWriteFile instance;
    return instance;
  }

  virtual bool WriteFile(const std::string &filename, uint8_t *out_buf,
                         size_t out_buf_len) {
    return false;
  }
};

class QuickjsCacheManagerForTestingWithEmptyGetCacheDir
    : public QuickjsCacheManagerForTesting {
 public:
  static QuickjsCacheManagerForTestingWithEmptyGetCacheDir &GetInstance() {
    static QuickjsCacheManagerForTestingWithEmptyGetCacheDir instance;
    return instance;
  }

  std::string GetCacheDir() { return ""; }
};

static std::string core_file = "['core'].toString()";
static std::string js_file = "['js_file'].toString()";
static std::string mock_cache = "['mock_cache'].toString()";

class TestingCacheGenerator : public CacheGenerator {
 public:
  TestingCacheGenerator(std::string file) : file(file) {}

  std::shared_ptr<Buffer> GenerateCache() override {
    return std::make_shared<StringBuffer>(file);
  }

  std::string file;
};

class TestingCacheGeneratorFailed : public CacheGenerator {
 public:
  TestingCacheGeneratorFailed(std::string file) : file(file) {}

  std::shared_ptr<Buffer> GenerateCache() override { return nullptr; }

  std::string file;
};

class JsCacheManagerTest : public ::testing::Test {
 public:
 protected:
  JsCacheManagerTest() {}

  ~JsCacheManagerTest() override{};

  static void SetUpTestSuite() {}
  static void TearDownTestSuite() {}

  void SetUp() override {
    // clean first.
    JsCacheManager::GetQuickjsInstance().ClearCacheDir();
    JsCacheManager::GetQuickjsInstance().cache_path_.clear();
    JsCacheManager::GetV8Instance().ClearCacheDir();
    JsCacheManager::GetV8Instance().cache_path_.clear();
    JsCacheTracker::s_test_intercept_event_ = &SetTestInterceptEvent;
  }

  void TearDown() override {
    // clean on finished.
    JsCacheManager::GetQuickjsInstance().ClearCacheDir();
    JsCacheManager::GetV8Instance().ClearCacheDir();
  }
};

TEST_F(JsCacheManagerTest, ConstructorQuickjs) {
  auto &instance = JsCacheManager::GetQuickjsInstance();
  EXPECT_EQ(JSRuntimeType::quickjs, instance.engine_type_);
}

TEST_F(JsCacheManagerTest, ConstructorV8) {
  auto &instance = JsCacheManager::GetV8Instance();
  EXPECT_EQ(JSRuntimeType::v8, instance.engine_type_);
}

TEST_F(JsCacheManagerTest, ConstructorJSC) {
  auto instance = JsCacheManagerTestBase(JSRuntimeType::jsc);
  EXPECT_EQ(JSRuntimeType::jsc, instance.RuntimeType());
}

// ClearCacheDir
TEST_F(JsCacheManagerTest, ClearCacheDir1) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  instance.ClearCacheDir();
}

// TryGetCache
TEST_F(JsCacheManagerTest, TryGetCacheGenerate) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  auto buffer = instance.TryGetCache(
      k_source_url, k_template_url, 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  EXPECT_FALSE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);

  buffer = instance.TryGetCache(
      k_source_url, k_template_url, 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  EXPECT_TRUE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::FILE, true, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheGetNull) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  auto buffer = instance.TryGetCache(
      "/nothing.js", k_template_url, 0, std::make_shared<StringBuffer>(""),
      std::make_unique<TestingCacheGenerator>(""));
  EXPECT_FALSE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, "/nothing.js",
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheGetNull2) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  auto buffer = instance.TryGetCache(
      k_source_url, "template2.js", 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  EXPECT_FALSE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheGenerateCore) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  const std::string source_url = "/lynx_core.js";
  auto buffer = instance.TryGetCache(
      source_url, "template3.js", 0, std::make_shared<StringBuffer>(core_file),
      std::make_unique<TestingCacheGenerator>(core_file));
  EXPECT_FALSE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, source_url, JsCacheType::NONE,
                          false, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheGetCore) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  auto buffer =
      instance.TryGetCache("/lynx_core.js", "template4.js", 0,
                           std::make_shared<StringBuffer>(core_file),
                           std::make_unique<TestingCacheGenerator>(core_file));
  EXPECT_TRUE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, "/lynx_core.js",
                          JsCacheType::MEMORY, true, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheDynamicComponentGenerate) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  const std::string source_url =
      "dynamic-component/http://testing/template.js//app-service.js";
  auto buffer = instance.TryGetCache(
      source_url, k_template_url, 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  EXPECT_FALSE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, source_url, JsCacheType::NONE,
                          false, true, true, 0ul, 0ul);

  buffer = instance.TryGetCache(
      source_url, "template2.js", 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  EXPECT_TRUE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, source_url, JsCacheType::FILE,
                          true, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheLynxDynamicJsGenerate) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  const std::string source_url = "/dynamic1.js";
  auto buffer = instance.TryGetCache(
      source_url, k_template_url, 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  EXPECT_FALSE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, source_url, JsCacheType::NONE,
                          false, true, true, 0ul, 0ul);

  buffer = instance.TryGetCache(
      source_url, "template2.js", 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  EXPECT_TRUE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, source_url, JsCacheType::FILE,
                          true, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheMultipleTasks) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  auto buffer1 = instance.TryGetCache(
      k_source_url, "template11.js", 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);

  auto buffer2 = instance.TryGetCache(
      k_source_url, "template12.js", 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);

  auto buffer3 = instance.TryGetCache(
      k_source_url, "template13.js", 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);

  auto buffer4 = instance.TryGetCache(
      k_source_url, "template14.js", 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);

  EXPECT_FALSE(buffer1);
  EXPECT_FALSE(buffer2);
  EXPECT_FALSE(buffer3);
  EXPECT_FALSE(buffer4);

  buffer1 = instance.TryGetCache(
      k_source_url, "template11.js", 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::FILE, true, true, true, 0ul, 0ul);

  buffer2 = instance.TryGetCache(
      k_source_url, "template12.js", 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::FILE, true, true, true, 0ul, 0ul);
  buffer3 = instance.TryGetCache(
      k_source_url, "template13.js", 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::FILE, true, true, true, 0ul, 0ul);
  buffer4 = instance.TryGetCache(
      k_source_url, "template14.js", 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::FILE, true, true, true, 0ul, 0ul);

  EXPECT_TRUE(buffer1);
  EXPECT_TRUE(buffer2);
  EXPECT_TRUE(buffer3);
  EXPECT_TRUE(buffer4);
}

TEST_F(JsCacheManagerTest, RequestCacheGenerationApp) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  instance.RequestCacheGeneration(
      k_source_url, "RequestCacheGeneration.js",
      std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file), false);

  auto buffer =
      instance.TryGetCache(k_source_url, "RequestCacheGeneration.js", 0,
                           std::make_shared<StringBuffer>(js_file),
                           std::make_unique<TestingCacheGenerator>(js_file));
  EXPECT_TRUE(buffer);
  EXPECT_EQ(std::string((char *)buffer->data()), std::string(js_file));

  instance.RequestCacheGeneration(
      k_source_url, "RequestCacheGeneration.js",
      std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(mock_cache), false);

  buffer =
      instance.TryGetCache(k_source_url, "RequestCacheGeneration.js", 0,
                           std::make_shared<StringBuffer>(js_file),
                           std::make_unique<TestingCacheGenerator>(js_file));
  EXPECT_TRUE(buffer);
  EXPECT_EQ(std::string((char *)buffer->data()), std::string(js_file));

  instance.RequestCacheGeneration(
      k_source_url, "RequestCacheGeneration.js",
      std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(mock_cache), true);

  buffer =
      instance.TryGetCache(k_source_url, "RequestCacheGeneration.js", 0,
                           std::make_shared<StringBuffer>(js_file),
                           std::make_unique<TestingCacheGenerator>(js_file));
  EXPECT_TRUE(buffer);
  EXPECT_EQ(std::string((char *)buffer->data()), std::string(mock_cache));
}

TEST_F(JsCacheManagerTest, RequestCacheGenerationCore) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();

  auto buffer =
      instance.TryGetCache("/lynx_core.js", "RequestCacheGeneration.js", 0,
                           std::make_shared<StringBuffer>(core_file),
                           std::make_unique<TestingCacheGenerator>(core_file));
  EXPECT_TRUE(buffer);
  EXPECT_EQ(std::string((char *)buffer->data()), std::string(core_file));

  instance.RequestCacheGeneration(
      "/lynx_core.js", "RequestCacheGeneration.js",
      std::make_shared<StringBuffer>(core_file),
      std::make_unique<TestingCacheGenerator>(mock_cache), true);

  buffer =
      instance.TryGetCache("/lynx_core.js", "RequestCacheGeneration.js", 0,
                           std::make_shared<StringBuffer>(core_file),
                           std::make_unique<TestingCacheGenerator>(mock_cache));
  EXPECT_TRUE(buffer);
  EXPECT_EQ(std::string((char *)buffer->data()), std::string(mock_cache));
}

// SaveCacheContentToStorage
TEST_F(JsCacheManagerTest, SaveCacheContentToStorageSuccess) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  JsFileIdentifier identifier;
  identifier.category = MetaData::CORE_JS;
  identifier.url = "/lynx_core.js";
  identifier.template_url = "template_SaveCacheContentToStorageSuccess";
  JsCacheErrorCode error_code;
  instance.SaveCacheContentToStorage(
      identifier,
      std::make_shared<StringBuffer>("SaveCacheContentToStorageSuccess"),
      "md5_SaveCacheContentToStorageSuccess", error_code);

  auto info = instance.GetMetaData().GetFileInfo(identifier);
  std::shared_ptr<Buffer> cache = instance.LoadCacheFromStorage(
      info.value(), "md5_SaveCacheContentToStorageSuccess");
  EXPECT_EQ(std::string((char *)cache->data()),
            std::string("SaveCacheContentToStorageSuccess"));
}

TEST_F(JsCacheManagerTest, SaveCacheContentToStorageSuccess2) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  JsFileIdentifier identifier;
  identifier.category = MetaData::PACKAGED;
  identifier.url = k_source_url;
  identifier.template_url = "template_SaveCacheContentToStorageSuccess2";

  JsCacheErrorCode error_code;
  instance.SaveCacheContentToStorage(
      identifier,
      std::make_shared<StringBuffer>("SaveCacheContentToStorageSuccess2"),
      "md5_SaveCacheContentToStorageSuccess2", error_code);

  auto info = instance.GetMetaData().GetFileInfo(identifier);
  std::shared_ptr<Buffer> cache = instance.LoadCacheFromStorage(
      info.value(), "md5_SaveCacheContentToStorageSuccess2");
  EXPECT_EQ(std::string((char *)cache->data()),
            std::string("SaveCacheContentToStorageSuccess2"));
}

TEST_F(JsCacheManagerTest, SaveCacheContentToStorageSuccess3) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  JsFileIdentifier identifier;
  identifier.category = MetaData::DYNAMIC;
  identifier.url = "/dynamic.js";
  identifier.template_url = "template_SaveCacheContentToStorageSuccess3";

  JsCacheErrorCode error_code;
  instance.SaveCacheContentToStorage(
      identifier,
      std::make_shared<StringBuffer>("SaveCacheContentToStorageSuccess3"),
      "md5_SaveCacheContentToStorageSuccess3", error_code);

  auto info = instance.GetMetaData().GetFileInfo(identifier);
  std::shared_ptr<Buffer> cache = instance.LoadCacheFromStorage(
      info.value(), "md5_SaveCacheContentToStorageSuccess3");
  EXPECT_EQ(std::string((char *)cache->data()),
            std::string("SaveCacheContentToStorageSuccess3"));
  std::cout << instance.GetMetaData().ToJson() << std::endl;
}

TEST_F(JsCacheManagerTest, SaveCacheContentToStorageFailure) {
  auto &instance =
      QuickjsCacheManagerForTestingWithFailedWriteFile::GetInstance();
  JsFileIdentifier identifier;
  identifier.category = MetaData::DYNAMIC;
  identifier.url = "/dynamic.js";
  identifier.template_url = "template_SaveCacheContentToStorageFailure";

  JsCacheErrorCode error_code;
  bool success = instance.SaveCacheContentToStorage(
      identifier,
      std::make_shared<StringBuffer>("SaveCacheContentToStorageFailure"),
      "md5_SaveCacheContentToStorageFailure", error_code);
  EXPECT_FALSE(success);

  auto info = instance.GetMetaData().GetFileInfo(identifier);
  std::shared_ptr<Buffer> cache = instance.LoadCacheFromStorage(
      info.value(), "md5_SaveCacheContentToStorageFailure");
  EXPECT_EQ(cache, nullptr);
}

// LoadCacheFromStorage
TEST_F(JsCacheManagerTest,
       LoadCacheFromStorageWithMetadataWithMetaInfoNotFoundFailure) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  JsFileIdentifier identifier;
  identifier.category = MetaData::PACKAGED;
  identifier.url = "/packaged.js";
  identifier.template_url = "template_HDFfhi348ht9lw4f";
  auto info = instance.GetMetaData().GetFileInfo(identifier);
  EXPECT_FALSE(info.has_value());
}

TEST_F(JsCacheManagerTest, LoadCacheFromStorageWithGetCacheDirFailure) {
  auto &instance =
      QuickjsCacheManagerForTestingWithEmptyGetCacheDir::GetInstance();
  JsFileIdentifier identifier;
  identifier.category = MetaData::CORE_JS;
  identifier.url = "/lynx_core.js";
  identifier.template_url = "template_IZWevh93awfhilsudgbf";
  instance.GetMetaData().UpdateFileInfo(identifier, "fsdh94w8hfoshf93lwhfg9",
                                        128);
  auto info = instance.GetMetaData().GetFileInfo(identifier);
  auto cache =
      instance.LoadCacheFromStorage(info.value(), "fsdh94w8hfoshf93lwhfg9");
  EXPECT_EQ(cache, nullptr);
}

TEST_F(JsCacheManagerTest, DifferentEngineVersion) {
  class DifferentEngineVersion : public QuickjsCacheManagerForTesting {
   public:
    static DifferentEngineVersion &GetInstance() {
      static DifferentEngineVersion instance;
      return instance;
    }

    std::string GetBytecodeGenerateEngineVersion() override {
      return "different_version";
    }
  };
  auto &meta1 = QuickjsCacheManagerForTesting::GetInstance().GetMetaData();
  EXPECT_EQ(meta1.GetBytecodeGenerateEngineVersion(),
            std::to_string(LEPUS_GetPrimjsVersion()));
  auto buffer = QuickjsCacheManagerForTesting::GetInstance().TryGetCache(
      k_source_url, k_template_url, 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  buffer = QuickjsCacheManagerForTesting::GetInstance().TryGetCache(
      k_source_url, k_template_url, 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));
  EXPECT_NE(buffer, nullptr);

  buffer = DifferentEngineVersion::GetInstance().TryGetCache(
      k_source_url, k_template_url, 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));

  EXPECT_EQ(buffer, nullptr);
  auto &meta2 = DifferentEngineVersion::GetInstance().GetMetaData();
  EXPECT_EQ(meta2.GetBytecodeGenerateEngineVersion(), "different_version");
}

// ClearCacheDir
TEST_F(JsCacheManagerTest, ClearCacheDir2) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  auto buffer = instance.TryGetCache(
      k_source_url, k_template_url, 0, std::make_shared<StringBuffer>(js_file),
      std::make_unique<TestingCacheGenerator>(js_file));

  {
    std::ifstream file(instance.GetCacheDir() + "/meta.json");
    EXPECT_EQ(file.is_open(), true);
  }
  instance.ClearCacheDir();
  {
    std::ifstream file(instance.GetCacheDir() + "/meta.json");
    EXPECT_EQ(file.is_open(), false);
  }
  EXPECT_EQ(rmdir(instance.GetCacheDir().c_str()), 0);
}

TEST_F(JsCacheManagerTest, CacheDirName) {
  auto &quickjs_instance = QuickjsCacheManagerForTesting::GetInstance();
  EXPECT_EQ("quickjs_cache", quickjs_instance.CacheDirName());

  auto v8_instance = JsCacheManagerTestBase(JSRuntimeType::v8);
  EXPECT_EQ("v8_cache", v8_instance.CacheDirName());

  auto jsc_instance = JsCacheManagerTestBase(JSRuntimeType::jsc);
  ASSERT_DEATH(jsc_instance.CacheDirName(), ".*");
}

TEST_F(JsCacheManagerTest, GetPlatformCacheDir) {
  auto &quickjs_instance = JsCacheManager::GetQuickjsInstance();
  EXPECT_EQ("./", quickjs_instance.GetPlatformCacheDir());
}

TEST_F(JsCacheManagerTest, RequestCacheGenerationSuccess) {
  auto &quickjs_instance = JsCacheManager::GetQuickjsInstance();
  const std::string source_url = "/generation1.js";
  const std::shared_ptr<const Buffer> buffer =
      std::make_shared<StringBuffer>(js_file);

  const std::string js_content =
      "var action = 'request_cache_generation_success';";
  quickjs_instance.RequestCacheGeneration(
      source_url, k_template_url, buffer,
      std::make_unique<TestingCacheGenerator>(js_content), true);
  auto cache = quickjs_instance.TryGetCache(
      source_url, k_template_url, 0, buffer,
      std::make_unique<TestingCacheGenerator>(js_content));
  EXPECT_TRUE(cache);
  fml::AutoResetWaitableEvent latch;
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [&latch, &buffer, &cache, &source_url] {
        MoveOnlyEvent event;
        if (s_current_builder) {
          s_current_builder(event);
        }
        CheckBytecodeGenerateEvent(
            JSRuntimeType::quickjs, source_url, k_template_url, true,
            buffer->size(), cache->size(), true, JsCacheErrorCode::NO_ERROR,
            std::move(event));
        latch.Signal();
      },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
  EXPECT_FALSE(latch.WaitWithTimeout(fml::TimeDelta::FromSeconds(5)));
}

TEST_F(JsCacheManagerTest, RequestCacheGenerationWriteFailed) {
  auto &quickjs_instance =
      QuickjsCacheManagerForTestingWithFailedWriteFile::GetInstance();
  const std::string source_url = "/generation2.js";
  const std::string js_content =
      "var action = 'request_cache_generation_write_failed';";
  const std::shared_ptr<const Buffer> buffer =
      std::make_shared<StringBuffer>(js_content);
  quickjs_instance.RequestCacheGeneration(
      source_url, k_template_url, buffer,
      std::make_unique<TestingCacheGenerator>(js_content), true);
  auto cache = quickjs_instance.TryGetCache(
      source_url, k_template_url, 0, buffer,
      std::make_unique<TestingCacheGenerator>(js_content));
  EXPECT_FALSE(cache);
  fml::AutoResetWaitableEvent latch;
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [&latch, &buffer, &source_url] {
        MoveOnlyEvent event;
        if (s_current_builder) {
          s_current_builder(event);
        }
        CheckBytecodeGenerateEvent(
            JSRuntimeType::quickjs, source_url, k_template_url, true,
            buffer->size(), 0ul, false, JsCacheErrorCode::META_FILE_WRITE_ERROR,
            std::move(event));
        latch.Signal();
      },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
  EXPECT_FALSE(latch.WaitWithTimeout(fml::TimeDelta::FromSeconds(5)));
}

TEST_F(JsCacheManagerTest, RequestCacheGenerationGenerateFailed) {
  auto &quickjs_instance =
      QuickjsCacheManagerForTestingWithFailedWriteFile::GetInstance();
  const std::string source_url = "/generation3.js";
  const std::string js_content =
      "var action = 'request_cache_generation_generate_failed';";
  const std::shared_ptr<const Buffer> buffer =
      std::make_shared<StringBuffer>(js_content);
  quickjs_instance.RequestCacheGeneration(
      source_url, k_template_url, buffer,
      std::make_unique<TestingCacheGeneratorFailed>(js_content), true);
  auto cache = quickjs_instance.TryGetCache(
      source_url, k_template_url, 0, buffer,
      std::make_unique<TestingCacheGeneratorFailed>(js_content));
  EXPECT_FALSE(cache);
  fml::AutoResetWaitableEvent latch;
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [&latch, &source_url] {
        MoveOnlyEvent event;
        if (s_current_builder) {
          s_current_builder(event);
        }
        CheckBytecodeGenerateEvent(
            JSRuntimeType::quickjs, source_url, k_template_url, false, 0ul, 0ul,
            false, JsCacheErrorCode::RUNTIME_GENERATE_FAILED, std::move(event));
        latch.Signal();
      },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
  EXPECT_FALSE(latch.WaitWithTimeout(fml::TimeDelta::FromSeconds(5)));
}

TEST_F(JsCacheManagerTest, ClearInvalidCache) {
  auto &quickjs_instance =
      QuickjsCacheManagerForTestingCleanCache::GetInstance();
  const std::string source_url_prefix = "/clean_invalid_cache";
  constexpr int count = 11;
  for (int i = 0; i < count; i++) {
    std::string js_content(1024, 'A' + i);
    const std::shared_ptr<const Buffer> buffer =
        std::make_shared<StringBuffer>(js_content);
    quickjs_instance.RequestCacheGeneration(
        k_source_url, source_url_prefix + std::to_string(i) + ".js", buffer,
        std::make_unique<TestingCacheGenerator>(js_content), true);
    // to generate different timestamp.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  auto before_clean_file_info =
      quickjs_instance.GetMetaData().GetAllCacheFileInfo();
  EXPECT_EQ(before_clean_file_info.size(), count);
  quickjs_instance.ClearInvalidCache();
  MoveOnlyEvent event;
  if (s_current_builder) {
    s_current_builder(event);
  }
  CheckCleanUpEvent(JSRuntimeType::quickjs, JsCacheErrorCode::NO_ERROR,
                    std::move(event));
  // Check if removed file is oldest.
  auto after_clean_file_info =
      quickjs_instance.GetMetaData().GetAllCacheFileInfo();
  EXPECT_EQ(after_clean_file_info.size(), count - 2);
  std::vector<CacheFileInfo> moved_cache_file_info;
  for (auto &before_it : before_clean_file_info) {
    bool contained = false;
    for (auto &after_it : after_clean_file_info) {
      if (before_it.identifier == after_it.identifier &&
          before_it.cache_size == after_it.cache_size &&
          before_it.last_accessed == after_it.last_accessed &&
          before_it.md5 == after_it.md5) {
        contained = true;
        break;
      }
    }
    if (!contained) {
      moved_cache_file_info.push_back(before_it);
    }
  }
  // increase order.
  auto compare_func = [](const CacheFileInfo &l, const CacheFileInfo &r) {
    return l.last_accessed < r.last_accessed;
  };
  std::sort(moved_cache_file_info.begin(), moved_cache_file_info.end(),
            compare_func);
  std::sort(after_clean_file_info.begin(), after_clean_file_info.end(),
            compare_func);

  // The retained ones are newer than the deleted ones
  EXPECT_LE(moved_cache_file_info.back().last_accessed,
            after_clean_file_info.cbegin()->last_accessed);
}

TEST_F(JsCacheManagerTest, ClearInvalidCacheError) {
  auto &quickjs_instance =
      QuickjsCacheManagerForTestingCleanCache::GetInstance();
  quickjs_instance.mock_write_success = false;
  const std::string source_url_prefix = "/clean_invalid_cache2";
  constexpr int count = 11;
  for (int i = 0; i < count; i++) {
    std::string js_content(1024, 'A' + i);
    const std::shared_ptr<const Buffer> buffer =
        std::make_shared<StringBuffer>(js_content);
    quickjs_instance.RequestCacheGeneration(
        k_source_url, source_url_prefix + std::to_string(i) + ".js", buffer,
        std::make_unique<TestingCacheGenerator>(js_content), true);
  }
  quickjs_instance.ClearInvalidCache();
  MoveOnlyEvent event;
  if (s_current_builder) {
    s_current_builder(event);
  }
  CheckCleanUpEvent(JSRuntimeType::quickjs,
                    JsCacheErrorCode::META_FILE_WRITE_ERROR, std::move(event));
  // reset flag.
  quickjs_instance.mock_write_success = true;
}

TEST_F(JsCacheManagerTest, ClearCache) {
  auto &quickjs_instance =
      QuickjsCacheManagerForTestingCleanCache::GetInstance();
  std::string js_content = "var action = 1;";
  const std::string source_url = "/app-service.js";
  const std::string clear_url = std::string("/0/") + k_template_url;
  for (int i = 0; i < 10; i++) {
    const std::shared_ptr<const Buffer> buffer =
        std::make_shared<StringBuffer>(js_content);
    quickjs_instance.RequestCacheGeneration(
        source_url, std::string("/") + std::to_string(i) + "/" + k_template_url,
        buffer, std::make_unique<TestingCacheGenerator>(js_content), true);
    if (i == 0) {
      quickjs_instance.RequestCacheGeneration(
          clear_url, k_template_url, buffer,
          std::make_unique<TestingCacheGenerator>(js_content), true);
    }
  }
  auto vec = quickjs_instance.GetMetaData().GetAllCacheFileInfo(clear_url);
  EXPECT_EQ(vec.size(), 2);
  LOGE("LYbB321:" << quickjs_instance.GetMetaData().ToJson());
  quickjs_instance.ClearCache(clear_url);
  vec = quickjs_instance.GetMetaData().GetAllCacheFileInfo(clear_url);
  EXPECT_EQ(vec.size(), 0);
  MoveOnlyEvent event;
  if (s_current_builder) {
    s_current_builder(event);
  }
  CheckCleanUpEvent(JSRuntimeType::quickjs, JsCacheErrorCode::NO_ERROR,
                    std::move(event));
}

}  // namespace testing
}  // namespace cache
}  // namespace piper
}  // namespace lynx
