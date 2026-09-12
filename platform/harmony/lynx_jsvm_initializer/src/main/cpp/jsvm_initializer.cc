// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/harmony/lynx_jsvm_initializer/src/main/cpp/jsvm_initializer.h"

#include <ark_runtime/jsvm.h>
#include <ark_runtime/jsvm_types.h>
#include <dlfcn.h>

#include <algorithm>
#include <array>
#include <mutex>
#include <string>
#include <vector>

namespace {
using JSVMInitFn = JSVM_Status (*)(const JSVM_InitOptions *options);
std::once_flag jsvm_init_once;
std::mutex jsvm_options_mutex;
bool jsvm_init_started = false;
bool has_jsvm_options = false;
std::array<int32_t, 3> jsvm_options{};
constexpr std::array<const char *, 3> kOptionNames = {
    "--incremental-marking-hard-trigger", "--min-semi-space-size",
    "--max-semi-space-size"};
void *jsvm_lib_handle = nullptr;
JSVMInitFn jsvm_init_fn = nullptr;
JSVM_Status jsvm_init_status = static_cast<JSVM_Status>(1);

JSVMInitFn ResolveJSVMInitFn() {
  if (jsvm_init_fn) {
    return jsvm_init_fn;
  }

  if (!jsvm_lib_handle) {
    jsvm_lib_handle = dlopen("/system/lib64/ndk/libjsvm.so", RTLD_LAZY);
  }
  if (!jsvm_lib_handle) {
    return nullptr;
  }

  jsvm_init_fn =
      reinterpret_cast<JSVMInitFn>(dlsym(jsvm_lib_handle, "OH_JSVM_Init"));
  return jsvm_init_fn;
}

JSVM_Status InitWithOptions(JSVMInitFn init_fn, const JSVM_InitOptions *options,
                            const std::array<int32_t, 3> &values) {
  JSVM_InitOptions merged = options ? *options : JSVM_InitOptions{};
  const int original_argc = merged.argc && merged.argv ? *merged.argc : 0;
  std::vector<char *> original_args;
  if (original_argc > 0) {
    original_args.assign(merged.argv, merged.argv + original_argc);
  }
  char program_name[] = "jsvm";
  std::vector<char *> args{original_args.empty() ? program_name
                                                 : original_args.front()};
  std::array<std::string, 3> flags;
  for (size_t i = 0; i < flags.size(); ++i) {
    flags[i] = std::string(kOptionNames[i]) + "=" + std::to_string(values[i]);
    args.push_back(flags[i].data());
  }
  bool parsing_flags = true;
  for (int i = 1; i < original_argc; ++i) {
    std::string name = original_args[i];
    if (name == "--") {
      parsing_flags = false;
    }
    const auto separator = name.find('=');
    name.resize(separator == std::string::npos ? name.size() : separator);
    std::replace(name.begin(), name.end(), '_', '-');
    if (parsing_flags && std::find(kOptionNames.begin(), kOptionNames.end(),
                                   name) != kOptionNames.end()) {
      // Accept both --flag=value and --flag value, including V8's underscore
      // aliases, so the saved configuration has no competing occurrences.
      if (separator == std::string::npos && i + 1 < original_argc) {
        ++i;
      }
    } else {
      args.push_back(original_args[i]);
    }
  }
  int argc = static_cast<int>(args.size());
  args.push_back(nullptr);
  merged.argc = &argc;
  merged.argv = args.data();
  const auto status = init_fn(&merged);
  if (merged.removeFlags && !original_args.empty()) {
    // Reflect removal back to the caller without exposing our temporary flags
    // or program name. Unrecognized caller arguments retain their pointers.
    int remaining = 0;
    for (int i = 0; i < argc; ++i) {
      if (std::find(original_args.begin(), original_args.end(), args[i]) !=
          original_args.end()) {
        options->argv[remaining++] = args[i];
      }
    }
    for (int i = remaining; i < original_argc; ++i) {
      options->argv[i] = nullptr;
    }
    *options->argc = remaining;
  }
  return status;
}
}  // namespace

__attribute__((visibility("default"))) bool Lynx_JSVM_SetInitOptions(
    int32_t incremental_marking_hard_trigger, int32_t min_semi_space_size,
    int32_t max_semi_space_size) {
  if (incremental_marking_hard_trigger < 0 ||
      incremental_marking_hard_trigger > 100 || min_semi_space_size <= 0 ||
      max_semi_space_size < min_semi_space_size) {
    return false;
  }
  std::lock_guard<std::mutex> lock(jsvm_options_mutex);
  if (jsvm_init_started) {
    return false;
  }
  jsvm_options = {incremental_marking_hard_trigger, min_semi_space_size,
                  max_semi_space_size};
  has_jsvm_options = true;
  return true;
}

__attribute__((visibility("default"))) JSVM_Status Lynx_JSVM_Common_Init(
    const JSVM_InitOptions *options) {
  std::call_once(jsvm_init_once, [options]() {
    bool configured;
    std::array<int32_t, 3> values;
    {
      std::lock_guard<std::mutex> lock(jsvm_options_mutex);
      jsvm_init_started = true;
      configured = has_jsvm_options;
      values = jsvm_options;
    }
    auto init_fn = ResolveJSVMInitFn();
    if (!init_fn) {
      return;
    }
    jsvm_init_status = configured ? InitWithOptions(init_fn, options, values)
                                  : init_fn(options);
  });
  return jsvm_init_status;
}
