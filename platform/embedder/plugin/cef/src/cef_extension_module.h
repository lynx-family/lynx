// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_CEF_EXTENSION_MODULE_H_
#define PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_CEF_EXTENSION_MODULE_H_

#include "platform/embedder/plugin/cef/include/cef_extension_module_creator.h"

namespace lynx {
namespace plugin {
namespace embedder {

class CEFExtensionModule : public lynx::pub::LynxExtensionModule {
 public:
  void OnLynxViewCreate(lynx_view_t* lynx_view) override;
};

}  // namespace embedder
}  // namespace plugin
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_CEF_EXTENSION_MODULE_H_
