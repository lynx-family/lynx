// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_JSVM_JSVM_DYN_LOAD_H_
#define CORE_RUNTIME_JSI_JSVM_JSVM_DYN_LOAD_H_

#include <ark_runtime/jsvm.h>
#include <deviceinfo.h>
#include <dlfcn.h>

#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "base/include/log/logging.h"

namespace lynx {
namespace piper {
static constexpr int kMinMajorVersion = 5;
static constexpr int kMinSeniorVersion = 0;
static constexpr int kMinFeatureVersion = 5;
static constexpr int kMinBuildVersion = 165;
static constexpr const char kJSVMLibraryPath[] = "/system/lib64/ndk/libjsvm.so";
static constexpr int kMinAPIVersion = 17;

class DeviceInfo {
 public:
  explicit DeviceInfo() { Parse(std::string(OH_GetOSFullName())); }
  int MajorVersion() const { return major_version_; }
  int SeniorVersion() const { return senior_version_; }
  int FeatureVersion() const { return feature_version_; }
  int BuildVersion() const { return build_version_; }

 private:
  void Parse(const std::string& input) {
    size_t hyphen_pos = input.find('-');
    if (hyphen_pos == std::string::npos) {
      return;
    }

    std::string version_part = input.substr(hyphen_pos + 1);

    std::vector<std::string> num_parts = SplitString(version_part, '.');
    if (num_parts.size() != 4) {
      return;
    }

    major_version_ = std::stoi(num_parts[0]);
    senior_version_ = std::stoi(num_parts[1]);
    feature_version_ = std::stoi(num_parts[2]);
    build_version_ = std::stoi(num_parts[3]);
  }

  std::vector<std::string> SplitString(const std::string& str, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream ss(str);
    std::string part;
    while (std::getline(ss, part, delimiter)) {
      if (!part.empty()) {
        parts.push_back(part);
      }
    }
    return parts;
  }

  int major_version_{0};
  int senior_version_{0};
  int feature_version_{0};
  int build_version_{0};
};

struct JSVMFunctionTable {
#define DECLARE(ret, name, params)    \
  using name##_ptr_t = ret(*) params; \
  name##_ptr_t name;
#include "core/runtime/jsi/jsvm/jsvm_declare.def"
#undef DECLARE
};

class DynamicLoader {
 public:
  explicit DynamicLoader(const char* lib_path) {
    if (!IsJsvmAvailable()) {
      return;
    }
    handle_ = dlopen(lib_path, RTLD_LAZY);
    dlerror();
  }

  ~DynamicLoader() {
    if (handle_) {
      dlclose(handle_);
    }
    if (jsvm_func_table_) {
      delete jsvm_func_table_;
    }
  }

  JSVMFunctionTable* GetOrLoadFuncTable() {
    if (jsvm_func_table_ || !handle_) {
      return jsvm_func_table_;
    }
    return LoadFuncTable();
  }

  bool IsJsvmAvailable() {
    int api_version = OH_GetSdkApiVersion();
    if (api_version < kMinAPIVersion) {
      LOGE("api_version: " << api_version << " < " << kMinAPIVersion);
      return false;
    }

    LOGI("majorVersion: " << device_info_.MajorVersion() << " seniorVersion: "
                          << device_info_.SeniorVersion() << " featureVersion: "
                          << device_info_.FeatureVersion()
                          << " buildVersion: " << device_info_.BuildVersion());
    int major_version = device_info_.MajorVersion();
    if (major_version != kMinMajorVersion) {
      return major_version > kMinMajorVersion;
    }

    int senior_version = device_info_.SeniorVersion();
    if (senior_version != kMinSeniorVersion) {
      return senior_version > kMinSeniorVersion;
    }

    int feature_version = device_info_.FeatureVersion();
    if (feature_version != kMinFeatureVersion) {
      return feature_version > kMinFeatureVersion;
    }

    return device_info_.BuildVersion() >= kMinBuildVersion;
  }

  static DynamicLoader* GetInstance() {
    static DynamicLoader instance(kJSVMLibraryPath);
    return &instance;
  }

  static JSVMFunctionTable* GetFuncTable() {
    return GetInstance()->GetOrLoadFuncTable();
  }

 private:
  JSVMFunctionTable* LoadFuncTable() {
    if (!handle_) {
      return nullptr;
    }
    jsvm_func_table_ = new JSVMFunctionTable();

#define DECLARE(ret, name, params)                                     \
  do {                                                                 \
    void* func = dlsym(handle_, #name);                                \
    jsvm_func_table_->name =                                           \
        func ? reinterpret_cast<JSVMFunctionTable::name##_ptr_t>(func) \
             : nullptr;                                                \
  } while (0);
#include "core/runtime/jsi/jsvm/jsvm_declare.def"
#undef DECLARE
    return jsvm_func_table_;
  }

  DeviceInfo device_info_{};
  void* handle_{nullptr};
  JSVMFunctionTable* jsvm_func_table_{nullptr};
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_JSVM_JSVM_DYN_LOAD_H_
