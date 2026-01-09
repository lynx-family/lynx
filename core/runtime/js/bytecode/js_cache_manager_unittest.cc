// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bytecode/js_cache_manager.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>

#include "base/include/fml/synchronization/waitable_event.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/runtime/js/bytecode/cache_generator.h"
#include "core/runtime/js/bytecode/js_cache_tracker.h"
#include "core/runtime/js/bytecode/js_cache_tracker_unittest.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif

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

void WaitNormalTaskFinish() {
  fml::AutoResetWaitableEvent latch;
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [&latch]() { latch.Signal(); },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
  EXPECT_FALSE(latch.WaitWithTimeout(fml::TimeDelta::FromSeconds(10)));
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
  TestingCacheGenerator(const std::string &source_url,
                        std::shared_ptr<const Buffer> src_buffer,
                        std::string file)
      : CacheGenerator(source_url, std::move(src_buffer)), file(file) {}

  std::shared_ptr<Buffer> GenerateCache() override {
    return std::make_shared<StringBuffer>(file);
  }

  std::string file;
};

class TestingCacheGeneratorFailed : public CacheGenerator {
 public:
  TestingCacheGeneratorFailed(const std::string &source_url,
                              std::shared_ptr<const Buffer> src_buffer,
                              std::string file)
      : CacheGenerator(source_url, std::move(src_buffer)), file(file) {}

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
    JsCacheManager::GetQuickjsInstance().task_set_.clear();
    JsCacheManager::GetV8Instance().ClearCacheDir();
    JsCacheManager::GetV8Instance().cache_path_.clear();
    JsCacheManager::GetV8Instance().task_set_.clear();
    QuickjsCacheManagerForTesting::GetInstance().ClearCacheDir();
    QuickjsCacheManagerForTesting::GetInstance().cache_path_.clear();
    QuickjsCacheManagerForTesting::GetInstance().task_set_.clear();
    QuickjsCacheManagerForTestingCleanCache::GetInstance().ClearCacheDir();
    QuickjsCacheManagerForTestingCleanCache::GetInstance().cache_path_.clear();
    QuickjsCacheManagerForTestingCleanCache::GetInstance().task_set_.clear();
    QuickjsCacheManagerForTestingWithFailedWriteFile::GetInstance()
        .ClearCacheDir();
    QuickjsCacheManagerForTestingWithFailedWriteFile::GetInstance()
        .cache_path_.clear();
    QuickjsCacheManagerForTestingWithFailedWriteFile::GetInstance()
        .task_set_.clear();
    ;
    QuickjsCacheManagerForTestingWithEmptyGetCacheDir::GetInstance()
        .ClearCacheDir();
    QuickjsCacheManagerForTestingWithEmptyGetCacheDir::GetInstance()
        .cache_path_.clear();
    QuickjsCacheManagerForTestingWithEmptyGetCacheDir::GetInstance()
        .task_set_.clear();
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
  auto js_file_buffer = std::make_shared<StringBuffer>(js_file);
  auto buffer =
      instance.TryGetCache(k_source_url, k_template_url, 0,
                           std::make_unique<TestingCacheGenerator>(
                               k_source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  EXPECT_FALSE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);

  buffer = instance.TryGetCache(k_source_url, k_template_url, 0,
                                std::make_unique<TestingCacheGenerator>(
                                    k_source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  EXPECT_TRUE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::FILE, true, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheGetNull) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  auto buffer = instance.TryGetCache(
      "/nothing.js", k_template_url, 0,
      std::make_unique<TestingCacheGenerator>(
          "/nothing.js", std::make_shared<StringBuffer>(""), ""));
  WaitNormalTaskFinish();
  EXPECT_FALSE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, "/nothing.js",
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheGetNull2) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  auto js_file_buffer = std::make_shared<StringBuffer>(js_file);
  auto buffer =
      instance.TryGetCache(k_source_url, "template2.js", 0,
                           std::make_unique<TestingCacheGenerator>(
                               k_source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  EXPECT_FALSE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheGenerateCore) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  const std::string source_url = "/lynx_core.js";
  auto core_file_buffer = std::make_shared<StringBuffer>(core_file);
  auto buffer =
      instance.TryGetCache(source_url, "template3.js", 0,
                           std::make_unique<TestingCacheGenerator>(
                               source_url, core_file_buffer, core_file));
  WaitNormalTaskFinish();
  EXPECT_FALSE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, source_url, JsCacheType::NONE,
                          false, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheGetCore) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  const std::string source_url =
      "/lynx_core.js";  // Added to match usage in TestingCacheGenerator
  auto core_file_buffer = std::make_shared<StringBuffer>(core_file);
  auto buffer =
      instance.TryGetCache(source_url, "template4.js", 0,
                           std::make_unique<TestingCacheGenerator>(
                               source_url, core_file_buffer, core_file));
  WaitNormalTaskFinish();
  EXPECT_TRUE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, source_url,
                          JsCacheType::MEMORY, true, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheDynamicComponentGenerate) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  const std::string source_url =
      "dynamic-component/http://testing/template.js//app-service.js";
  auto js_file_buffer = std::make_shared<StringBuffer>(js_file);
  auto buffer = instance.TryGetCache(source_url, k_template_url, 0,
                                     std::make_unique<TestingCacheGenerator>(
                                         source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  EXPECT_FALSE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, source_url, JsCacheType::NONE,
                          false, true, true, 0ul, 0ul);

  buffer = instance.TryGetCache(source_url, "template2.js", 0,
                                std::make_unique<TestingCacheGenerator>(
                                    source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  EXPECT_TRUE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, source_url, JsCacheType::FILE,
                          true, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheLynxDynamicJsGenerate) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  const std::string source_url = "http://dynamic1.js";
  auto js_file_buffer = std::make_shared<StringBuffer>(js_file);
  auto buffer = instance.TryGetCache(source_url, k_template_url, 0,
                                     std::make_unique<TestingCacheGenerator>(
                                         source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  EXPECT_FALSE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, source_url, JsCacheType::NONE,
                          false, true, true, 0ul, 0ul);

  buffer = instance.TryGetCache(source_url, "template2.js", 0,
                                std::make_unique<TestingCacheGenerator>(
                                    source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  EXPECT_TRUE(buffer);
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, source_url, JsCacheType::FILE,
                          true, true, true, 0ul, 0ul);
}

TEST_F(JsCacheManagerTest, TryGetCacheMultipleTasks) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  auto js_file_buffer = std::make_shared<StringBuffer>(js_file);
  auto buffer1 =
      instance.TryGetCache(k_source_url, "template11.js", 0,
                           std::make_unique<TestingCacheGenerator>(
                               k_source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);

  auto buffer2 =
      instance.TryGetCache(k_source_url, "template12.js", 0,
                           std::make_unique<TestingCacheGenerator>(
                               k_source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);

  auto buffer3 =
      instance.TryGetCache(k_source_url, "template13.js", 0,
                           std::make_unique<TestingCacheGenerator>(
                               k_source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);

  auto buffer4 =
      instance.TryGetCache(k_source_url, "template14.js", 0,
                           std::make_unique<TestingCacheGenerator>(
                               k_source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::NONE, false, true, true, 0ul, 0ul);

  EXPECT_FALSE(buffer1);
  EXPECT_FALSE(buffer2);
  EXPECT_FALSE(buffer3);
  EXPECT_FALSE(buffer4);

  buffer1 = instance.TryGetCache(k_source_url, "template11.js", 0,
                                 std::make_unique<TestingCacheGenerator>(
                                     k_source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::FILE, true, true, true, 0ul, 0ul);

  buffer2 = instance.TryGetCache(k_source_url, "template12.js", 0,
                                 std::make_unique<TestingCacheGenerator>(
                                     k_source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::FILE, true, true, true, 0ul, 0ul);
  buffer3 = instance.TryGetCache(k_source_url, "template13.js", 0,
                                 std::make_unique<TestingCacheGenerator>(
                                     k_source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::FILE, true, true, true, 0ul, 0ul);
  buffer4 = instance.TryGetCache(k_source_url, "template14.js", 0,
                                 std::make_unique<TestingCacheGenerator>(
                                     k_source_url, js_file_buffer, js_file));
  WaitNormalTaskFinish();
  CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                          JsCacheType::FILE, true, true, true, 0ul, 0ul);

  EXPECT_TRUE(buffer1);
  EXPECT_TRUE(buffer2);
  EXPECT_TRUE(buffer3);
  EXPECT_TRUE(buffer4);
}

TEST_F(JsCacheManagerTest, TryGetCacheForExternal) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  auto js_file_buffer = std::make_shared<StringBuffer>("");
  {
    // dynamic url
    const std::string dynamic_url = "http://from_external.js";
    auto bytecode_getter = std::make_unique<piper::BytecodeGetter>(
        [&dynamic_url](const std::string &url) {
          EXPECT_EQ(url, dynamic_url);
          return std::make_shared<piper::StringBuffer>("from_external");
        });
    auto buffer = instance.TryGetCache(dynamic_url, k_template_url, 0,
                                       std::make_unique<TestingCacheGenerator>(
                                           dynamic_url, js_file_buffer, ""),
                                       bytecode_getter.get());
    WaitNormalTaskFinish();
    EXPECT_TRUE(buffer);
    CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, dynamic_url,
                            JsCacheType::EXTERNAL, true, true, true, 0ul, 0ul);
  }
  {
    // packaged url for app-service.js
    auto bytecode_getter =
        std::make_unique<piper::BytecodeGetter>([](const std::string &url) {
          EXPECT_EQ(url, std::string(k_template_url) + std::string("##") +
                             std::string(k_source_url));
          return std::make_shared<piper::StringBuffer>("from_package");
        });
    auto buffer = instance.TryGetCache(k_source_url, k_template_url, 0,
                                       std::make_unique<TestingCacheGenerator>(
                                           k_source_url, js_file_buffer, ""),
                                       bytecode_getter.get());
    WaitNormalTaskFinish();
    EXPECT_TRUE(buffer);
    CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, k_source_url,
                            JsCacheType::EXTERNAL, true, true, true, 0ul, 0ul);
  }
  {
    // packaged url for lynx://
    const std::string package_url = "lynx://package.js";
    auto bytecode_getter = std::make_unique<piper::BytecodeGetter>(
        [&package_url](const std::string &url) {
          EXPECT_EQ(url, std::string(k_template_url) + std::string("##") +
                             package_url);
          return std::make_shared<piper::StringBuffer>("from_lynx");
        });
    auto buffer = instance.TryGetCache(package_url, k_template_url, 0,
                                       std::make_unique<TestingCacheGenerator>(
                                           package_url, js_file_buffer, ""),
                                       bytecode_getter.get());
    WaitNormalTaskFinish();
    EXPECT_TRUE(buffer);
    CheckOnGetBytecodeEvent(JSRuntimeType::quickjs, package_url,
                            JsCacheType::EXTERNAL, true, true, true, 0ul, 0ul);
  }
}

TEST_F(JsCacheManagerTest, RequestCacheGenerationApp) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  auto js_file_buffer = std::make_shared<StringBuffer>(js_file);
  {
    std::vector<std::unique_ptr<CacheGenerator>> generators;
    generators.push_back(std::make_unique<TestingCacheGenerator>(
        k_source_url, js_file_buffer, js_file));
    instance.RequestCacheGeneration("RequestCacheGeneration.js",
                                    std::move(generators), false);
    WaitNormalTaskFinish();

    auto buffer =
        instance.TryGetCache(k_source_url, "RequestCacheGeneration.js", 0,
                             std::make_unique<TestingCacheGenerator>(
                                 k_source_url, js_file_buffer, js_file));
    WaitNormalTaskFinish();
    EXPECT_TRUE(buffer);
    EXPECT_EQ(std::string((char *)buffer->data()), std::string(js_file));
  }

  {
    std::vector<std::unique_ptr<CacheGenerator>> generators;
    generators.push_back(std::make_unique<TestingCacheGenerator>(
        k_source_url, js_file_buffer, mock_cache));
    instance.RequestCacheGeneration("RequestCacheGeneration.js",
                                    std::move(generators), false);
    WaitNormalTaskFinish();

    auto buffer =
        instance.TryGetCache(k_source_url, "RequestCacheGeneration.js", 0,
                             std::make_unique<TestingCacheGenerator>(
                                 k_source_url, js_file_buffer, js_file));
    WaitNormalTaskFinish();
    EXPECT_TRUE(buffer);
    EXPECT_EQ(std::string((char *)buffer->data()), std::string(js_file));
  }

  {
    std::vector<std::unique_ptr<CacheGenerator>> generators;
    generators.push_back(std::make_unique<TestingCacheGenerator>(
        k_source_url, js_file_buffer, mock_cache));
    instance.RequestCacheGeneration("RequestCacheGeneration.js",
                                    std::move(generators), true);

    WaitNormalTaskFinish();
    auto buffer =
        instance.TryGetCache(k_source_url, "RequestCacheGeneration.js", 0,
                             std::make_unique<TestingCacheGenerator>(
                                 k_source_url, js_file_buffer, js_file));
    WaitNormalTaskFinish();
    EXPECT_TRUE(buffer);
    EXPECT_EQ(std::string((char *)buffer->data()), std::string(js_file));
  }
}

TEST_F(JsCacheManagerTest, RequestCacheGenerationCore) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  const std::string source_url =
      "/lynx_core.js";  // Added to match usage in TestingCacheGenerator
  auto core_file_buffer = std::make_shared<StringBuffer>(core_file);

  auto buffer =
      instance.TryGetCache(source_url, "RequestCacheGeneration.js", 0,
                           std::make_unique<TestingCacheGenerator>(
                               source_url, core_file_buffer, core_file));
  WaitNormalTaskFinish();
  EXPECT_TRUE(buffer);
  EXPECT_EQ(std::string((char *)buffer->data()), std::string(core_file));

  std::vector<std::unique_ptr<CacheGenerator>> generators;
  generators.push_back(std::make_unique<TestingCacheGenerator>(
      source_url, core_file_buffer, mock_cache));
  instance.RequestCacheGeneration("RequestCacheGeneration.js",
                                  std::move(generators), true);
  WaitNormalTaskFinish();

  buffer = instance.TryGetCache(source_url, "RequestCacheGeneration.js", 0,
                                std::make_unique<TestingCacheGenerator>(
                                    source_url, core_file_buffer, mock_cache));
  WaitNormalTaskFinish();
  EXPECT_TRUE(buffer);
  EXPECT_EQ(std::string((char *)buffer->data()), std::string(mock_cache));
}

TEST_F(JsCacheManagerTest, RequestCacheGenerationCallback) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  bool called = false;
  std::vector<std::unique_ptr<CacheGenerator>> generators;
  const std::string cache_key = "RequestCacheGenerationCallback.js";
  const std::string package_url = "lynx://external.js";
  const std::string external_url = "http://external.js";
  generators.push_back(std::make_unique<TestingCacheGenerator>(
      k_source_url, std::make_shared<StringBuffer>(js_file), js_file));
  generators.push_back(std::make_unique<TestingCacheGenerator>(
      package_url, std::make_shared<StringBuffer>(js_file), js_file));
  generators.push_back(std::make_unique<TestingCacheGenerator>(
      external_url, std::make_shared<StringBuffer>(js_file), js_file));
  instance.RequestCacheGeneration(
      cache_key, std::move(generators), false,
      std::make_unique<BytecodeGenerateCallback>(
          [&called, &cache_key, &package_url, &external_url](
              std::string msg,
              std::unordered_map<std::string, std::shared_ptr<Buffer>>
                  buffers) {
            EXPECT_EQ(buffers.size(), 3);
            const std::string app_service_url = cache_key + "##" + k_source_url;
            EXPECT_NE(buffers.find(app_service_url), buffers.end());
            EXPECT_EQ(std::string((char *)buffers[app_service_url]->data()),
                      std::string(js_file));

            const std::string cb_package_url = cache_key + "##" + package_url;
            EXPECT_NE(buffers.find(cb_package_url), buffers.end());
            EXPECT_EQ(std::string((char *)buffers[cb_package_url]->data()),
                      std::string(js_file));

            EXPECT_NE(buffers.find(external_url), buffers.end());
            EXPECT_EQ(std::string((char *)buffers[external_url]->data()),
                      std::string(js_file));
            called = true;
          }));
  WaitNormalTaskFinish();
  EXPECT_TRUE(called);
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

  std::shared_ptr<Buffer> cache = instance.LoadCacheFromStorage(
      identifier, "md5_SaveCacheContentToStorageSuccess");
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

  std::shared_ptr<Buffer> cache = instance.LoadCacheFromStorage(
      identifier, "md5_SaveCacheContentToStorageSuccess2");
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

  std::shared_ptr<Buffer> cache = instance.LoadCacheFromStorage(
      identifier, "md5_SaveCacheContentToStorageSuccess3");
  EXPECT_EQ(std::string((char *)cache->data()),
            std::string("SaveCacheContentToStorageSuccess3"));
  std::cout << instance.GetLockedMetaData()->ToJson() << std::endl;
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

  std::shared_ptr<Buffer> cache = instance.LoadCacheFromStorage(
      identifier, "md5_SaveCacheContentToStorageFailure");
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
  auto info = instance.GetLockedMetaData()->GetFileInfo(identifier);
  EXPECT_FALSE(info.has_value());
}

TEST_F(JsCacheManagerTest, LoadCacheFromStorageWithGetCacheDirFailure) {
  auto &instance =
      QuickjsCacheManagerForTestingWithEmptyGetCacheDir::GetInstance();
  JsFileIdentifier identifier;
  identifier.category = MetaData::CORE_JS;
  identifier.url = "/lynx_core.js";
  identifier.template_url = "template_IZWevh93awfhilsudgbf";
  instance.GetLockedMetaData()->UpdateFileInfo(identifier,
                                               "fsdh94w8hfoshf93lwhfg9", 128);
  auto cache =
      instance.LoadCacheFromStorage(identifier, "fsdh94w8hfoshf93lwhfg9");
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
  {
    auto meta1 =
        QuickjsCacheManagerForTesting::GetInstance().GetLockedMetaData();
    EXPECT_EQ(meta1->GetBytecodeGenerateEngineVersion(),
              std::to_string(LEPUS_GetPrimjsVersion()));
  }
  auto buffer = QuickjsCacheManagerForTesting::GetInstance().TryGetCache(
      k_source_url, k_template_url, 0,
      std::make_unique<TestingCacheGenerator>(
          k_source_url, std::make_shared<StringBuffer>(js_file), js_file));
  WaitNormalTaskFinish();
  buffer = QuickjsCacheManagerForTesting::GetInstance().TryGetCache(
      k_source_url, k_template_url, 0,
      std::make_unique<TestingCacheGenerator>(
          k_source_url, std::make_shared<StringBuffer>(js_file), js_file));
  WaitNormalTaskFinish();
  EXPECT_NE(buffer, nullptr);

  buffer = DifferentEngineVersion::GetInstance().TryGetCache(
      k_source_url, k_template_url, 0,
      std::make_unique<TestingCacheGenerator>(
          k_source_url, std::make_shared<StringBuffer>(js_file), js_file));

  WaitNormalTaskFinish();
  EXPECT_EQ(buffer, nullptr);
  {
    auto meta2 = DifferentEngineVersion::GetInstance().GetLockedMetaData();
    EXPECT_EQ(meta2->GetBytecodeGenerateEngineVersion(), "different_version");
  }
}

// ClearCacheDir
TEST_F(JsCacheManagerTest, ClearCacheDir2) {
  auto &instance = QuickjsCacheManagerForTesting::GetInstance();
  auto buffer = instance.TryGetCache(
      k_source_url, k_template_url, 0,
      std::make_unique<TestingCacheGenerator>(
          k_source_url, std::make_shared<StringBuffer>(js_file), js_file));
  WaitNormalTaskFinish();

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

  std::vector<std::unique_ptr<CacheGenerator>> generators;
  generators.push_back(
      std::make_unique<TestingCacheGenerator>(source_url, buffer, js_content));
  quickjs_instance.RequestCacheGeneration(k_template_url, std::move(generators),
                                          true);
  WaitNormalTaskFinish();
  auto cache = quickjs_instance.TryGetCache(
      source_url, k_template_url, 0,
      std::make_unique<TestingCacheGenerator>(source_url, buffer, js_content));
  WaitNormalTaskFinish();
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
  std::vector<std::unique_ptr<CacheGenerator>> generators;
  generators.push_back(
      std::make_unique<TestingCacheGenerator>(source_url, buffer, js_content));
  quickjs_instance.RequestCacheGeneration(k_template_url, std::move(generators),
                                          true);
  WaitNormalTaskFinish();
  auto cache = quickjs_instance.TryGetCache(
      source_url, k_template_url, 0,
      std::make_unique<TestingCacheGenerator>(source_url, buffer, js_content));
  WaitNormalTaskFinish();
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
  std::vector<std::unique_ptr<CacheGenerator>> generators;
  generators.push_back(std::make_unique<TestingCacheGeneratorFailed>(
      source_url, buffer, js_content));
  quickjs_instance.RequestCacheGeneration(k_template_url, std::move(generators),
                                          true);
  WaitNormalTaskFinish();
  auto cache = quickjs_instance.TryGetCache(
      source_url, k_template_url, 0,
      std::make_unique<TestingCacheGeneratorFailed>(source_url, buffer,
                                                    js_content));
  WaitNormalTaskFinish();
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
    std::vector<std::unique_ptr<CacheGenerator>> generators;
    generators.push_back(std::make_unique<TestingCacheGenerator>(
        k_source_url, buffer, js_content));
    quickjs_instance.RequestCacheGeneration(
        source_url_prefix + std::to_string(i) + ".js", std::move(generators),
        true);
    WaitNormalTaskFinish();
    // to generate different timestamp.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  auto before_clean_file_info =
      quickjs_instance.GetLockedMetaData()->GetAllCacheFileInfo();
  EXPECT_EQ(before_clean_file_info.size(), count);
  quickjs_instance.ClearInvalidCache();
  WaitNormalTaskFinish();
  MoveOnlyEvent event;
  if (s_current_builder) {
    s_current_builder(event);
  }
  CheckCleanUpEvent(JSRuntimeType::quickjs, JsCacheErrorCode::NO_ERROR,
                    std::move(event));
  // Check if removed file is oldest.
  auto after_clean_file_info =
      quickjs_instance.GetLockedMetaData()->GetAllCacheFileInfo();
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
    std::vector<std::unique_ptr<CacheGenerator>> generators;
    generators.push_back(std::make_unique<TestingCacheGenerator>(
        k_source_url, buffer, js_content));
    quickjs_instance.RequestCacheGeneration(
        source_url_prefix + std::to_string(i) + ".js", std::move(generators),
        true);
    WaitNormalTaskFinish();
  }
  quickjs_instance.ClearInvalidCache();
  WaitNormalTaskFinish();
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
    std::vector<std::unique_ptr<CacheGenerator>> generators;
    generators.push_back(std::make_unique<TestingCacheGenerator>(
        source_url, buffer, js_content));
    quickjs_instance.RequestCacheGeneration(
        std::string("/") + std::to_string(i) + "/" + k_template_url,
        std::move(generators), true);
    WaitNormalTaskFinish();
    if (i == 0) {
      std::vector<std::unique_ptr<CacheGenerator>> generators2;
      generators2.push_back(std::make_unique<TestingCacheGenerator>(
          clear_url, buffer, js_content));
      quickjs_instance.RequestCacheGeneration(k_template_url,
                                              std::move(generators2), true);
      WaitNormalTaskFinish();
    }
  }
  auto vec =
      quickjs_instance.GetLockedMetaData()->GetAllCacheFileInfo(clear_url);
  EXPECT_EQ(vec.size(), 1);
  quickjs_instance.ClearCache(clear_url);
  WaitNormalTaskFinish();
  vec = quickjs_instance.GetLockedMetaData()->GetAllCacheFileInfo(clear_url);
  EXPECT_EQ(vec.size(), 0);
  MoveOnlyEvent event;
  if (s_current_builder) {
    s_current_builder(event);
  }
  CheckCleanUpEvent(JSRuntimeType::quickjs, JsCacheErrorCode::NO_ERROR,
                    std::move(event));
}

TEST_F(JsCacheManagerTest, RequestCacheGenerationWithGlobalCallback) {
  auto &quickjs_instance = JsCacheManager::GetQuickjsInstance();
  const std::string source_url = "/generationWithGlobalCallback.js";
  const std::shared_ptr<const Buffer> buffer =
      std::make_shared<StringBuffer>(js_file);

  const std::string js_content =
      "var action = 'request_cache_generation_success';";

  fml::AutoResetWaitableEvent latch;
  JsCacheManager::g_bytecode_generate_callback = std::make_unique<
      BytecodeGenerateCallback>(
      [&latch, &source_url](
          std::string error_msg,
          std::unordered_map<std::string, std::shared_ptr<lynx::piper::Buffer>>
              buffers) {
        EXPECT_EQ(error_msg, "");
        EXPECT_EQ(buffers.size(), 1);
        EXPECT_TRUE(buffers[k_template_url + std::string("##") + source_url]);
        latch.Signal();
      });
  std::vector<std::unique_ptr<CacheGenerator>> generators;
  generators.push_back(
      std::make_unique<TestingCacheGenerator>(source_url, buffer, js_content));
  quickjs_instance.RequestCacheGeneration(k_template_url, std::move(generators),
                                          true);
  WaitNormalTaskFinish();
  JsCacheManager::g_bytecode_generate_callback = nullptr;
  EXPECT_FALSE(latch.WaitWithTimeout(fml::TimeDelta::FromSeconds(5)));
}

TEST_F(JsCacheManagerTest, RequestCacheGenerationWithTwoCallback) {
  auto &quickjs_instance = JsCacheManager::GetQuickjsInstance();
  const std::string source_url = "/generationWithTwoCallback.js";
  const std::shared_ptr<const Buffer> buffer =
      std::make_shared<StringBuffer>(js_file);

  const std::string js_content =
      "var action = 'request_cache_generation_success';";

  fml::AutoResetWaitableEvent latch;
  JsCacheManager::g_bytecode_generate_callback =
      std::make_unique<BytecodeGenerateCallback>(
          [](std::string error_msg,
             std::unordered_map<std::string,
                                std::shared_ptr<lynx::piper::Buffer>>
                 buffers) { EXPECT_TRUE(false); });
  std::vector<std::unique_ptr<CacheGenerator>> generators;
  generators.push_back(
      std::make_unique<TestingCacheGenerator>(source_url, buffer, js_content));
  quickjs_instance.RequestCacheGeneration(
      k_template_url, std::move(generators), true,
      std::make_unique<BytecodeGenerateCallback>(
          [&latch, &source_url](
              std::string error_msg,
              std::unordered_map<std::string,
                                 std::shared_ptr<lynx::piper::Buffer>>
                  buffers) {
            EXPECT_EQ(error_msg, "");
            EXPECT_EQ(buffers.size(), 1);
            EXPECT_TRUE(
                buffers[k_template_url + std::string("##") + source_url]);
            latch.Signal();
          }));
  WaitNormalTaskFinish();
  JsCacheManager::g_bytecode_generate_callback = nullptr;
  EXPECT_FALSE(latch.WaitWithTimeout(fml::TimeDelta::FromSeconds(5)));
}

TEST_F(JsCacheManagerTest, TryGetCacheWithGlobalCallback) {
  auto &quickjs_instance = JsCacheManager::GetQuickjsInstance();
  const std::string source_url = "/TryGetCacheWithGlobalCallback.js";
  const std::shared_ptr<const Buffer> buffer =
      std::make_shared<StringBuffer>(js_file);

  const std::string js_content =
      "var action = 'request_cache_generation_success';";

  fml::AutoResetWaitableEvent latch;
  JsCacheManager::g_bytecode_generate_callback = std::make_unique<
      BytecodeGenerateCallback>(
      [&latch, &source_url](
          std::string error_msg,
          std::unordered_map<std::string, std::shared_ptr<lynx::piper::Buffer>>
              buffers) {
        EXPECT_EQ(error_msg, "");
        EXPECT_EQ(buffers.size(), 1);
        EXPECT_TRUE(buffers[k_template_url + std::string("##") + source_url]);
        latch.Signal();
      });
  quickjs_instance.TryGetCache(
      source_url, k_template_url, 0,
      std::make_unique<TestingCacheGenerator>(source_url, buffer, js_content));
  WaitNormalTaskFinish();
  JsCacheManager::g_bytecode_generate_callback = nullptr;
  EXPECT_FALSE(latch.WaitWithTimeout(fml::TimeDelta::FromSeconds(5)));
}

}  // namespace testing
}  // namespace cache
}  // namespace piper
}  // namespace lynx
