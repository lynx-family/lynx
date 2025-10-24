// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSCACHE_JS_CACHE_MANAGER_FACADE_H_
#define CORE_RUNTIME_JSCACHE_JS_CACHE_MANAGER_FACADE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/closure.h"
#include "core/runtime/jscache/js_cache_manager.h"

namespace lynx {
namespace tasm {
class LynxTemplateBundle;
}

namespace piper {
class StringBuffer;
class Buffer;
class JsContent;
enum class JSRuntimeType;

namespace cache {
// The JsCacheManagerFacade class is the C++ layer interface of Bytecode, and
// it is a wrapper of the JsCacheManager class. This class receives different
// parameter inputs, forwards the operation request to a specific class
// according to the target JS engine type, and hides the specific
// implementation. The methods of this class guarantee thread safety for code
// cache operations (excluding reading and writing of incoming parameters, which
// need to be guaranteed by the caller).
class JsCacheManagerFacade {
 public:
  // The PostCacheGenerationTask method makes a cache generation request to the
  // app-service.js code stored in the incoming LynxTemplateBundle. Cache
  // generation will be performed asynchronously on a background thread.
  static void PostCacheGenerationTask(
      const tasm::LynxTemplateBundle& bundle, const std::string& template_url,
      JSRuntimeType engine_type,
      std::unique_ptr<BytecodeGenerateCallback> callback = nullptr);

  static void ClearBytecode(const std::string& template_url,
                            JSRuntimeType engine_type);

  static void SetGlobalBytecodeGenerateCallback(
      std::unique_ptr<BytecodeGenerateCallback> callback);

 private:
  static void PostCacheGenerationTask(
      const std::string& template_url,
      std::unordered_map<std::string, JsContent> js_contents,
      JSRuntimeType engine_type,
      std::unique_ptr<BytecodeGenerateCallback> callback = nullptr);

  static inline void PostCacheGenerationTaskQuickJs(
      const std::string& template_url,
      std::unordered_map<std::string, JsContent> js_contents,
      std::unique_ptr<BytecodeGenerateCallback> callback);

#ifdef QUICKJS_CACHE_UNITTEST
 public:
  static inline void (*post_cache_generation_task_quickjs_for_testing)(
      const std::string& template_url,
      std::unordered_map<std::string, JsContent> js_contents) = nullptr;
#endif
};
}  // namespace cache
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSCACHE_JS_CACHE_MANAGER_FACADE_H_
