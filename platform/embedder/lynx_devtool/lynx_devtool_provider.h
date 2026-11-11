// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_DEVTOOL_LYNX_DEVTOOL_PROVIDER_H_
#define PLATFORM_EMBEDDER_LYNX_DEVTOOL_LYNX_DEVTOOL_PROVIDER_H_

#include <memory>

#include "core/base/lynx_export.h"
#include "platform/embedder/lynx_devtool/lynx_devtool.h"
#include "platform/embedder/public/capi/lynx_env_capi.h"

namespace lynx {
namespace runtime {
namespace js {
class LynxModuleManager;
}
}  // namespace runtime
namespace embedder {

class EmbedderDevToolProvider {
 public:
  virtual ~EmbedderDevToolProvider() = default;
  virtual std::unique_ptr<EmbedderDevTool> Create(
      devtool::LynxDevToolProxy* proxy) = 0;
  virtual void InitializeModuleManager(
      runtime::js::LynxModuleManager* module_manager) = 0;
  virtual void SetAppInfo(const char* name, const char* value) = 0;
  virtual void SetDevToolEnabled(bool enabled) = 0;
  virtual bool IsDevToolEnabled() = 0;
  virtual int GetLocalPort() = 0;
  virtual bool ConnectDevTool(const char* url) = 0;
  virtual void SetOpenCardCallback(lynx_env_open_card_callback callback,
                                   void* callback_context) = 0;
  virtual void SetCloseCardCallback(lynx_env_close_card_callback callback,
                                    void* callback_context) = 0;
  virtual void SetLogBoxEnabled(bool enabled) {}
  virtual bool IsLogBoxEnabled() { return false; }
};

// Internal integration for libraries built from the same source, toolchain and
// C++ ABI configuration. Lite builds accept one provider; full builds reject
// external registration. Register on the host UI thread during startup, before
// environment calls or renderer creation. The provider is borrowed and must
// remain alive, together with its library, for the process lifetime.
LYNX_EXPORT_FOR_DEVTOOL bool RegisterEmbedderDevToolProvider(
    EmbedderDevToolProvider* provider);

// Called once during library initialization in builtin mode.
void RegisterBuiltinEmbedderDevToolProvider(EmbedderDevToolProvider* provider);
std::unique_ptr<EmbedderDevTool> CreateEmbedderDevTool(
    devtool::LynxDevToolProxy* proxy);
void InitializeEmbedderDevToolModuleManager(
    runtime::js::LynxModuleManager* module_manager);

void SetEmbedderDevToolAppInfo(const char* name, const char* value);
void SetEmbedderDevToolEnabled(bool enabled);
bool IsEmbedderDevToolEnabled();
int GetEmbedderDevToolLocalPort();
bool ConnectEmbedderDevTool(const char* url);
void SetEmbedderDevToolOpenCardCallback(lynx_env_open_card_callback callback,
                                        void* callback_context);
void SetEmbedderDevToolCloseCardCallback(lynx_env_close_card_callback callback,
                                         void* callback_context);
void SetEmbedderLogBoxEnabled(bool enabled);
bool IsEmbedderLogBoxEnabled();

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_DEVTOOL_LYNX_DEVTOOL_PROVIDER_H_
