// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/js_cache_manager_facade.h"

#include <memory>
#include <utility>

#include "core/runtime/bindings/jsi/js_app.h"
#include "core/runtime/jscache/js_cache_manager.h"
#include "core/runtime/jscache/quickjs/quickjs_cache_generator.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace piper {
namespace cache {
void JsCacheManagerFacade::PostCacheGenerationTask(
    const tasm::LynxTemplateBundle& bundle, const std::string& template_url,
    JSRuntimeType engine_type) {
  std::string path = runtime::kAppServiceJSName;
  auto source_opt = bundle.GetJsBundle().GetJsContent(path);
  if (source_opt) {
    const JsContent& source = *source_opt;
    if (source.IsSourceCode()) {
      PostCacheGenerationTask(
          bundle.IsCard() ? path
                          : piper::App::GenerateDynamicComponentSourceUrl(
                                template_url, path),
          template_url, source.GetBuffer(), engine_type);
    }
  }
}

void JsCacheManagerFacade::PostCacheGenerationTask(
    const std::string& source_url, const std::string& template_url,
    const std::shared_ptr<const StringBuffer>& source,
    JSRuntimeType engine_type) {
  LOGI("JsCacheManagerFacade::PostCacheGenerationTask source_url: "
       << source_url << " template_url: " << template_url
       << " engine_type: " << static_cast<int>(engine_type));
  switch (engine_type) {
    case JSRuntimeType::v8:
      LOGI("PostCacheGenerationTask for V8 is not supported");
      return;
    case JSRuntimeType::jsc:
      LOGI("PostCacheGenerationTask for JSC is not supported");
      return;
    case JSRuntimeType::quickjs: {
      PostCacheGenerationTaskQuickJs(source_url, template_url, source);
      return;
    }
  }
}

void JsCacheManagerFacade::ClearBytecode(const std::string& template_url,
                                         JSRuntimeType engine_type) {
  if (engine_type == JSRuntimeType::quickjs) {
    JsCacheManager::GetQuickjsInstance().ClearCache(template_url);
  } else if (engine_type == JSRuntimeType::v8) {
    JsCacheManager::GetV8Instance().ClearCache(template_url);
  }
}

inline void JsCacheManagerFacade::PostCacheGenerationTaskQuickJs(
    const std::string& source_url, const std::string& template_url,
    const std::shared_ptr<const StringBuffer>& buffer) {
#ifdef QUICKJS_CACHE_UNITTEST
  post_cache_generation_task_quickjs_for_testing(source_url, template_url,
                                                 buffer);
#else
  auto generator =
      std::make_unique<cache::QuickjsCacheGenerator>(source_url, buffer);
  JsCacheManager::GetQuickjsInstance().RequestCacheGeneration(
      source_url, template_url, buffer, std::move(generator), false);
#endif
}
}  // namespace cache
}  // namespace piper
}  // namespace lynx
