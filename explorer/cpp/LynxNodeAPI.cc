// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//
// Portions of this file are derived from react-native-node-api:
// https://github.com/callstackincubator/react-native-node-api
// Copyright (c) 2023 Callstack
// Licensed under the MIT License.

#include "LynxNodeAPI.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__ANDROID__) || defined(__OHOS__)
#include <dlfcn.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#endif

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined(__APPLE__) && TARGET_OS_IPHONE

#include <LynxWeakNodeAPI/headers/node_api.h>
#ifdef USE_WEAK_SUFFIX_NAPI
#include <LynxWeakNodeAPI/headers/weak_napi_defines.h>
#endif

#else

#include "third_party/weak-node-api/headers/node_api.h"
#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif
#endif

namespace lynx {
namespace explorer {

namespace {

#if defined(__ANDROID__) || defined(__OHOS__) || defined(_WIN32)

static bool EndsWith(const std::string& s, const char* suffix) {
  const size_t s_len = s.size();
  const size_t suf_len = std::strlen(suffix);
  if (suf_len > s_len) return false;
  return s.compare(s_len - suf_len, suf_len, suffix) == 0;
}

static bool StartsWith(const std::string& s, const char* prefix) {
  const size_t pre_len = std::strlen(prefix);
  if (pre_len > s.size()) return false;
  return s.compare(0, pre_len, prefix) == 0;
}

static bool ContainsPathSeparator(const std::string& s) {
  return s.find('/') != std::string::npos || s.find('\\') != std::string::npos;
}

static void AddUnique(std::vector<std::string>& out, std::string v) {
  if (v.empty()) return;
  if (std::find(out.begin(), out.end(), v) != out.end()) return;
  out.emplace_back(std::move(v));
}

static std::vector<std::string> BuildAddonCandidates(
    const std::string& library_name) {
  std::vector<std::string> candidates;

  // 1) Try caller-provided name/path first. If it's a bare name we still try it
  // as-is because Android/ELF loaders can resolve from default search paths.
  AddUnique(candidates, library_name);

  const bool has_path =
      ContainsPathSeparator(library_name) || StartsWith(library_name, "@");
  const bool has_ext =
      EndsWith(library_name, ".node") || EndsWith(library_name, ".so") ||
      EndsWith(library_name, ".dylib") || EndsWith(library_name, ".dll");

  if (has_path) {
    // If user provides a path without extension, allow common suffixes.
    if (!has_ext) {
      AddUnique(candidates, library_name + ".node");
      AddUnique(candidates, library_name + ".so");
      AddUnique(candidates, library_name + ".dylib");
      AddUnique(candidates, library_name + ".dll");
    }
    return candidates;
  }

#if defined(__ANDROID__) || defined(__OHOS__)
  // Android/Harmony: native libraries are typically extracted to the app's
  // native library directory. dlopen() can resolve by soname ("libxxx.so")
  // without an absolute path.
  const std::string name = library_name;

  const bool has_lib_prefix = StartsWith(name, "lib");

  // If caller passed ".node", also try the ".so" variant (packagers may
  // rename).
  if (EndsWith(name, ".node")) {
    AddUnique(candidates, name.substr(0, name.size() - 5) + ".so");
  }
  if (!has_ext) {
    if (has_lib_prefix) {
      AddUnique(candidates, name + ".so");
      AddUnique(candidates, name + ".node");
    } else {
      AddUnique(candidates, "lib" + name + ".so");
      AddUnique(candidates, "lib" + name + ".node");
    }
  }
#elif defined(_WIN32)
  // Windows: addons are typically distributed as .node or .dll files placed
  // beside the executable or in a directory configured through the standard
  // DLL search path.
  const std::string name = library_name;

  if (EndsWith(name, ".node")) {
    AddUnique(candidates, name.substr(0, name.size() - 5) + ".dll");
  }
  if (!has_ext) {
    AddUnique(candidates, name + ".node");
    AddUnique(candidates, name + ".dll");
  }
#endif

  return candidates;
}

#endif  // defined(__ANDROID__) || defined(__OHOS__) || defined(_WIN32)

}  // namespace

void LynxNodeAPI::PutEnvByToken(uint64_t token_id, void* napi_env_ptr) {
  std::lock_guard<std::mutex> lock(env_mutex_);
  if (token_id == 0 || napi_env_ptr == nullptr) {
    tokenEnvMap_.erase(token_id);
    return;
  }
  tokenEnvMap_[token_id] = napi_env_ptr;
}

void LynxNodeAPI::RemoveEnvByToken(uint64_t token_id) {
  std::lock_guard<std::mutex> lock(env_mutex_);
  tokenEnvMap_.erase(token_id);
}

void LynxNodeAPI::RegisterStaticNodeAddon(const std::string& addon_name,
                                          void* register_func) {
  std::lock_guard<std::mutex> lock(env_mutex_);
  if (addon_name.empty() || register_func == nullptr) {
    return;
  }
  staticNodeAddons_[addon_name] = register_func;
  nodeAddons_.erase(addon_name);
}

void RegisterStaticNodeAddon(const char* addon_name, void* register_func) {
  if (addon_name == nullptr || register_func == nullptr) {
    return;
  }
  LynxNodeAPI::GetInstance().RegisterStaticNodeAddon(addon_name, register_func);
}

void LynxNodeAPI::RequireNodeAddonByToken(uint64_t token_id,
                                          const std::string& addon_name) {
  void* env_ptr = nullptr;
  {
    std::lock_guard<std::mutex> lock(env_mutex_);
    auto it = tokenEnvMap_.find(token_id);
    if (it == tokenEnvMap_.end()) {
      return;
    }
    env_ptr = it->second;
  }
  if (env_ptr == nullptr) {
    return;
  }
  RequireNodeAddon(env_ptr, addon_name);
}

#if defined(__ANDROID__) || defined(__OHOS__)
struct PosixLoader {
  using Module = void*;
  using Symbol = void*;

  static Module loadLibrary(const char* filePath) {
    Module result = dlopen(filePath, RTLD_NOW | RTLD_LOCAL);
    return result;
  }

  static Symbol getSymbol(Module library, const char* name) {
    Symbol result = dlsym(library, name);
    return result;
  }

  static void unloadLibrary(Module library) {
    if (NULL != library) {
      dlclose(library);
    }
  }
};
#endif

#if defined(_WIN32)
struct WinLoader {
  using Module = HMODULE;
  using Symbol = FARPROC;

  static Module loadLibrary(const char* filePath) {
    return LoadLibraryA(filePath);
  }

  static Symbol getSymbol(Module library, const char* name) {
    return GetProcAddress(library, name);
  }

  static void unloadLibrary(Module library) {
    if (library != NULL) {
      FreeLibrary(library);
    }
  }
};
#endif

void LynxNodeAPI::RequireNodeAddon(void* napi_env_ptr,
                                   const std::string& addon_name) {
  if (napi_env_ptr == nullptr) {
    return;
  }

  NodeAddon addon;
  {
    std::lock_guard<std::mutex> lock(env_mutex_);

    auto [it, inserted] = nodeAddons_.try_emplace(addon_name);
    NodeAddon& cached_addon = it->second;

    if (inserted) {
      if (!LoadNodeAddon(cached_addon, addon_name)) {
        nodeAddons_.erase(it);
        return;  // Load failed
      }
    }

    addon = cached_addon;
  }

  InitializeNodeModule(napi_env_ptr, addon);
}

// Note: On Android, HarmonyOS, and Windows, the dynamic library loading logic
// here is for demonstration purposes only. In production use, strong
// constraints and fallback mechanisms must be implemented to prevent attempts
// to load unexpected shared libraries.
bool LynxNodeAPI::LoadNodeAddon(NodeAddon& addon,
                                const std::string& libraryName) const {
#if defined(__APPLE__)
  auto it = staticNodeAddons_.find(libraryName);
  if (it == staticNodeAddons_.end() || it->second == nullptr) {
    std::fprintf(
        stderr,
        "Failed to find statically linked Node Addon '%s'. Ensure it is "
        "linked into the app so its constructor can register itself before "
        "calling requireNodeAddon.\n",
        libraryName.c_str());
    return false;
  }
  addon.moduleHandle = nullptr;
  addon.generatedName = libraryName;
  addon.init = it->second;
  return true;
#elif defined(_WIN32) || defined(__ANDROID__) || defined(__OHOS__)
#if defined(_WIN32)
  using LoaderPolicy = WinLoader;
#else
  using LoaderPolicy = PosixLoader;
#endif
  // Reject untrusted addon names that could be used for path traversal or
  // loading unexpected libraries (e.g., via '/', '\\', '..', '@', drive
  // letters). Only allow a simple module name: [A-Za-z0-9_.-]
  if (libraryName.empty() || libraryName.size() > 128 ||
      libraryName.find("..") != std::string::npos ||
      libraryName.find('/') != std::string::npos ||
      libraryName.find('\\') != std::string::npos ||
      libraryName.find('@') != std::string::npos ||
      libraryName.find(':') != std::string::npos ||
      libraryName.find('\0') != std::string::npos ||
      libraryName.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOP"
                                    "QRSTUVWXYZ0123456789_.-") !=
          std::string::npos) {
    std::fprintf(stderr, "Failed to load Node Addon: invalid name '%s'\n",
                 libraryName.c_str());
    return false;
  }

  LoaderPolicy::Symbol initFn = NULL;

  const std::vector<std::string> candidates = BuildAddonCandidates(libraryName);

#if defined(_WIN32)
  DWORD last_error = ERROR_SUCCESS;
#else
  const char* last_error = nullptr;
#endif
  for (const auto& path : candidates) {
#if defined(_WIN32)
    SetLastError(ERROR_SUCCESS);
#else
    // Clear any previous dl error.
    (void)dlerror();
#endif

    LoaderPolicy::Module library = LoaderPolicy::loadLibrary(path.c_str());
    if (library == nullptr) {
#if defined(_WIN32)
      last_error = GetLastError();
#else
      last_error = dlerror();
#endif
      continue;
    }

    addon.moduleHandle = library;
    addon.generatedName = libraryName;  // key under __lynx_node_addon_exports__

    initFn = LoaderPolicy::getSymbol(library, "napi_register_module_v1");
    if (initFn != nullptr) {
      addon.init = initFn;
      return true;
    }

    // Loaded but missing expected symbol, unload and continue.
    LoaderPolicy::unloadLibrary(library);
    addon.moduleHandle = nullptr;
    addon.init = nullptr;

#if defined(_WIN32)
    last_error = GetLastError();
#else
    last_error = dlerror();
#endif
  }

#if defined(_WIN32)
  if (last_error != ERROR_SUCCESS) {
    std::fprintf(
        stderr,
        "Failed to load Node Addon '%s'. Last LoadLibrary/GetProcAddress "
        "error: %lu\n",
        libraryName.c_str(), static_cast<unsigned long>(last_error));
  }
#else
  if (last_error != nullptr) {
    std::fprintf(
        stderr, "Failed to load Node Addon '%s'. Last dlopen/dlsym error: %s\n",
        libraryName.c_str(), last_error);
  }
#endif
  return false;
#else
  (void)addon;
  (void)libraryName;
  return false;
#endif  // defined(__APPLE__)
}

bool LynxNodeAPI::InitializeNodeModule(void* env_ptr, NodeAddon& addon) {
  if (addon.init == nullptr) return false;
  if (env_ptr == nullptr) return false;

  napi_env env = reinterpret_cast<napi_env>(env_ptr);
  napi_status status = napi_ok;

  napi_value exports;
  status = napi_create_object(env, &exports);
  if (status != napi_ok) return false;

  // Call the addon init function to populate the "exports" object
  auto init = reinterpret_cast<napi_addon_register_func>(addon.init);
  exports = init(env, exports);

  napi_value global;
  status = napi_get_global(env, &global);
  if (status != napi_ok) return false;

  napi_value exports_obj;
  status = napi_get_named_property(env, global, "__lynx_node_addon_exports__",
                                   &exports_obj);

  napi_valuetype type;
  if (status != napi_ok) {
    type = napi_undefined;
  } else {
    status = napi_typeof(env, exports_obj, &type);
    if (status != napi_ok) {
      type = napi_undefined;
    }
  }

  // Ensure __lynx_node_addon_exports__ is an object; otherwise
  // recreate/overwrite it.
  if (type != napi_object) {
    status = napi_create_object(env, &exports_obj);
    if (status != napi_ok) return false;
    status = napi_set_named_property(env, global, "__lynx_node_addon_exports__",
                                     exports_obj);
    if (status != napi_ok) return false;
  }

  status = napi_set_named_property(env, exports_obj,
                                   addon.generatedName.c_str(), exports);
  return status == napi_ok;
}

}  // namespace explorer
}  // namespace lynx
