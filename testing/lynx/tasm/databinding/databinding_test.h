// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef TESTING_LYNX_TASM_DATABINDING_DATABINDING_TEST_H_
#define TESTING_LYNX_TASM_DATABINDING_DATABINDING_TEST_H_

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/message_loop_impl.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/node_selector.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/template_assembler.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/shared_data/white_board_delegate.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/testing/mock_runner_manufactor.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
#include "testing/lynx/tasm/databinding/data_update_replayer.h"
#include "testing/lynx/tasm/databinding/element_dump_helper.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

#define TEST_INPUT_FOLDER "/test_cases/input/"
#define EXPECTED_FOLDER "/test_cases/expected/"
#define REPLAY_TEST_INPUT_FOLDER "/test_cases/replay_input/"
#define REPLAY_TEST_REPLAY_FOLDER "/test_cases/replay_input/replay/"
#define REPLAY_EXPECTED_FOLDER "/test_cases/replay_expected/"

class TestOptions {
 public:
  TestOptions(const std::string input, const std::string page,
              const std::string element, const std::string delegate)
      : input_(std::move(input)),
        page_output_(std::move(page)),
        element_output_(std::move(element)),
        delegate_output_(std::move(delegate)) {}
  const std::string input_;
  const std::string page_output_;
  const std::string element_output_;
  const std::string delegate_output_;
};

class FiberTestOptions : public TestOptions {
 public:
  FiberTestOptions(const std::string& prefix)
      : TestOptions(prefix + ".js", prefix + "_page.json", "",
                    prefix + "_delegate.txt") {}

  FiberTestOptions(const char* input, const char* page, const char* delegate)
      : TestOptions(input, page, "", delegate) {}
};

class ReplayTestOptions : public TestOptions {
 public:
  ReplayTestOptions(const char* input, const char* replay_input,
                    const char* page, const char* element, const char* delegate)
      : TestOptions(input, page, element, delegate),
        replay_input_(replay_input) {}
  const std::string replay_input_;
};

class SnapshotTestOptions {
 public:
  SnapshotTestOptions(const char* input, const char* page_markup,
                      const char* page_snapshot)
      : input_(input),
        page_markup_output_(page_markup),
        page_snapshot_output_(page_snapshot) {}
  const std::string input_;
  const std::string page_markup_output_;
  const std::string page_snapshot_output_;
};

class MockRunner : public fml::TaskRunner {
 public:
  MockRunner() : fml::TaskRunner(nullptr) {}
  void PostTask(base::closure task) override {}
};

class MockTasmRunner : public fml::TaskRunner {
 public:
  MockTasmRunner() : fml::TaskRunner(nullptr) {}
  bool RunsTasksOnCurrentThread() override { return true; }
  void PostTask(base::closure task) override { task(); }
};

class Utils {
 public:
  static constexpr int32_t kWidth = 1024;
  static constexpr int32_t kHeight = 768;
  static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
  static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;
  static std::string g_test_case_root;
  static std::string diff_prefix;
  static std::string radon_fiber_prefix;
  static bool dump_to_expected;
  static bool disable_log;
  static std::string ReadStringFromPath(const std::string& folder,
                                        const std::string& filename);
  static std::vector<uint8_t> ReadFileFromPath(const std::string& folder,
                                               const std::string& filename);

  static void WriteStringToFile(const std::string& folder,
                                const std::string& content,
                                const std::string& filename);

  static void CheckStringEqual(const std::string& expected,
                               const std::string& actual);

  static void InitEnv() {
    // hook ui runner, avoid PerfCollector influence benchmark
    lynx::base::UIThread::Init();
    lynx::base::UIThread::GetRunner() = fml::MakeRefCounted<MockRunner>();
  }

  static int SupressStdout();
  static void ResumeStdout(int fd);
  static std::string ReplacePrefixString(const std::string& str,
                                         const std::string& target,
                                         const std::string& replacement);
};

class RadonFiberTestOptions : public TestOptions {
 public:
  RadonFiberTestOptions(const std::string& input, const std::string& page,
                        const std::string& element, const std::string& delegate,
                        bool reuse_delegate = true)
      : TestOptions(input,
                    Utils::ReplacePrefixString(page, Utils::diff_prefix,
                                               Utils::radon_fiber_prefix),
                    Utils::ReplacePrefixString(element, Utils::diff_prefix,
                                               Utils::radon_fiber_prefix),
                    reuse_delegate ? delegate
                                   : Utils::ReplacePrefixString(
                                         delegate, Utils::diff_prefix,
                                         Utils::radon_fiber_prefix)) {}
};

class RadonFiberReplayTestOptions : public ReplayTestOptions {
 public:
  RadonFiberReplayTestOptions(const char* input, const char* replay_input,
                              const char* page, const char* element,
                              const char* delegate)
      : ReplayTestOptions(
            input, replay_input,
            Utils::ReplacePrefixString(page, Utils::diff_prefix,
                                       Utils::radon_fiber_prefix)
                .c_str(),
            Utils::ReplacePrefixString(element, Utils::diff_prefix,
                                       Utils::radon_fiber_prefix)
                .c_str(),
            delegate) {}
};

class DataBindingComponentLoader;
class DataBindingShell {
 public:
  bool test_pre_painting_ = false;
  using ThreadStrategy = lynx::base::ThreadStrategyForRendering;

  DataBindingShell(ThreadStrategy thread_strategy = ThreadStrategy::ALL_ON_UI);
  virtual ~DataBindingShell();

  void LoadTemplate(
      std::string file_name, std::string folder = TEST_INPUT_FOLDER,
      const std::shared_ptr<TemplateData> template_data = nullptr);

  void LoadTemplate(
      std::string file_name, std::vector<uint8_t> data,
      const std::shared_ptr<TemplateData> template_data = nullptr);

  void LoadTemplateBundle(
      const std::string& file_name,
      const std::string& folder = TEST_INPUT_FOLDER,
      const std::shared_ptr<TemplateData>& template_data = nullptr);

  void LoadTemplateBundle(
      const std::string& file_name, std::vector<uint8_t> data,
      const std::shared_ptr<TemplateData> template_data = nullptr);

  void LoadTemplateWithDynamicComponent(
      const std::string& file_name,
      const std::unordered_map<std::string, std::string>& component_info_map,
      bool sync, std::string folder = TEST_INPUT_FOLDER,
      const std::shared_ptr<TemplateData> template_data = nullptr);

  void UpdateDataByJS(const lepus::Value& table);

  void UpdateDataByPreParsedData(const lepus::Value& table, bool reset = false);

  void SetOptimizedStyleDiff();

  void ResetTasm();

  void Replay(std::string replay_file_name, bool use_ark_source = false);

  std::string page_source() {
    return ElementDumpHelper::DumpTree(tasm_->page_proxy());
  }

  std::string page_source_markup() {
    std::ostringstream ss;
    ElementDumpHelper::DumpToMarkup(
        tasm_->page_proxy()->element_manager()->root(), ss);
    return ss.str();
  }
  std::string page_source_snapshot() {
    auto json = lepus::lepusValueToString(ElementDumpHelper::DumpToSnapshot(
        tasm_->page_proxy()->element_manager()->root()));

    // prettier the json
    rapidjson::Document d;
    d.Parse(json);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);

    return buffer.GetString();
  }

  std::string element_source() {
    auto* root = tasm_->page_proxy()->element_manager()->root();
    return ElementDumpHelper::DumpElement(root);
  }

  void CheckWithOption(const TestOptions& input);

  void CheckWithOption(const ReplayTestOptions& input);

  void CheckWithOption(const SnapshotTestOptions& input);

  std::string delegate_source() { return delegate_->DumpDelegate(); }

  void LoadDynamicComponent(const std::string& url);

  void PreloadDynamicComponents(
      const std::vector<std::string>& urls,
      const std::unordered_map<std::string, std::string>& component_info_map);

  const std::shared_ptr<WhiteBoardDelegate>& GetWhiteBoardDelegate();

  std::unique_ptr<MockTasmDelegate> delegate_;
  std::shared_ptr<TemplateAssembler> tasm_;
  std::shared_ptr<shell::LynxEngine> engine_;

 protected:
  virtual void TasmLoadTemplate(
      const std::string& url, std::vector<uint8_t> source,
      const std::shared_ptr<TemplateData>& template_data);

  virtual void LoadTemplateWithDynamicComponentLoader(
      const std::string& file_name, std::vector<uint8_t> data,
      const std::shared_ptr<TemplateData>& template_data,
      const std::shared_ptr<DataBindingComponentLoader>& loader = nullptr);

 private:
  std::shared_ptr<DataBindingComponentLoader> GenerateDynamicComponentLoader(
      const std::unordered_map<std::string, std::string>& component_info_map,
      bool sync = true, bool enable_bundle_as_result = false);

  void DestroyEngineActor();
  /**
   * For dynamic component loader.
   * In order to prevent circular references, it must be actively released.
   */
  std::shared_ptr<shell::LynxActor<shell::LynxEngine>> engine_actor_;
  std::shared_ptr<DataBindingComponentLoader> dynamic_component_loader_;
};

class DataBindingComponentLoader : public LazyBundleLoader {
 public:
  DataBindingComponentLoader(const std::unordered_map<std::string, std::string>&
                                 dynamic_component_info_map = {},
                             bool sync = false);
  virtual void RequireTemplate(RadonLazyComponent* comp, const std::string& url,
                               int trace_id) override;
  void LoadDataBindingDynamicComponent(const std::string& url,
                                       bool sync = false,
                                       RadonLazyComponent* comp = nullptr);

  void PreloadTemplates(const std::vector<std::string>& urls) override;

  void SetEnableTemplateBundleAsResult(bool enable_bundle_result) {
    this->enable_bundle_as_result_ = enable_bundle_result;
  }

 private:
  std::vector<uint8_t> FetchResource(const std::string& url);

  std::unordered_map<std::string, std::string> dynamic_component_info_map_;
  bool sync_;
  std::unordered_map<std::string, int32_t> async_requires_;

  bool enable_bundle_as_result_{false};
};

class DataBindingTest : public ::testing::Test {
 public:
  void SetUp() override {
    if (!initialized) {
      lynx::tasm::test::Utils::InitEnv();
      initialized = true;
    }
  }

  void TearDown() override {}

  DataBindingTest() {
    left_.reset(new DataBindingShell());
    right_.reset(new DataBindingShell());
  }
  virtual ~DataBindingTest() = default;

  void SingleTest(const TestOptions& input);

  void CheckEquality(const std::string& left, const std::string& right);

  std::unique_ptr<DataBindingShell> left_{nullptr};
  std::unique_ptr<DataBindingShell> right_{nullptr};
  const std::string PAGE_GROUP_ID = "-1";

  bool static initialized;
};

class DataBindingLoadTemplateBundleShell : public DataBindingShell {
 protected:
  void TasmLoadTemplate(
      const std::string& url, std::vector<uint8_t> source,
      const std::shared_ptr<TemplateData>& template_data) override;
};

// The only difference between this test and DataBindingTest is that
// DataBindingLoadTemplateBundleTest will override the behavior of LoadTemplate
// to LoadTemplateBundle to verify the consistency of codec results
class DataBindingLoadTemplateBundleTest : public DataBindingTest {
 public:
  DataBindingLoadTemplateBundleTest() {
    left_.reset(new DataBindingLoadTemplateBundleShell());
    right_.reset(new DataBindingLoadTemplateBundleShell());
  }
};

class DataBindingTemplateBundleRecycleShell : public DataBindingShell {
 protected:
  void TasmLoadTemplate(
      const std::string& url, std::vector<uint8_t> source,
      const std::shared_ptr<TemplateData>& template_data) override;
};

// The only difference between this test and DataBindingTest is that
// DataBindingLoadTemplateBundleTest will override the behavior of LoadTemplate
// to Load the recycled TemplateBundle to verify the consistency of codec
// results
class DataBindingTemplateBundleRecycleTest : public DataBindingTest {
 public:
  DataBindingTemplateBundleRecycleTest() {
    left_.reset(new DataBindingTemplateBundleRecycleShell());
    right_.reset(new DataBindingTemplateBundleRecycleShell());
  }
};

class DataBindingDynamicComponentWithTemplateBundleShell
    : public DataBindingShell {
 public:
  void LoadTemplateWithDynamicComponentLoader(
      const std::string& file_name, std::vector<uint8_t> data,
      const std::shared_ptr<TemplateData>& template_data,
      const std::shared_ptr<DataBindingComponentLoader>& loader =
          nullptr) override;
};

class DataBindingDynamicComponentWithTemplateBundleTest
    : public DataBindingTest {
 public:
  DataBindingDynamicComponentWithTemplateBundleTest() {
    left_.reset(new DataBindingDynamicComponentWithTemplateBundleShell());
    right_.reset(new DataBindingDynamicComponentWithTemplateBundleShell());
  }
};

class RadonFiberTest {
 public:
  RadonFiberTest();
  ~RadonFiberTest();
};

class RadonFiberDataBindingTest : public DataBindingTest,
                                  public RadonFiberTest {};

class RadonFiberDataBindingLoadTemplateBundleTest
    : public DataBindingLoadTemplateBundleTest,
      public RadonFiberTest {};

class RadonFiberDataBindingTemplateBundleRecycleTest
    : public DataBindingTemplateBundleRecycleTest,
      public RadonFiberTest {};

class RadonFiberDataBindingDynamicComponentWithTemplateBundleTest
    : public DataBindingDynamicComponentWithTemplateBundleTest,
      public RadonFiberTest {};

}  // namespace test
}  // namespace tasm
}  // namespace lynx

#endif  // TESTING_LYNX_TASM_DATABINDING_DATABINDING_TEST_H_
