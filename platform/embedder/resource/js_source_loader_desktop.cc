// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/resource/js_source_loader_desktop.h"

#include <fstream>

#include "base/include/log/logging.h"
#include "clay/fml/paths.h"
#include "core/base/utils/paths_win.h"
#include "core/renderer/utils/lynx_env.h"
#include "platform/embedder/resource/core_js_loader_manager.h"
#if OS_WIN
#include "base/include/string/string_conversion_win.h"
#define DIR_SEP '\\'
#else
#define DIR_SEP '/'
#endif

namespace lynx {
namespace runtime {
namespace js {
// JSSourceLoaderDesktop is used for windows,
// and Chinese path problems need to be considered
std::string JSSourceLoaderDesktop::LoadFileData(const std::string& path) {
  std::string res;
#if OS_WIN
  std::wfstream file_rstream;
  file_rstream.open(lynx::base::Utf16FromUtf8(path),
                    std::ios::in | std::ios::binary);
#else
  std::ifstream file_rstream;
  file_rstream.open(path, std::ios::in | std::ios::binary);
#endif
  if (file_rstream.is_open()) {
#if OS_WIN
    res = std::string(std::istreambuf_iterator<wchar_t>(file_rstream),
                      std::istreambuf_iterator<wchar_t>());
#else
    res = std::string(std::istreambuf_iterator<char>(file_rstream),
                      std::istreambuf_iterator<char>());
#endif
  } else {
    LOGE("JSSourceLoaderDesktop::LoadJSSource load js error path= " << path);
  }
  file_rstream.close();
  return res;
}
std::string JSSourceLoaderDesktop::LoadJSSource(const std::string& path) {
  static const std::string FILE_SCHEME = "file://";
  static const std::string FILE_LYNX_SCHEME = "file://lynx?local://";
  static const std::string CORE_DEBUG_JS = "lynx_core_dev";
  static const std::string ASSETS_SCHEME = "assets://";
  static const std::string ASSETS_CORE_SCHEME = "assets://lynx_core.js";
  std::string res;
  LOGI("JSSourceLoaderDesktop::LoadJSSource load js source path= " << path);

  std::string real_path;
  if (ASSETS_CORE_SCHEME == path) {
    ICoreJsLoader* loader = CoreJsLoaderManager::GetInstance()->GetLoader();
    if (loader) {
      real_path = loader->GetCoreJs();
    }
  }

  if (real_path.empty()) {
    // todo: handle more js path format ,remove log
    if (path.find(ASSETS_SCHEME) !=
        std::string::npos) {  // platform related path
#if OS_LINUX
      auto [result, assets_dir] = ::fml::paths::GetExecutableDirectoryPath();
#else
      auto [result, assets_dir] = lynx::base::GetModuleDirectoryPath();
#endif
      if (!result) {
        LOGE(
            "JSSourceLoaderDesktop::LoadJSSource GetModuleDirectoryPath "
            "failed");
        return "";
      }
      if (!tasm::LynxEnv::GetInstance().IsDevToolEnabled()) {
        auto real_start = path.find(ASSETS_SCHEME) + ASSETS_SCHEME.size();
        real_path = path.substr(real_start, path.size());
      } else {
        real_path = CORE_DEBUG_JS + ".js";
      }
      if (assets_dir.back() == DIR_SEP) {
        real_path = assets_dir + real_path;
      } else {
        real_path = assets_dir + DIR_SEP + real_path;
      }
    } else if (path.find(FILE_LYNX_SCHEME) != std::string::npos) {
      // Handle file://lynx?local:// format
      static const std::string LOCAL_SCHEME = "local://";
      auto real_start = path.find(LOCAL_SCHEME) + LOCAL_SCHEME.size();
      real_path = path.substr(real_start, path.size());

      // Remove characters after lynx.bundle
      const std::string bundle_suffix = ".lynx.bundle";
      size_t bundle_pos = real_path.find(bundle_suffix);
      if (bundle_pos != std::string::npos) {
        real_path = real_path.substr(0, bundle_pos + bundle_suffix.size());
      }

      // Use assets directory for local scheme
#if OS_LINUX
      auto [result, assets_dir] = ::fml::paths::GetExecutableDirectoryPath();
#else
      auto [result, assets_dir] = lynx::base::GetModuleDirectoryPath();
#endif
      if (!result) {
        LOGE(
            "JSSourceLoaderDesktop::LoadJSSource GetModuleDirectoryPath "
            "failed");
        return "";
      }
      std::string resources_dir = "resources";
      if (assets_dir.back() == DIR_SEP) {
        real_path = assets_dir + resources_dir + DIR_SEP + real_path;
      } else {
        real_path = assets_dir + DIR_SEP + resources_dir + DIR_SEP + real_path;
      }
    } else if (path.find(FILE_SCHEME) != std::string::npos) {
      auto real_start = path.find(FILE_SCHEME) + FILE_SCHEME.size();
      real_path = path.substr(real_start, path.size());
    } else {  // standard file path, do nothing
      real_path = path;
    }
  }
  LOGI("JSSourceLoaderDesktop::LoadJSSource load js source real path= "
       << real_path);
  res = LoadFileData(real_path);
  return res;
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
