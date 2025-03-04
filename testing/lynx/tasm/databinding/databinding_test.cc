// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "testing/lynx/tasm/databinding/databinding_test.h"

#include <fcntl.h>

#include <algorithm>
#include <fstream>
#include <memory>

#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/shared_data/lynx_white_board.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"

namespace lynx {
namespace tasm {
namespace test {

std::string Utils::g_test_case_root = "databinding_test";
std::string Utils::diff_prefix = "diff/";
std::string Utils::radon_fiber_prefix = "radon_fiber/";
bool Utils::disable_log = true;
bool Utils::dump_to_expected = false;
bool DataBindingTest::initialized = false;

std::vector<uint8_t> Utils::ReadFileFromPath(const std::string& folder,
                                             const std::string& filename) {
  int fd = Utils::SupressStdout();
  std::string full_path = g_test_case_root + folder + filename;
  std::cout << "[DataBinding] Starting read template: " << full_path
            << std::endl;
  std::ifstream instream(full_path, std::ios::in | std::ios::binary);
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(instream)),
                            std::istreambuf_iterator<char>());
  std::cout << "[DataBinding] After read template, size:" << data.size()
            << std::endl;
  Utils::ResumeStdout(fd);
  return data;
}

void Utils::CheckStringEqual(const std::string& expected,
                             const std::string& actual) {
  if (expected == actual) {
    EXPECT_TRUE(true);
    return;
  }
  std::cout << "Check String Equal Failed:\nExpected: \n"
            << expected << "\nActual: \n"
            << actual;
  EXPECT_TRUE(false);
}

std::string Utils::ReadStringFromPath(const std::string& folder,
                                      const std::string& filename) {
  std::string full_path = g_test_case_root + folder + filename;
  std::cout << "[DataBinding] Starting read template: " << full_path
            << std::endl;
  const std::ifstream input_stream(full_path, std::ios_base::binary);
  if (input_stream.fail()) {
    std::ostringstream error_msg;
    error_msg << "Failed to open file: " << full_path << " - ";
    if (input_stream.eof()) {
      error_msg << "EOF Reached";
    } else if (input_stream.bad()) {
      error_msg << "I/O error";
    } else if (input_stream.fail()) {
      error_msg << "Input error";
    }
    throw std::runtime_error(error_msg.str());
  }
  std::stringstream buffer;
  buffer << input_stream.rdbuf();
  return buffer.str();
}

void Utils::WriteStringToFile(const std::string& folder,
                              const std::string& content,
                              const std::string& filename) {
  std::string full_path = g_test_case_root + folder + filename;
  std::ofstream out(full_path);
  out << content;
  out.close();
}

int Utils::SupressStdout() {
  if (disable_log) {
    std::fflush(stdout);
    int ret = dup(1);
    int nullfd = open("/dev/null", O_WRONLY);
    dup2(nullfd, 1);
    close(nullfd);
    return ret;
  }
  return 0;
}

void Utils::ResumeStdout(int fd) {
  if (disable_log) {
    std::fflush(stdout);
    dup2(fd, 1);
    close(fd);
  }
}

std::string Utils::ReplacePrefixString(const std::string& str,
                                       const std::string& target,
                                       const std::string& replacement) {
  std::string modified_str = str;
  lynx::base::ReplaceAll(modified_str, target, replacement);
  return modified_str;
}

DataBindingShell::DataBindingShell(
    DataBindingShell::ThreadStrategy thread_strategy) {
  delegate_ = std::make_unique<testing::NiceMock<MockTasmDelegate>>();
  ResetTasm();
}

DataBindingShell::~DataBindingShell() { DestroyEngineActor(); }

void DataBindingShell::LoadTemplate(
    std::string file_name, std::string folder,
    const std::shared_ptr<TemplateData> template_data) {
  std::vector<uint8_t> data = Utils::ReadFileFromPath(folder, file_name);
  LoadTemplateWithDynamicComponentLoader(file_name, std::move(data),
                                         template_data);
}

void DataBindingShell::LoadTemplate(
    std::string file_name, std::vector<uint8_t> data,
    const std::shared_ptr<TemplateData> template_data) {
  LoadTemplateWithDynamicComponentLoader(file_name, std::move(data),
                                         template_data);
}

void DataBindingShell::LoadTemplateBundle(
    const std::string& file_name, const std::string& folder,
    const std::shared_ptr<TemplateData>& template_data) {
  std::vector<uint8_t> data = Utils::ReadFileFromPath(folder, file_name);
  LoadTemplateWithDynamicComponentLoader(file_name, std::move(data),
                                         template_data);
}

void DataBindingShell::LoadTemplateBundle(
    const std::string& file_name, std::vector<uint8_t> data,
    const std::shared_ptr<TemplateData> template_data) {
  LoadTemplateWithDynamicComponentLoader(file_name, std::move(data),
                                         template_data);
}

void DataBindingShell::LoadTemplateWithDynamicComponent(
    const std::string& file_name,
    const std::unordered_map<std::string, std::string>& component_info_map,
    bool sync, std::string folder,
    const std::shared_ptr<TemplateData> template_data) {
  std::vector<uint8_t> data = Utils::ReadFileFromPath(folder, file_name);
  LoadTemplateWithDynamicComponentLoader(
      file_name, std::move(data), template_data,
      GenerateDynamicComponentLoader(component_info_map, sync));
}

void DataBindingShell::LoadTemplateWithDynamicComponentLoader(
    const std::string& file_name, std::vector<uint8_t> data,
    const std::shared_ptr<TemplateData>& template_data,
    const std::shared_ptr<DataBindingComponentLoader>& loader) {
  int fd = Utils::SupressStdout();
  delegate_->ResetThemeConfig();
  tasm_->page_proxy()->ResetComponentId();
  tasm_->SetLazyBundleLoader(loader);
  TasmLoadTemplate(file_name, std::move(data), template_data);
  Utils::ResumeStdout(fd);
}

void DataBindingShell::LoadDynamicComponent(const std::string& url) {
  if (dynamic_component_loader_) {
    dynamic_component_loader_->LoadDataBindingDynamicComponent(url);
  }
}

void DataBindingShell::PreloadDynamicComponents(
    const std::vector<std::string>& urls,
    const std::unordered_map<std::string, std::string>& component_info_map) {
  tasm_->SetLazyBundleLoader(
      GenerateDynamicComponentLoader(component_info_map));
  tasm_->PreloadLazyBundles(urls);
}

void DataBindingShell::SetOptimizedStyleDiff() {
  tasm_->GetPageConfig()->SetForceCalcNewStyle(false);
}

void DataBindingShell::ResetTasm() {
  constexpr int32_t instance_id = 0;
  auto lynx_env_config = LynxEnvConfig(
      Utils::kWidth, Utils::kHeight, Utils::kDefaultLayoutsUnitPerPx,
      Utils::kDefaultPhysicalPixelsPerLayoutUnit);
  auto manager =
      std::make_unique<ElementManager>(std::make_unique<MockPaintingContext>(),
                                       delegate_.get(), lynx_env_config);
  tasm_ = std::make_shared<lynx::tasm::TemplateAssembler>(
      *delegate_.get(), std::move(manager), instance_id);
  engine_actor_ = std::make_shared<shell::LynxActor<shell::LynxEngine>>(
      std::make_unique<shell::LynxEngine>(tasm_, nullptr, nullptr,
                                          shell::kUnknownInstanceId),
      fml::MakeRefCounted<MockTasmRunner>());

  tasm_->SetWhiteBoard(std::make_shared<WhiteBoard>());
  tasm_->SetLazyBundleLoader(GenerateDynamicComponentLoader({}, false));
}

void DataBindingShell::Replay(std::string replay_file_name,
                              bool use_ark_source) {
  int fd = Utils::SupressStdout();
  std::string replay_string =
      Utils::ReadStringFromPath(REPLAY_TEST_REPLAY_FOLDER, replay_file_name);
  auto replayer = std::make_shared<DataUpdateReplayer>(tasm_);
  replayer->DataUpdateReplay(replay_string, use_ark_source);
  Utils::ResumeStdout(fd);
}

void DataBindingShell::CheckWithOption(const TestOptions& input) {
  std::string page_source = this->page_source();
  std::string delegate_source = this->delegate_source();
  if (Utils::dump_to_expected) {
    Utils::WriteStringToFile(EXPECTED_FOLDER, page_source, input.page_output_);
    Utils::WriteStringToFile(EXPECTED_FOLDER, delegate_source,
                             input.delegate_output_);
  } else {
    std::string page_expected =
        Utils::ReadStringFromPath(EXPECTED_FOLDER, input.page_output_);
    Utils::CheckStringEqual(page_expected, page_source);
    std::string delegate_expected =
        Utils::ReadStringFromPath(EXPECTED_FOLDER, input.delegate_output_);
    Utils::CheckStringEqual(delegate_expected, delegate_source);
  }
  if (!input.element_output_.empty()) {
    std::string element_source = this->element_source();
    if (Utils::dump_to_expected) {
      Utils::WriteStringToFile(EXPECTED_FOLDER, element_source,
                               input.element_output_);
    } else {
      std::string element_expected =
          Utils::ReadStringFromPath(EXPECTED_FOLDER, input.element_output_);
      Utils::CheckStringEqual(element_expected, element_source);
    }
  }
}

void DataBindingShell::CheckWithOption(const ReplayTestOptions& input) {
  std::string page_source = this->page_source();
  std::string element_source = this->element_source();
  std::string delegate_source = this->delegate_source();
  if (Utils::dump_to_expected) {
    Utils::WriteStringToFile(REPLAY_EXPECTED_FOLDER, page_source,
                             input.page_output_);
    Utils::WriteStringToFile(REPLAY_EXPECTED_FOLDER, element_source,
                             input.element_output_);
    Utils::WriteStringToFile(REPLAY_EXPECTED_FOLDER, delegate_source,
                             input.delegate_output_);

  } else {
    std::string page_expected =
        Utils::ReadStringFromPath(REPLAY_EXPECTED_FOLDER, input.page_output_);
    Utils::CheckStringEqual(page_expected, page_source);
    std::string element_expected = Utils::ReadStringFromPath(
        REPLAY_EXPECTED_FOLDER, input.element_output_);
    Utils::CheckStringEqual(element_expected, element_source);
    std::string delegate_expected = Utils::ReadStringFromPath(
        REPLAY_EXPECTED_FOLDER, input.delegate_output_);
    Utils::CheckStringEqual(delegate_expected, delegate_source);
  }
}

void DataBindingShell::CheckWithOption(const SnapshotTestOptions& input) {
  auto markup = this->page_source_markup();
  auto snapshot = this->page_source_snapshot();

  if (Utils::dump_to_expected) {
    Utils::WriteStringToFile(EXPECTED_FOLDER, markup,
                             input.page_markup_output_);
    Utils::WriteStringToFile(EXPECTED_FOLDER, snapshot,
                             input.page_snapshot_output_);
  } else {
    std::string page_source_markup_expected =
        Utils::ReadStringFromPath(EXPECTED_FOLDER, input.page_markup_output_);
    Utils::CheckStringEqual(page_source_markup_expected, markup);
    std::string page_source_snapshot_expected =
        Utils::ReadStringFromPath(EXPECTED_FOLDER, input.page_snapshot_output_);
    Utils::CheckStringEqual(page_source_snapshot_expected, snapshot);
  }
}

std::shared_ptr<DataBindingComponentLoader>
DataBindingShell::GenerateDynamicComponentLoader(
    const std::unordered_map<std::string, std::string>& component_info_map,
    bool sync, bool enable_bundle_as_result) {
  auto loader =
      std::make_shared<DataBindingComponentLoader>(component_info_map, sync);
  loader->SetEnableTemplateBundleAsResult(enable_bundle_as_result);
  loader->SetEngineActor(engine_actor_);

  dynamic_component_loader_ = loader;

  return loader;
}

void DataBindingShell::DestroyEngineActor() {
  if (engine_actor_) {
    engine_actor_->Act([](auto& engine) { engine = nullptr; });
  }
}

DataBindingComponentLoader::DataBindingComponentLoader(
    const std::unordered_map<std::string, std::string>&
        dynamic_component_info_map,
    bool sync)
    : dynamic_component_info_map_(dynamic_component_info_map), sync_(sync) {}

void DataBindingComponentLoader::RequireTemplate(RadonLazyComponent* comp,
                                                 const std::string& url,
                                                 int instance_id) {
  if (sync_) {
    LoadDataBindingDynamicComponent(url, sync_, comp);
  } else {
    ++async_requires_[url];
  }
}

void DataBindingComponentLoader::LoadDataBindingDynamicComponent(
    const std::string& url, bool sync, RadonLazyComponent* comp) {
  if (!sync) {
    // Prevent triggering requests that were not made in databinding
    auto require = async_requires_.find(url);
    if (require == async_requires_.end() || require->second <= 0) {
      return;
    }
    --require->second;
  }
  auto source = FetchResource(url);

  if (enable_bundle_as_result_) {
    auto reader = tasm::LynxBinaryReader::CreateLynxBinaryReader(source);
    if (reader.Decode()) {
      auto template_bundle = reader.GetTemplateBundle();
      DidLoadComponent(lynx::tasm::LazyBundleLoader::CallBackInfo{
          url, source, std::move(template_bundle), std::nullopt, comp, 0});
      return;
    }
  }
  DidLoadComponent(lynx::tasm::LazyBundleLoader::CallBackInfo{
      url, source, std::nullopt, std::nullopt, comp, 0});
}

void DataBindingComponentLoader::PreloadTemplates(
    const std::vector<std::string>& urls) {
  std::for_each(urls.begin(), urls.end(), [this](const auto& url) {
    LazyBundleLoader::CallBackInfo info{url, this->FetchResource(url),
                                        std::nullopt, std::nullopt};
    this->DidPreloadTemplate(std::move(info));
  });
}

std::vector<uint8_t> DataBindingComponentLoader::FetchResource(
    const std::string& url) {
  auto path = dynamic_component_info_map_.find(url);
  return path != dynamic_component_info_map_.end()
             ? Utils::ReadFileFromPath(TEST_INPUT_FOLDER, path->second)
             : std::vector<uint8_t>();
}

void DataBindingTest::CheckEquality(const std::string& left,
                                    const std::string& right) {
  left_->LoadTemplate(left);
  right_->LoadTemplate(right);
  Utils::CheckStringEqual(left_->element_source(), right_->element_source());
  Utils::CheckStringEqual(left_->delegate_source(), right_->delegate_source());
}

void DataBindingShell::TasmLoadTemplate(
    const std::string& url, std::vector<uint8_t> source,
    const std::shared_ptr<TemplateData>& template_data) {
  PipelineOptions pipeline_options;
  tasm_->LoadTemplate(url, std::move(source), template_data, pipeline_options,
                      test_pre_painting_);
}

void DataBindingLoadTemplateBundleShell::TasmLoadTemplate(
    const std::string& url, std::vector<uint8_t> source,
    const std::shared_ptr<TemplateData>& template_data) {
  auto reader =
      tasm::LynxBinaryReader::CreateLynxBinaryReader(std::move(source));
  reader.SetIsCardType(true);
  if (!reader.Decode()) {
    return;
  }

  auto template_bundle = reader.GetTemplateBundle();
  // pre-create context if is lepusNG
  if (template_bundle.is_lepusng_binary()) {
    template_bundle.page_configs_->SetEnableUseContextPool(true);
    template_bundle.quick_context_pool_ =
        lepus::QuickContextPool::Create(template_bundle.context_bundle_);
    template_bundle.quick_context_pool_->AddContextSafely(1);
  }

  PipelineOptions pipeline_options;
  tasm_->LoadTemplateBundle(url, std::move(template_bundle), template_data,
                            pipeline_options, test_pre_painting_);
}

void DataBindingTemplateBundleRecycleShell::TasmLoadTemplate(
    const std::string& url, std::vector<uint8_t> source,
    const std::shared_ptr<TemplateData>& template_data) {
  LynxTemplateBundle recycled_bundle;

  PipelineOptions pipeline_options;
  // get recycled bundle
  {
    DataBindingShell mock_shell;
    mock_shell.tasm_->LoadTemplate(url, std::move(source), nullptr,
                                   pipeline_options, false, true);
    recycled_bundle = mock_shell.delegate_->GetTemplateBundle();
  }

  // load with recycled bundle
  tasm_->LoadTemplateBundle(url, std::move(recycled_bundle), template_data,
                            pipeline_options, test_pre_painting_);
}

void DataBindingShell::UpdateDataByJS(const lepus::Value& table) {
  PipelineOptions pipeline_options;
  runtime::UpdateDataTask task(true, "-1", table, piper::ApiCallBack(),
                               runtime::UpdateDataType(),
                               std::move(pipeline_options));
  tasm_->UpdateDataByJS(task, task.pipeline_options_);
}

void DataBindingShell::UpdateDataByPreParsedData(const lepus::Value& table,
                                                 bool reset) {
  PipelineOptions pipeline_options;
  UpdatePageOption update_page_option;
  update_page_option.from_native = true;
  update_page_option.reset_page_data = reset;
  tasm_->UpdateDataByPreParsedData(
      std::make_shared<TemplateData>(lepus::Value(table), true),
      update_page_option, pipeline_options);
}

const std::shared_ptr<WhiteBoardDelegate>&
DataBindingShell::GetWhiteBoardDelegate() {
  return tasm_->GetWhiteBoardDelegate();
}

void DataBindingDynamicComponentWithTemplateBundleShell::
    LoadTemplateWithDynamicComponentLoader(
        const std::string& file_name, std::vector<uint8_t> data,
        const std::shared_ptr<TemplateData>& template_data,
        const std::shared_ptr<DataBindingComponentLoader>& loader) {
  if (loader != nullptr) {
    loader->SetEnableTemplateBundleAsResult(true);
  }
  DataBindingShell::LoadTemplateWithDynamicComponentLoader(
      file_name, data, template_data, loader);
}

RadonFiberTest::RadonFiberTest() {
  lynx::tasm::LynxEnv::GetInstance().external_env_map_
      [lynx::tasm::LynxEnv::Key::ENABLE_FIBER_ELEMENT_FOR_RADON_DIFF] = "true";
}

RadonFiberTest::~RadonFiberTest() {
  lynx::tasm::LynxEnv::GetInstance().external_env_map_
      [lynx::tasm::LynxEnv::Key::ENABLE_FIBER_ELEMENT_FOR_RADON_DIFF] = "false";
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
