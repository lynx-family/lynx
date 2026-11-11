// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_devtool/lynx_devtool_provider.h"

namespace lynx {
namespace embedder {
namespace {

EmbedderDevToolProvider*& GetDevToolProvider() {
  static EmbedderDevToolProvider* provider = nullptr;
  return provider;
}

}  // namespace

bool RegisterEmbedderDevToolProvider(EmbedderDevToolProvider* provider) {
#if ENABLE_EMBEDDER_LITE
  auto& registered = GetDevToolProvider();
  if (!provider || registered) {
    return false;
  }
  registered = provider;
  return true;
#else
  return false;
#endif
}

void RegisterBuiltinEmbedderDevToolProvider(EmbedderDevToolProvider* provider) {
#if !ENABLE_EMBEDDER_LITE
  GetDevToolProvider() = provider;
#endif
}

std::unique_ptr<EmbedderDevTool> CreateEmbedderDevTool(
    devtool::LynxDevToolProxy* proxy) {
  auto* provider = GetDevToolProvider();
  return provider ? provider->Create(proxy) : nullptr;
}

void InitializeEmbedderDevToolModuleManager(
    runtime::js::LynxModuleManager* module_manager) {
  auto* provider = GetDevToolProvider();
  if (provider) {
    provider->InitializeModuleManager(module_manager);
  }
}

void SetEmbedderDevToolAppInfo(const char* name, const char* value) {
  auto* provider = GetDevToolProvider();
  if (provider) {
    provider->SetAppInfo(name ? name : "", value ? value : "");
  }
}

void SetEmbedderDevToolEnabled(bool enabled) {
  auto* provider = GetDevToolProvider();
  if (provider) {
    provider->SetDevToolEnabled(enabled);
  }
}

bool IsEmbedderDevToolEnabled() {
  auto* provider = GetDevToolProvider();
  return provider && provider->IsDevToolEnabled();
}

int GetEmbedderDevToolLocalPort() {
  auto* provider = GetDevToolProvider();
  return provider ? provider->GetLocalPort() : -1;
}

bool ConnectEmbedderDevTool(const char* url) {
  auto* provider = GetDevToolProvider();
  return provider && provider->ConnectDevTool(url ? url : "");
}

void SetEmbedderDevToolOpenCardCallback(lynx_env_open_card_callback callback,
                                        void* callback_context) {
  auto* provider = GetDevToolProvider();
  if (provider) {
    provider->SetOpenCardCallback(callback, callback_context);
  }
}

void SetEmbedderDevToolCloseCardCallback(lynx_env_close_card_callback callback,
                                         void* callback_context) {
  auto* provider = GetDevToolProvider();
  if (provider) {
    provider->SetCloseCardCallback(callback, callback_context);
  }
}

void SetEmbedderLogBoxEnabled(bool enabled) {
  auto* provider = GetDevToolProvider();
  if (provider) {
    provider->SetLogBoxEnabled(enabled);
  }
}

bool IsEmbedderLogBoxEnabled() {
  auto* provider = GetDevToolProvider();
  return provider && provider->IsLogBoxEnabled();
}

}  // namespace embedder
}  // namespace lynx
