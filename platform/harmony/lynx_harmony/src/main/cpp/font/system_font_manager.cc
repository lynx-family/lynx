// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/font/system_font_manager.h"

#include <dirent.h>

#include <cerrno>
#include <cstring>
#include <string>
#include <tuple>
#include <utility>

#include "base/include/log/logging.h"

namespace lynx {
namespace tasm {
namespace harmony {

static bool EndsWith(const std::string& str, const std::string& suffix) {
  if (str.length() < suffix.length()) {
    return false;
  }
  return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

static std::string GetFontFamilyName(const std::string& path) {
  std::string font_family_name = "";
  DIR* dir;
  struct dirent* ent;
  if ((dir = opendir(path.c_str())) == nullptr) {
    return font_family_name;
  }
  while ((ent = readdir(dir)) != nullptr) {
    if (!std::strcmp(ent->d_name, ".") || !std::strcmp(ent->d_name, "..")) {
      continue;
    }
    if (EndsWith(ent->d_name, ".ttf")) {
      font_family_name = ent->d_name;
      break;
    }
  }
  closedir(dir);
  return font_family_name;
}

static bool IsFontFileExistInPath(const std::string& path) {
  DIR* dir;
  struct dirent* ent;
  bool is_flag_file_exist = false;
  bool is_font_dir_exist = false;
  if ((dir = opendir(path.c_str())) == nullptr) {
    LOGE("try open file ERROR:" << errno);
    return false;
  }
  while ((ent = readdir(dir)) != nullptr) {
    if (!std::strcmp(ent->d_name, ".") || !std::strcmp(ent->d_name, "..")) {
      continue;
    }

    if (!std::strcmp(ent->d_name, "flag")) {
      is_flag_file_exist = true;
    } else if (!std::strcmp(ent->d_name, "fonts")) {
      is_font_dir_exist = true;
    }
  }
  closedir(dir);
  if (is_flag_file_exist && is_font_dir_exist) {
    LOGI("font path exist");
    return true;
  }
  return false;
}

struct CallbackContext {
  CallbackContext(SystemFontCallback callback)
      : callback_(std::move(callback)) {}
  SystemFontCallback callback_;
  napi_async_work work_;
};

void SystemFontManager::GetSystemFont(napi_env env, SystemFontCallback callback,
                                      bool force_update) {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  if (!system_font_family_.empty() && !force_update) {
    callback(system_font_family_, system_font_path_);
    return;
  }

  auto* context = new CallbackContext(std::move(callback));

  napi_value work_name;
  napi_async_work async_work;
  napi_create_string_utf8(env, "LynxSystemFontManager::GetSystemFont",
                          NAPI_AUTO_LENGTH, &work_name);

  napi_create_async_work(
      env, nullptr, work_name,
      [](napi_env env, void* data) {
        CallbackContext* context = reinterpret_cast<CallbackContext*>(data);
        if (context) {
          // run on worker thread
          SystemFontManager::GetInstance().CheckAndGetSystemFont(
              std::move(context->callback_));
        }
      },
      [](napi_env env, napi_status status, void* data) {
        CallbackContext* context = reinterpret_cast<CallbackContext*>(data);
        napi_delete_async_work(env, context->work_);
        delete context;
      },
      reinterpret_cast<void*>(context), &async_work);
  context->work_ = async_work;
  napi_queue_async_work(env, async_work);
}

bool SystemFontManager::CheckAndGetSystemFont(SystemFontCallback callback) {
  std::string family_name = "";
  std::string path = "/data/themes/a/app";
  if (!IsFontFileExistInPath(path)) {
    path = "/data/themes/b/app";
    if (!IsFontFileExistInPath(path)) {
      return false;
    }
  }
  path = path.append("/fonts/");
  family_name = GetFontFamilyName(path);
  if (family_name.empty()) {
    return false;
  }
  path = path.append(family_name);

  callback(family_name, path);
  {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    system_font_family_ = std::move(family_name);
    system_font_path_ = std::move(path);
  }
  return true;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
