// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bytecode/js_cache_manager_facade.h"

#include <iostream>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

#define private public

#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace runtime {
namespace js {
namespace cache {
namespace testing {

namespace {
std::string source_url_for_testing;
}

void PostCacheGenerationTaskForTesting(
    const std::string& template_url,
    std::unordered_map<std::string, JsContent> js_contents) {
  for (const auto& iter : js_contents) {
    source_url_for_testing += iter.first;
  }
}

TEST(JsCacheManagerFacadeTest, PostCacheGenerationTaskQuickJs) {
  JsCacheManagerFacade::post_cache_generation_task_quickjs_for_testing =
      &PostCacheGenerationTaskForTesting;

  source_url_for_testing = "";

  tasm::LynxTemplateBundle bundle;

  // test bundle of card
  bundle.app_type_ = tasm::APP_TYPE_CARD;
  bundle.js_bundle_.AddJsContent("/app-service.js",
                                 {"test", JsContent::Type::SOURCE});

  JsCacheManagerFacade::PostCacheGenerationTask(bundle, "template_url",
                                                JSRuntimeType::quickjs);

  ASSERT_EQ(source_url_for_testing, "/app-service.js");

  // test bundle of dynamic components
  source_url_for_testing = "";
  bundle.app_type_ = tasm::APP_TYPE_DYNAMIC_COMPONENT;
  bundle.js_bundle_.AddJsContent("/app-service.js",
                                 {"test", JsContent::Type::SOURCE});
  JsCacheManagerFacade::PostCacheGenerationTask(
      bundle, "http://dynamic/template.js", JSRuntimeType::quickjs);

  ASSERT_EQ(source_url_for_testing,
            "dynamic-component/http://dynamic/template.js//app-service.js");
}

TEST(JsCacheManagerFacadeTest, PostCacheGenerationTaskQuickJsFail) {
  source_url_for_testing = "";

  tasm::LynxTemplateBundle bundle;
  JsCacheManagerFacade::PostCacheGenerationTask(bundle, "template_url",
                                                JSRuntimeType::quickjs);

  ASSERT_EQ(source_url_for_testing, "");
}

TEST(JsCacheManagerFacadeTest, PostCacheGenerationTaskV8) {
  source_url_for_testing = "";

  tasm::LynxTemplateBundle bundle;
  bundle.js_bundle_.AddJsContent("/app-service.js",
                                 {"test", JsContent::Type::SOURCE});

  JsCacheManagerFacade::PostCacheGenerationTask(bundle, "template_url",
                                                JSRuntimeType::v8);

  ASSERT_EQ(source_url_for_testing, "");
}

}  // namespace testing
}  // namespace cache
}  // namespace js
}  // namespace runtime
}  // namespace lynx
